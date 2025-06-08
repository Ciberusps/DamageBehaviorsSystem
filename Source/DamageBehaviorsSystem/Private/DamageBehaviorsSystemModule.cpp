// Pavel Penkov 2025 All Rights Reserved.

#include "DamageBehaviorsSystemModule.h"

#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FDamageBehaviorsSystemModule"

static TAutoConsoleVariable<int32> CVarDBSHitBoxes(
	TEXT("DamageBehaviorsSystem.HitBoxes"),
	0,
	TEXT("Show hitboxes"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarDBSHitBoxesHistory(
	TEXT("DamageBehaviorsSystem.HitBoxes.History"),
	0,
	TEXT("Show hitboxes history"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarDBSHitBoxesSlowMotion(
	TEXT("DamageBehaviorsSystem.HitBoxes.SlowMotion"),
	0,
	TEXT("Slowmotion on invoking damage behaviors from Player"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarDBSHitLog(
	TEXT("DamageBehaviorsSystem.HitLog"),
	0,
	TEXT("Show hits on screen"),
	ECVF_Default
);

void FDamageBehaviorsSystemModule::StartupModule()
{
}

void FDamageBehaviorsSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDamageBehaviorsSystemModule, DamageBehaviorsSystem)
