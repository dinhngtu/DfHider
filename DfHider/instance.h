#pragma once

#include "stdafx.h"

typedef struct _DF_INSTANCE_CONTEXT {
    BOOLEAN IsLocal;
    BOOLEAN IsMup;
    UINT32 P9ProviderId;
} DF_INSTANCE_CONTEXT, * PDF_INSTANCE_CONTEXT;
#define DF_INSTANCE_CONTEXT_TAG 'IDfH'

NTSTATUS
FLTAPI
DfInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

NTSTATUS
FLTAPI
DfInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);
