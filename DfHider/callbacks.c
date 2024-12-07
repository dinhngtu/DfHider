#include "stdafx.h"
#include "callbacks.h"
#include "instance.h"

static BOOLEAN DfIsHiddenName(CONST PFLT_FILE_NAME_INFORMATION fnInfo) {
    if (!(fnInfo->NamesParsed & FLTFL_FILE_NAME_PARSED_STREAM) || !fnInfo->Stream.Length)
        if (fnInfo->FinalComponent.Length > 0 && fnInfo->FinalComponent.Buffer[0] == L'.')
            return TRUE;
    return FALSE;
}

static NTSTATUS DfGetParsedFileName(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Outptr_ PFLT_FILE_NAME_INFORMATION* FileNameInformation
) {
    NTSTATUS status;
    *FileNameInformation = NULL;

    status = FltGetFileNameInformation(
        CallbackData,
        NameOptions,
        FileNameInformation);
    if (!NT_SUCCESS(status))
        return status;

    status = FltParseFileNameInformation(*FileNameInformation);
    if (!NT_SUCCESS(status)) {
        FltReleaseFileNameInformation(*FileNameInformation);
        *FileNameInformation = NULL;
    }
    return status;
}

FLT_POSTOP_CALLBACK_STATUS
FLTAPI
DfPostQueryInformation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    NTSTATUS status = Data->IoStatus.Status;
    PDF_INSTANCE_CONTEXT instance = NULL;
    FSRTL_MUP_PROVIDER_INFO_LEVEL_1 mupProvider = { 0 };
    ULONG mupProviderSize = sizeof(mupProvider);
    PFLT_FILE_NAME_INFORMATION fnInfo = NULL;
    FILE_INFORMATION_CLASS infoClass = Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;
    PVOID infoBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;

    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    if (!NT_SUCCESS(status))
        return FLT_POSTOP_FINISHED_PROCESSING;

    status = FltGetInstanceContext(FltObjects->Instance, &instance);
    if (!NT_SUCCESS(status))
        return FLT_POSTOP_FINISHED_PROCESSING;

    if (!instance->IsMup)
        goto done_context;

    status = FsRtlMupGetProviderInfoFromFileObject(
        FltObjects->FileObject,
        1,
        &mupProvider,
        &mupProviderSize);
    if (!NT_SUCCESS(status))
        goto done_context;

    if (mupProvider.ProviderId != instance->P9ProviderId)
        goto done_context;

    status = DfGetParsedFileName(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &fnInfo);
    if (!NT_SUCCESS(status))
        goto done_context;

    if (!DfIsHiddenName(fnInfo))
        goto done_fnInfo;

    switch (infoClass) {
    case FileBasicInformation:
        ((PFILE_BASIC_INFORMATION)infoBuffer)->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
        break;
    case FileAllInformation:
        ((PFILE_ALL_INFORMATION)infoBuffer)->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
        break;
    case FileAttributeTagInformation:
        ((PFILE_ATTRIBUTE_TAG_INFORMATION)infoBuffer)->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
        break;
    default:
        break;
    }

done_fnInfo:
    FltReleaseFileNameInformation(fnInfo);
done_context:
    FltReleaseContext(instance);
    return FLT_POSTOP_FINISHED_PROCESSING;
}

#define DO_HIDE_FILE(T, data, infoBuffer) \
    { \
        T di = (infoBuffer); \
        while ((ULONG_PTR)(di + 1) <= (ULONG_PTR)(infoBuffer) + (data)->Iopb->Parameters.DirectoryControl.QueryDirectory.Length) { \
            if (di->FileNameLength >= sizeof(WCHAR) && di->FileName[0] == L'.') \
                di->FileAttributes |= FILE_ATTRIBUTE_HIDDEN; \
            if (!di->NextEntryOffset) \
                break; \
            di = (T)((ULONG_PTR)di + di->NextEntryOffset); \
        } \
    }

FLT_POSTOP_CALLBACK_STATUS
FLTAPI
DfPostDirectoryControl(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    NTSTATUS status = Data->IoStatus.Status;
    FSRTL_MUP_PROVIDER_INFO_LEVEL_1 mupProvider = { 0 };
    ULONG mupProviderSize = sizeof(mupProvider);
    PDF_INSTANCE_CONTEXT instance = NULL;
    FILE_INFORMATION_CLASS infoClass;
    PVOID infoBuffer;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    if (Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)
        return FLT_POSTOP_FINISHED_PROCESSING;

    status = FltGetInstanceContext(FltObjects->Instance, &instance);
    if (!NT_SUCCESS(status))
        return FLT_POSTOP_FINISHED_PROCESSING;

    if (!instance->IsMup)
        goto done_context;

    status = FsRtlMupGetProviderInfoFromFileObject(
        FltObjects->FileObject,
        1,
        &mupProvider,
        &mupProviderSize);
    if (!NT_SUCCESS(status))
        goto done_context;

    if (mupProvider.ProviderId != instance->P9ProviderId)
        goto done_context;

    infoClass = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
    infoBuffer = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;

    switch (infoClass) {
    case FileBothDirectoryInformation:
        DO_HIDE_FILE(PFILE_BOTH_DIR_INFORMATION, Data, infoBuffer);
        break;
    case FileDirectoryInformation:
        DO_HIDE_FILE(PFILE_DIRECTORY_INFORMATION, Data, infoBuffer);
        break;
    case FileFullDirectoryInformation:
        DO_HIDE_FILE(PFILE_FULL_DIR_INFORMATION, Data, infoBuffer);
        break;
    case FileIdBothDirectoryInformation:
        DO_HIDE_FILE(PFILE_ID_BOTH_DIR_INFORMATION, Data, infoBuffer);
        break;
    case FileIdFullDirectoryInformation:
        DO_HIDE_FILE(PFILE_ID_FULL_DIR_INFORMATION, Data, infoBuffer);
        break;
    default:
        break;
    }

done_context:
    FltReleaseContext(instance);
    return FLT_POSTOP_FINISHED_PROCESSING;
}

#undef DO_HIDE_FILE
