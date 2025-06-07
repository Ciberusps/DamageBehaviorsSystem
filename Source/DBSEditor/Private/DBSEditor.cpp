// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditor.h"

#include "DBSEditorDamageBehaviorDetails.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

static const FName UHLDebugSystemEditorTabName("DBSEditor");

#define LOCTEXT_NAMESPACE "FDBSEditorModule"

void FDBSEditorModule::StartupModule()
{
	FPropertyEditorModule& PEM =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// DamageBehavior
	PEM.RegisterCustomPropertyTypeLayout(
	"HitRegistratorsToActivateSource",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FDBSEditorDamageBehaviorDetails::MakeInstance
		)
	);

	// DamageBehaviorComponent
	// PEM.RegisterCustomClassLayout(
	// 	"DamageBehaviorsComponent",
	// 	FOnGetDetailCustomizationInstance::CreateStatic(&FDBSEditorDamageBehaviorComponentDetails::MakeInstance)
	// );
	// PEM.NotifyCustomizationModuleChanged();
}

void FDBSEditorModule::ShutdownModule()
{
	// DamageBehavior
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PEM =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PEM.UnregisterCustomPropertyTypeLayout("HitRegistratorsToActivateSource");
	}

	// DamageBehaviorComponent
	// if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	// {
	// 	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// 	PropertyModule.UnregisterCustomClassLayout("DamageBehaviorsComponent");
	// }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDBSEditorModule, DBSEditor)
