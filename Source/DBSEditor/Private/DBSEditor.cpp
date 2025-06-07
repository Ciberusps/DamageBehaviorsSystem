// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditor.h"

#include "DBSEditorDamageBehaviorDetails.h"
#include "DamageBehavior.h"
#include "DamageBehaviorsComponent.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

static const FName UHLDebugSystemEditorTabName("DBSEditor");

#define LOCTEXT_NAMESPACE "FDBSEditorModule"

void FDBSEditorModule::StartupModule()
{
	FPropertyEditorModule& PEM =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PEM.RegisterCustomClassLayout(
		UDamageBehaviorsComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FDBSEditorDamageBehaviorDetails::MakeInstance)
	);
}

void FDBSEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PEM =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		PEM.UnregisterCustomClassLayout(UDamageBehaviorsComponent::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDBSEditorModule, DBSEditor)
