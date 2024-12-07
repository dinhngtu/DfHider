#pragma once

#include "stdafx.h"

#define DFHIDER_FILTER_NAME L"DfHider"

typedef struct _DF_FILTER_DATA {
    PFLT_FILTER FilterHandle;
} DF_FILTER_DATA, * PDF_FILTER_DATA;

DRIVER_INITIALIZE DriverEntry;

NTSTATUS
FLTAPI
DfUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);
