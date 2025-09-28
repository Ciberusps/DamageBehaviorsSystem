// Pavel Penkov 2025 All Rights Reserved.

#include "DBSPreviewDebugBridge.h"

static FDBSOnPreviewDebug GDBSOnPreviewDebug;

FDBSOnPreviewDebug& DBS_GetOnPreviewDebugDelegate()
{
	return GDBSOnPreviewDebug;
}



