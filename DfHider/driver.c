#include "stdafx.h"
#include "driver.h"
#include "instance.h"
#include "callbacks.h"

DF_FILTER_DATA g_DfHiderData;

static CONST FLT_CONTEXT_REGISTRATION ContextRegistration[] = {
    {
        FLT_INSTANCE_CONTEXT,
        0,
        NULL,
        sizeof(DF_INSTANCE_CONTEXT),
        DF_INSTANCE_CONTEXT_TAG
    },
    { FLT_CONTEXT_END }
};

static CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    {
        IRP_MJ_QUERY_INFORMATION,
        0,
        NULL,
        DfPostQueryInformation
    },
    {
        IRP_MJ_DIRECTORY_CONTROL,
        0,
        NULL,
        DfPostDirectoryControl
    },
    { IRP_MJ_OPERATION_END }
};

static CONST FLT_REGISTRATION FilterRegistration = {
    .Size = sizeof(FLT_REGISTRATION),
    .Version = FLT_REGISTRATION_VERSION,
    .Flags = 0,

    .ContextRegistration = ContextRegistration,
    .OperationRegistration = Callbacks,

    .FilterUnloadCallback = DfUnload,

    .InstanceSetupCallback = DfInstanceSetup,
    .InstanceQueryTeardownCallback = DfQueryTeardown,
    .InstanceTeardownStartCallback = NULL,
    .InstanceTeardownCompleteCallback = NULL,

    .GenerateFileNameCallback = NULL,
    .NormalizeNameComponentCallback = NULL,
    .NormalizeContextCleanupCallback = NULL
};

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    status = FltRegisterFilter(DriverObject,
        &FilterRegistration,
        &g_DfHiderData.FilterHandle);

    FLT_ASSERT(NT_SUCCESS(status));

    if (!NT_SUCCESS(status))
        return status;

    status = FltStartFiltering(g_DfHiderData.FilterHandle);
    if (!NT_SUCCESS(status))
        FltUnregisterFilter(g_DfHiderData.FilterHandle);

    return status;
}

NTSTATUS
DfUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);

    FltUnregisterFilter(g_DfHiderData.FilterHandle);

    return STATUS_SUCCESS;
}
