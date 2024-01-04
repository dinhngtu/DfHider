#include "callbacks.h"
#include "instance.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DfPostCreate)
#pragma alloc_text(PAGE, DfPostQueryInformation)
#pragma alloc_text(PAGE, DfPostDirectoryControl)
#endif

static BOOLEAN DfIsHiddenName(CONST PFLT_FILE_NAME_INFORMATION fnInfo) {
    if (!(fnInfo->NamesParsed & FLTFL_FILE_NAME_PARSED_STREAM) || !fnInfo->Stream.Length)
    {
        if (fnInfo->FinalComponent.Length > 0 && fnInfo->FinalComponent.Buffer[0] == L'.')
            return TRUE;
    }
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
DfPostCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    NTSTATUS status = Data->IoStatus.Status;
    PDF_INSTANCE_CONTEXT instance = NULL;
    PFLT_FILE_NAME_INFORMATION fnInfo = NULL;
    FILE_BASIC_INFORMATION fileInfo = { 0 };

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    if (!NT_SUCCESS(status) ||
        (status == STATUS_REPARSE) ||
        (Data->IoStatus.Information != FILE_CREATED &&
            Data->IoStatus.Information != FILE_SUPERSEDED))
        return FLT_POSTOP_FINISHED_PROCESSING;

    status = FltGetInstanceContext(FltObjects->Instance, &instance);
    if (!NT_SUCCESS(status))
        return FLT_POSTOP_FINISHED_PROCESSING;

    if (!instance->IsDisk)
        goto done_context;

    status = DfGetParsedFileName(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &fnInfo);
    if (!NT_SUCCESS(status))
        goto done_context;

    if (!DfIsHiddenName(fnInfo))
        goto done_fnInfo;

    status = FltQueryInformationFile(
        FltObjects->Instance,
        FltObjects->FileObject,
        &fileInfo,
        sizeof(fileInfo),
        FileBasicInformation,
        NULL);
    if (!NT_SUCCESS(status)) {
        DbgPrint("FltQueryInformationFile failed: %#lx", status);
        goto done_fnInfo;
    }

    if (fileInfo.FileAttributes & FILE_ATTRIBUTE_HIDDEN)
        goto done_fnInfo;
    fileInfo.FileAttributes |= FILE_ATTRIBUTE_HIDDEN;

    status = FltSetInformationFile(
        FltObjects->Instance,
        FltObjects->FileObject,
        &fileInfo,
        sizeof(fileInfo),
        FileBasicInformation);
    if (!NT_SUCCESS(status)) {
        DbgPrint("FltSetInformationFile failed: %#lx", status);
        goto done_fnInfo;
    }

done_fnInfo:
    FltReleaseFileNameInformation(fnInfo);
done_context:
    FltReleaseContext(instance);
    return FLT_POSTOP_FINISHED_PROCESSING;
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

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

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

    PAGED_CODE();

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
    case FileBothDirectoryInformation: {
        PFILE_BOTH_DIR_INFORMATION di = infoBuffer;
        while ((ULONG_PTR)(di + 1) <= (ULONG_PTR)infoBuffer + Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length) {
            if (di->FileNameLength >= sizeof(WCHAR) && di->FileName[0] == L'.')
                di->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            if (!di->NextEntryOffset)
                break;
            di = (PFILE_BOTH_DIR_INFORMATION)((ULONG_PTR)di + di->NextEntryOffset);
        }
        break;
    }
    case FileDirectoryInformation: {
        PFILE_DIRECTORY_INFORMATION di = infoBuffer;
        while ((ULONG_PTR)(di + 1) <= (ULONG_PTR)infoBuffer + Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length) {
            if (di->FileNameLength >= sizeof(WCHAR) && di->FileName[0] == L'.')
                di->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            if (!di->NextEntryOffset)
                break;
            di = (PFILE_DIRECTORY_INFORMATION)((ULONG_PTR)di + di->NextEntryOffset);
        }
        break;
    }
    case FileFullDirectoryInformation: {
        PFILE_FULL_DIR_INFORMATION di = infoBuffer;
        while ((ULONG_PTR)(di + 1) <= (ULONG_PTR)infoBuffer + Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length) {
            if (di->FileNameLength >= sizeof(WCHAR) && di->FileName[0] == L'.')
                di->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            if (!di->NextEntryOffset)
                break;
            di = (PFILE_FULL_DIR_INFORMATION)((ULONG_PTR)di + di->NextEntryOffset);
        }
        break;
    }
    case FileIdBothDirectoryInformation: {
        PFILE_ID_BOTH_DIR_INFORMATION di = infoBuffer;
        while ((ULONG_PTR)(di + 1) <= (ULONG_PTR)infoBuffer + Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length) {
            if (di->FileNameLength >= sizeof(WCHAR) && di->FileName[0] == L'.')
                di->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            if (!di->NextEntryOffset)
                break;
            di = (PFILE_ID_BOTH_DIR_INFORMATION)((ULONG_PTR)di + di->NextEntryOffset);
        }
        break;
    }
    case FileIdFullDirectoryInformation: {
        PFILE_ID_FULL_DIR_INFORMATION di = infoBuffer;
        while ((ULONG_PTR)(di + 1) <= (ULONG_PTR)infoBuffer + Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length) {
            if (di->FileNameLength >= sizeof(WCHAR) && di->FileName[0] == L'.')
                di->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            if (!di->NextEntryOffset)
                break;
            di = (PFILE_ID_FULL_DIR_INFORMATION)((ULONG_PTR)di + di->NextEntryOffset);
        }
        break;
    }
    default:
        break;
    }

done_context:
    FltReleaseContext(instance);
    return FLT_POSTOP_FINISHED_PROCESSING;
}
