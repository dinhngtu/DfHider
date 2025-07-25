#pragma once

#include "stdafx.h"

FLT_PREOP_CALLBACK_STATUS
FLTAPI
DfPreQueryInformation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext
);

// Post callbacks may be called at DISPATCH_LEVEL. Specify _IRQL_requires_ to help the analyzer.

_IRQL_requires_(DISPATCH_LEVEL)
FLT_POSTOP_CALLBACK_STATUS
FLTAPI
DfPostQueryInformation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FLTAPI
DfPreDirectoryControl(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext
);

_IRQL_requires_(DISPATCH_LEVEL)
FLT_POSTOP_CALLBACK_STATUS
FLTAPI
DfPostDirectoryControl(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
);
