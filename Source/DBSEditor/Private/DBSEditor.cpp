// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditor.h"

#include "DBSEditorDamageBehaviorDetails.h"
#include "DamageBehaviorsComponent.h"
#include "DBSEditorPreviewDrawer.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "DamageBehaviorsSystemSettings.h"
#include "DBSEditorInvokeDebugActorDetails.h"
#include "PropertyEditorModule.h"
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
	PEM.RegisterCustomPropertyTypeLayout(
	"DBSHitRegistratorsToActivateSource",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FDBSEditorDamageBehaviorDetails::MakeInstance
		)
	);

	// DebugActor struct customization (placeholder; leverages class filter in pickers elsewhere if used)
	PEM.RegisterCustomPropertyTypeLayout(
		"DBSInvokeDamageBehaviorDebugActor",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FDBSEditorInvokeDebugActorDetails::MakeInstance
		)
	);

	// DamageBehaviorComponent
	// PEM.RegisterCustomClassLayout(
	// 	"DamageBehaviorsComponent",
	// 	FOnGetDetailCustomizationInstance::CreateStatic(&FDBSEditorDamageBehaviorComponentDetails::MakeInstance)
	// );
	// PEM.NotifyCustomizationModuleChanged();

	// Check UDamageBehaviorsComponent::DamageBehaviorsList description why this used
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FDBSEditorModule::HandleObjectPropertyChanged);

	// Register preview drawer
	static FDBSEditorPreviewDrawer GPreviewDrawer;

	// Register a toolbar extender in AnimSequence/Persona/SkeletalMesh editors via ToolMenus
    auto AddDBSToolbarToMenu = [] (const FName MenuName)
    {
        if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(MenuName))
        {
            FToolMenuSection& Section = Menu->AddSection("DBS", FText::FromString("DamageBehaviors"));
            FToolMenuEntry Entry = FToolMenuEntry::InitComboButton(
                "DBS_Debug",
                FUIAction(),
                FOnGetContent::CreateLambda([]() -> TSharedRef<SWidget>
                {
                    FMenuBuilder MenuBuilder(true, nullptr);

                    MenuBuilder.AddMenuEntry(
                        FText::FromString("Open Settings"),
                        FText::FromString("Edit default DBS debug actors"),
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([]
                        {
                            // Fallback: simply open Project Settings page name via console (if available), else no-op
                            // Developers can open Project Settings and find DamageBehaviorsSystemSettings manually
                        }))
                    );

                    // Toggles
                    auto ToggleBoolSetting = [](bool& Flag)
                    {
                        Flag = !Flag;
                        GetMutableDefault<UDamageBehaviorsSystemSettings>()->SaveConfig();
                    };

                    UDamageBehaviorsSystemSettings* Settings = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                    MenuBuilder.AddMenuEntry(
                        FText::FromString("Enable Editor Debug Draw"),
                        FText::FromString("Toggle drawing DBS capsules in preview"),
                        FSlateIcon(),
                        FUIAction(
                            FExecuteAction::CreateLambda([Settings, ToggleBoolSetting]() mutable { ToggleBoolSetting(Settings->bEnableEditorDebugDraw); }),
                            FCanExecuteAction(),
                            FIsActionChecked::CreateLambda([Settings]() { return Settings->bEnableEditorDebugDraw; })
                        ),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );

                    MenuBuilder.AddMenuEntry(
                        FText::FromString("Preserve Draw On Pause"),
                        FText::FromString("Keep last draw when montage is paused"),
                        FSlateIcon(),
                        FUIAction(
                            FExecuteAction::CreateLambda([Settings, ToggleBoolSetting]() mutable { ToggleBoolSetting(Settings->bPreserveDrawOnPause); }),
                            FCanExecuteAction(),
                            FIsActionChecked::CreateLambda([Settings]() { return Settings->bPreserveDrawOnPause; })
                        ),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );

                    MenuBuilder.AddMenuEntry(
                        FText::FromString("Spawn DebugActors In Preview"),
                        FText::FromString("Spawn debug actors in Anim preview world"),
                        FSlateIcon(),
                        FUIAction(
                            FExecuteAction::CreateLambda([Settings, ToggleBoolSetting]() mutable { ToggleBoolSetting(Settings->bSpawnDebugActorsInPreview); }),
                            FCanExecuteAction(),
                            FIsActionChecked::CreateLambda([Settings]() { return Settings->bSpawnDebugActorsInPreview; })
                        ),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );

                    MenuBuilder.AddMenuEntry(
                        FText::FromString("Attach DebugActors To Sockets"),
                        FText::FromString("Attach spawned actors to sockets if provided"),
                        FSlateIcon(),
                        FUIAction(
                            FExecuteAction::CreateLambda([Settings, ToggleBoolSetting]() mutable { ToggleBoolSetting(Settings->bAttachDebugActorsToSockets); }),
                            FCanExecuteAction(),
                            FIsActionChecked::CreateLambda([Settings]() { return Settings->bAttachDebugActorsToSockets; })
                        ),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );

                    // Minimal menu without class pickers to avoid unavailable APIs

                    return MenuBuilder.MakeWidget();
                }),
                FText::FromString("DBS Debug"),
                FText::FromString("Configure DBS debug actors and options"),
                FSlateIcon()
            );
            Section.AddEntry(Entry);
        }
    };

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([AddDBSToolbarToMenu]
    {
        AddDBSToolbarToMenu("AssetEditor.SkeletonEditor.ToolBar");
        AddDBSToolbarToMenu("AssetEditor.SkeletalMeshEditor.ToolBar");
        AddDBSToolbarToMenu("AssetEditor.AnimationEditor.ToolBar");
    }));
}

void FDBSEditorModule::ShutdownModule()
{
	// DamageBehavior
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PEM =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PEM.UnregisterCustomPropertyTypeLayout("DBSHitRegistratorsToActivateSource");
		PEM.UnregisterCustomPropertyTypeLayout("DBSInvokeDamageBehaviorDebugActor");
	}

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
