// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditor.h"

#include "DamageBehaviorsComponent.h"
#include "Misc/MessageDialog.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Blueprint.h"
#include "ToolMenus.h"

static const FName UHLDebugSystemEditorTabName("DBSEditor");

#define LOCTEXT_NAMESPACE "FDBSEditorModule"

void FDBSEditorModule::StartupModule()
{
	FPropertyEditorModule& PEM =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// DamageBehavior
	// PEM.RegisterCustomPropertyTypeLayout(
	// "DBSHitRegistratorsToActivateSource",
	// 	FOnGetPropertyTypeCustomizationInstance::CreateStatic(
	// 		&FDBSEditorDamageBehaviorDetails::MakeInstance
	// 	)
	// );

	// DamageBehaviorComponent
	// PEM.RegisterCustomClassLayout(
	// 	"DamageBehaviorsComponent",
	// 	FOnGetDetailCustomizationInstance::CreateStatic(&FDBSEditorDamageBehaviorComponentDetails::MakeInstance)
	// );
	// PEM.NotifyCustomizationModuleChanged();

	// Check UDamageBehaviorsComponent::DamageBehaviorsList description why this used
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FDBSEditorModule::HandleObjectPropertyChanged);
}

void FDBSEditorModule::ShutdownModule()
{
	// DamageBehavior
	// if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	// {
	// 	FPropertyEditorModule& PEM =
	// 		FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// 	PEM.UnregisterCustomPropertyTypeLayout("DBSHitRegistratorsToActivateSource");
	// }

	// DamageBehaviorComponent
	// if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	// {
	// 	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// 	PropertyModule.UnregisterCustomClassLayout("DamageBehaviorsComponent");
	// }

	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

void FDBSEditorModule::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	// Detect Blueprint CDO update
	UBlueprint* Blueprint = Cast<UBlueprint>(Object);
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return;
	}

	// Get Actor CDO and its UDamageBehaviorsComponent default
    const AActor* ActorCDO = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject());
    if (!ActorCDO)
    {
        return;
    }

    // Early return if this Blueprint's Actor CDO has no UDamageBehaviorsComponent
    if (!ActorCDO->FindComponentByClass<UDamageBehaviorsComponent>())
    {
        return;
    }


	TArray<UDamageBehaviorsComponent*> TemplateComps;
	ActorCDO->GetComponents<UDamageBehaviorsComponent>(TemplateComps);
	if (TemplateComps.Num() == 0)
	{
		return;
	}

	UDamageBehaviorsComponent* SourceComp = TemplateComps[0];

	// Copy default list into all valid instances
	for (TObjectIterator<UDamageBehaviorsComponent> It; It; ++It)
	{
		UDamageBehaviorsComponent* Comp = *It;
		if (!Comp->GetOwner() || !Comp->GetWorld())
		{
			continue;
		}

		// Skip templates, archetypes, and trash
		if (Comp->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			|| Comp->GetOwner()->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			|| Comp->GetName().StartsWith(TEXT("TRASH_"))
			|| Comp->GetOwner()->GetName().StartsWith(TEXT("TRASH_")))
		{
			continue;
		}

		Comp->Modify();
		Comp->DamageBehaviorsList = SourceComp->DamageBehaviorsList;
		// No need to signal UI here; values will persist until next editor refresh
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDBSEditorModule, DBSEditor)
