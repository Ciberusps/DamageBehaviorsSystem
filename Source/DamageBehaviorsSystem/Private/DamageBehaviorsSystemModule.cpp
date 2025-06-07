// Pavel Penkov 2025 All Rights Reserved.

#include "DamageBehaviorsSystemModule.h"

#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FDamageBehaviorsSystemModule"


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
