#include "instance.h"

DECLARE_GLOBAL_CONST_UNICODE_STRING(g_DfP9DeviceName, L"\\Device\\P9Rdr");

NTSTATUS
FLTAPI
DfInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
{
    NTSTATUS status;
    PDF_INSTANCE_CONTEXT instance = NULL;
    BOOLEAN writable;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);

    status = FltAllocateContext(
        FltObjects->Filter,
        FLT_INSTANCE_CONTEXT,
        sizeof(DF_INSTANCE_CONTEXT),
        NonPagedPoolNx,
        &instance);
    if (!NT_SUCCESS(status))
        return status;

    DbgPrint("DfHider attaching %#lx fs type %d\n", VolumeDeviceType, VolumeFilesystemType);

    status = FltIsVolumeWritable(FltObjects->Volume, &writable);
    if (!NT_SUCCESS(status))
        goto exit;
    instance->IsLocal = VolumeDeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM;
    instance->IsMup = VolumeFilesystemType == FLT_FSTYPE_MUP;
    if (instance->IsMup) {
        status = FsRtlMupGetProviderIdFromName(&g_DfP9DeviceName, &instance->P9ProviderId);
        if (!NT_SUCCESS(status)) {
            DbgPrint("cannot find P9Rdr: %#lx\n", status);
            status = STATUS_FLT_DO_NOT_ATTACH;
            goto exit;
        }
        //DbgPrint("P9Rdr is %#x\n", instance->P9ProviderId);
    }
    else if (!instance->IsLocal || !writable) {
        status = STATUS_FLT_DO_NOT_ATTACH;
        goto exit;
    }
    status = FltSetInstanceContext(
        FltObjects->Instance,
        FLT_SET_CONTEXT_KEEP_IF_EXISTS,
        instance,
        NULL);
    if (!NT_SUCCESS(status)) {
        DbgPrint("cannot attach %#lx\n", status);
        status = STATUS_FLT_DO_NOT_ATTACH;
    }

exit:
    FltReleaseContext(instance);
    return status;
}

NTSTATUS
FLTAPI
DfQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    return STATUS_SUCCESS;
}
