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
        DfPreQueryInformation,
        DfPostQueryInformation
    },
    {
        IRP_MJ_DIRECTORY_CONTROL,
        0,
        DfPreDirectoryControl,
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
    .InstanceQueryTeardownCallback = DfInstanceQueryTeardown,
    .InstanceTeardownStartCallback = NULL,
    .InstanceTeardownCompleteCallback = NULL,

    .GenerateFileNameCallback = NULL,
    .NormalizeNameComponentCallback = NULL,
    .NormalizeContextCleanupCallback = NULL
};

_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    status = FltRegisterFilter(
        DriverObject,
        &FilterRegistration,
        &g_DfHiderData.FilterHandle);
    if (!NT_SUCCESS(status))
        goto fail_register;

    status = FltStartFiltering(g_DfHiderData.FilterHandle);
    if (!NT_SUCCESS(status))
        goto fail_start;

fail_start:
    FltUnregisterFilter(g_DfHiderData.FilterHandle);

fail_register:
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
