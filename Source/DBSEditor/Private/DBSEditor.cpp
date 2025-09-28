// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditor.h"

#include "DBSEditorDamageBehaviorDetails.h"
#include "DamageBehaviorsComponent.h"
#include "DBSEditorPreviewDrawer.h"
#include "DBSPreviewDebugBridge.h"
#include "Engine/SkeletalMesh.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "DamageBehaviorsSystemSettings.h"
#include "ISettingsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "DBSEditorPreviewDrawer.h"
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
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"

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

	// Ensure preview drawer singleton exists
	FDBSEditorPreviewDrawer::Get();

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

                    // Removed broken "Open Settings" entry below; working one remains later

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

                    // Build dynamic per-source editor UI
                    UDamageBehaviorsSystemSettings* S = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                    // Resolve target mesh from selection or active preview
                    USkeletalMesh* TargetMesh = nullptr;
                    {
                        FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                        TArray<FAssetData> Selection; CBModule.Get().GetSelectedAssets(Selection);
                        for (const FAssetData& A : Selection)
                        {
                            if (USkeletalMesh* SM = Cast<USkeletalMesh>(A.GetAsset())) { TargetMesh = SM; break; }
                        }
                        if (!TargetMesh) { TargetMesh = FDBSEditorPreviewDrawer::Get()->GetAnyActiveMesh(); }
                    }

                    TArray<FString> SourceNames;
                    SourceNames.Add(DEFAULT_DAMAGE_BEHAVIOR_SOURCE);
                    for (const TSubclassOf<UDamageBehaviorsSourceEvaluator> E : S->DamageBehaviorsSourcesEvaluators)
                    {
                        if (E) { SourceNames.Add(E.GetDefaultObject()->SourceName); }
                    }

                    MenuBuilder.AddMenuSeparator();
                    MenuBuilder.BeginSection(NAME_None, FText::FromString(TEXT("Per-Source DebugActors")));
                    TSharedRef<SVerticalBox> SourcesBox = SNew(SVerticalBox);
                    const FDBSInvokeDamageBehaviorDebugForMesh* MappingView = TargetMesh ? S->DebugActors.FindByKey(TargetMesh) : nullptr;
                    for (const FString& Source : SourceNames)
                    {
                        const FDBSInvokeDamageBehaviorDebugActor* Found = nullptr;
                        if (MappingView)
                        {
                            Found = MappingView->DebugActors.FindByPredicate([&Source](const FDBSInvokeDamageBehaviorDebugActor& E){ return E.SourceName == Source; });
                        }
                        const FString ClassLabel = Found && Found->Actor.IsValid() ? Found->Actor.GetAssetName() : FString(TEXT("<none>"));
                        const FString SocketLabel = Found && Found->bCustomSocketName ? Found->SocketName.ToString() : FString();

                        SourcesBox->AddSlot()
                        .AutoHeight()
                        .Padding(FMargin(6.0f, 6.0f))
                        [
                            SNew(SBorder)
                            .Padding(FMargin(8.0f))
                            [
                                SNew(SVerticalBox)
                                // Row: Source Name
                                + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,4)
                                [
                                    SNew(SHorizontalBox)
                                    + SHorizontalBox::Slot().AutoWidth().Padding(0,0)
                                    [ SNew(STextBlock).Text(FText::FromString(TEXT("Source:"))) ]
                                    + SHorizontalBox::Slot().AutoWidth().Padding(8,0)
                                    [ SNew(STextBlock).Text(FText::FromString(Source)) ]
                                ]
                                // Row: Class picker
                                + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,4)
                                [
                                    SNew(SHorizontalBox)
                                    + SHorizontalBox::Slot().AutoWidth().Padding(0,0)
                                    [ SNew(STextBlock).Text(FText::FromString(TEXT("Class:"))) ]
                                    + SHorizontalBox::Slot().AutoWidth().Padding(8,0)
                                    [ SNew(STextBlock).Text(FText::FromString(ClassLabel)) ]
                                    + SHorizontalBox::Slot().AutoWidth().Padding(12,0)
                                    [
                                        SNew(SComboButton)
                                        .ButtonContent()
                                        [ SNew(STextBlock).Text(FText::FromString(TEXT("Select Class"))) ]
                                        .OnGetMenuContent_Lambda([Source, TargetMesh]() -> TSharedRef<SWidget>
                                        {
                                            FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                                            FAssetPickerConfig Picker;
                                            Picker.Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
                                            Picker.SelectionMode = ESelectionMode::Single;
                                            Picker.bFocusSearchBoxWhenOpened = true;
                                            Picker.OnAssetSelected = FOnAssetSelected::CreateLambda([Source, TargetMesh](const FAssetData& Asset)
                                            {
                                                if (!TargetMesh) return;
                                                UBlueprint* BP = Cast<UBlueprint>(Asset.GetAsset());
                                                if (!BP || !BP->GeneratedClass) return;
                                                UDamageBehaviorsSystemSettings* SettingsLocal = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                                                FDBSInvokeDamageBehaviorDebugForMesh* Mapping = SettingsLocal->DebugActors.FindByKey(TargetMesh);
                                                if (!Mapping) { const int32 NewIdx = SettingsLocal->DebugActors.AddDefaulted(); Mapping = &SettingsLocal->DebugActors[NewIdx]; Mapping->Mesh = TargetMesh; }
                                                FDBSInvokeDamageBehaviorDebugActor* Existing = Mapping->DebugActors.FindByPredicate([&Source](const FDBSInvokeDamageBehaviorDebugActor& E){ return E.SourceName == Source; });
                                                if (!Existing) { FDBSInvokeDamageBehaviorDebugActor NewEntry; NewEntry.SourceName = Source; NewEntry.Actor = BP->GeneratedClass; Mapping->DebugActors.Add(NewEntry); }
                                                else { Existing->Actor = BP->GeneratedClass; }
                                                SettingsLocal->SaveConfig();
                                                FSlateApplication::Get().DismissAllMenus();
                                            });
                                            return CBModule.Get().CreateAssetPicker(Picker);
                                        })
                                    ]
                                ]
                                // Row: Socket
                                + SVerticalBox::Slot().AutoHeight()
                                [
                                    SNew(SHorizontalBox)
                                    + SHorizontalBox::Slot().AutoWidth().Padding(0,0)
                                    [ SNew(STextBlock).Text(FText::FromString(TEXT("Socket:"))) ]
                                    + SHorizontalBox::Slot().AutoWidth().Padding(8,0)
                                    [
                                        SNew(SEditableTextBox)
                                        .Text(FText::FromString(SocketLabel))
                                        .OnTextCommitted_Lambda([Source, TargetMesh](const FText& NewText, ETextCommit::Type)
                                        {
                                            if (!TargetMesh) return;
                                            UDamageBehaviorsSystemSettings* SettingsLocal = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                                            FDBSInvokeDamageBehaviorDebugForMesh* Mapping = SettingsLocal->DebugActors.FindByKey(TargetMesh);
                                            if (!Mapping) { const int32 NewIdx = SettingsLocal->DebugActors.AddDefaulted(); Mapping = &SettingsLocal->DebugActors[NewIdx]; Mapping->Mesh = TargetMesh; }
                                            FDBSInvokeDamageBehaviorDebugActor* Existing = Mapping->DebugActors.FindByPredicate([&Source](const FDBSInvokeDamageBehaviorDebugActor& E){ return E.SourceName == Source; });
                                            if (!Existing) { FDBSInvokeDamageBehaviorDebugActor NewEntry; NewEntry.SourceName = Source; Mapping->DebugActors.Add(NewEntry); Existing = &Mapping->DebugActors.Last(); }
                                            const FString Str = NewText.ToString();
                                            Existing->bCustomSocketName = !Str.IsEmpty();
                                            Existing->SocketName = FName(Str);
                                            SettingsLocal->SaveConfig();
                                        })
                                    ]
                                ]
                            ]
                        ];
                    }
                    MenuBuilder.AddWidget(SourcesBox, FText::GetEmpty(), false);
                    MenuBuilder.EndSection();

                    MenuBuilder.AddMenuSeparator();
                    MenuBuilder.AddMenuEntry(
                        FText::FromString(TEXT("Apply (Spawn/Attach in Preview)")),
                        FText::FromString(TEXT("Applies current per-source mapping by spawning debug actors in the preview viewport")),
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([]
                        {
                            UDamageBehaviorsSystemSettings* SettingsLocal = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                            // Determine mesh from selection or active preview
                            USkeletalMesh* Mesh = nullptr;
                            {
                                FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                                TArray<FAssetData> Selection; CBModule.Get().GetSelectedAssets(Selection);
                                for (const FAssetData& A : Selection)
                                {
                                    if (USkeletalMesh* SM = Cast<USkeletalMesh>(A.GetAsset())) { Mesh = SM; break; }
                                }
                                if (!Mesh) { Mesh = FDBSEditorPreviewDrawer::Get()->GetAnyActiveMesh(); }
                            }
                            if (!Mesh) return;

                            const FDBSInvokeDamageBehaviorDebugForMesh* Mapping = SettingsLocal->DebugActors.FindByKey(Mesh);
                            if (!Mapping)
                            {
                                Mapping = &SettingsLocal->FallbackDebugMesh;
                            }
                            TArray<FDBSPreviewDebugActorSpawnInfo> Infos;
                            if (Mapping)
                            {
                                for (const FDBSInvokeDamageBehaviorDebugActor& A : Mapping->DebugActors)
                                {
                                    FDBSPreviewDebugActorSpawnInfo Info;
                                    Info.SourceName = A.SourceName;
                                    Info.Actor = A.Actor;
                                    Info.bCustomSocketName = A.bCustomSocketName;
                                    Info.SocketName = A.SocketName;
                                    Infos.Add(Info);
                                }
                            }
                            FDBSEditorPreviewDrawer::Get()->ApplySpawnForMesh(Mesh, Infos);
                        }))
                    );

                    MenuBuilder.AddMenuEntry(
                        FText::FromString(TEXT("Save To Defaults")),
                        FText::FromString(TEXT("Ensure current mesh mapping is saved in Debug Actors")),
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([]
                        {
                            FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                            TArray<FAssetData> Selection; CBModule.Get().GetSelectedAssets(Selection);
                            USkeletalMesh* Mesh = nullptr;
                            for (const FAssetData& A : Selection)
                            {
                                if (USkeletalMesh* SM = Cast<USkeletalMesh>(A.GetAsset())) { Mesh = SM; break; }
                            }
                            if (!Mesh) Mesh = FDBSEditorPreviewDrawer::Get()->GetAnyActiveMesh();
                            if (!Mesh) return;
                            UDamageBehaviorsSystemSettings* SettingsLocal = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                            FDBSInvokeDamageBehaviorDebugForMesh* Mapping = SettingsLocal->DebugActors.FindByKey(Mesh);
                            if (!Mapping)
                            {
                                const int32 NewIdx = SettingsLocal->DebugActors.AddDefaulted();
                                Mapping = &SettingsLocal->DebugActors[NewIdx];
                                Mapping->Mesh = Mesh;
                            }
                            SettingsLocal->SaveConfig();
                        }))
                    );

                    // Working open settings
                    MenuBuilder.AddMenuEntry(
                        FText::FromString("Open Settings"),
                        FText::FromString("Open Project Settings → DamageBehaviorsSystemSettings"),
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([]
                        {
                            ISettingsModule& Settings = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
                            Settings.ShowViewer("Project", "Project", TEXT("DamageBehaviorsSystemSettings"));
                        }))
                    );

                    return MenuBuilder.MakeWidget();
                }),
                FText::FromString("DBS Debug"),
                FText::FromString("Configure DBS debug actors and options"),
                FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visibility")
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
