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
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Editor/EditorEngine.h"

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

	// Editor-only: listen to asset editor open/close to spawn/respawn DebugActors for montages
	if (GEditor)
	{
		if (UAssetEditorSubsystem* AES = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AES->OnAssetEditorOpened().AddRaw(this, &FDBSEditorModule::HandleAssetEditorOpened);
			AES->OnAssetClosedInEditor().AddRaw(this, &FDBSEditorModule::HandleAssetEditorClosed);
		}
	}

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

                    UDamageBehaviorsSystemSettings* S = GetMutableDefault<UDamageBehaviorsSystemSettings>();
					// If not forced, follow the focused asset editor's preview mesh (AnimMontage/AnimSequence/AnimBlueprint/Skeleton)
					if (!S->ForcePreviewMeshForDebugUI.IsValid())
					{
						if (USkeletalMesh* FocusedMesh = FDBSEditorPreviewDrawer::Get()->GetFocusedEditorPreviewMesh())
						{
							S->CurrentPreviewMeshForDebugUI = FocusedMesh;
						}
					}
                    MenuBuilder.AddMenuEntry(
                        FText::FromString("Draw Debug"),
                        FText::FromString("Toggle drawing DBS capsules in preview"),
                        FSlateIcon(),
                        FUIAction(
                            FExecuteAction::CreateLambda([S, ToggleBoolSetting]() mutable { ToggleBoolSetting(S->bEnableEditorDebugDraw); }),
                            FCanExecuteAction(),
                            FIsActionChecked::CreateLambda([S]() { return S->bEnableEditorDebugDraw; })
                        ),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );

                    MenuBuilder.AddMenuEntry(
                        FText::FromString("Preserve Last Draw"),
                        FText::FromString("Keep last draw when montage is paused"),
                        FSlateIcon(),
                        FUIAction(
                            FExecuteAction::CreateLambda([S, ToggleBoolSetting]() mutable { ToggleBoolSetting(S->bPreserveDrawOnPause); }),
                            FCanExecuteAction(),
                            FIsActionChecked::CreateLambda([S]() { return S->bPreserveDrawOnPause; })
                        ),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );

                    // Show HitRegistrators shapes (default OFF)
                    MenuBuilder.AddMenuEntry(
                        FText::FromString("Show HitRegistrators shapes"),
                        FText::FromString("Toggle visibility of UCapsuleHitRegistrator shapes on spawned DebugActors"),
                        FSlateIcon(),
                        FUIAction(
                            FExecuteAction::CreateLambda([S]() mutable 
                            {
                                S->bHideDebugActorHitRegistratorShapes = !S->bHideDebugActorHitRegistratorShapes;
                                S->SaveConfig();
                                FDBSEditorPreviewDrawer::Get()->UpdateHitRegistratorShapesVisibilityForAll();
                            }),
                            FCanExecuteAction(),
                            FIsActionChecked::CreateLambda([S]() { return !S->bHideDebugActorHitRegistratorShapes; })
                        ),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );

                    // Do NOT auto-write forced mesh here; only set when user chooses explicitly

                    // Force Debug Mesh selector (if multiple previews)
                    {
                        TArray<USkeletalMesh*> ActiveMeshes; FDBSEditorPreviewDrawer::Get()->GetActivePreviewMeshes(ActiveMeshes);
                        const bool bForced = S->ForcePreviewMeshForDebugUI.IsValid();
                        const USkeletalMesh* ForcedMesh = bForced ? S->ForcePreviewMeshForDebugUI.Get() : nullptr;
                        const FString MeshLabel = bForced && ForcedMesh ? ForcedMesh->GetName() : (S->CurrentPreviewMeshForDebugUI.IsValid() ? S->CurrentPreviewMeshForDebugUI.Get()->GetName() + TEXT(" (Auto)") : FString(TEXT("<auto>")));
                        TSharedRef<SHorizontalBox> MeshRow = SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0,0).VAlign(VAlign_Center)
                        [ SNew(STextBlock).Text(FText::FromString(TEXT("Current Mesh:"))) ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(8,0).VAlign(VAlign_Center)
                        [ SNew(STextBlock).Text(FText::FromString(MeshLabel)) ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(12,0).VAlign(VAlign_Center)
                        [
                            SNew(SComboButton)
                            .ButtonContent()[ SNew(STextBlock).Text(FText::FromString(TEXT("Choose"))) ]
                            .OnGetMenuContent_Lambda([ActiveMeshes, S]() -> TSharedRef<SWidget>
                            {
                                FMenuBuilder MeshMenu(true, nullptr);
                                // Auto option clears the force
                                MeshMenu.AddMenuEntry(
                                    FText::FromString(TEXT("Auto (follow active tab)")),
                                    FText::GetEmpty(), FSlateIcon(),
                                    FUIAction(FExecuteAction::CreateLambda([S]()
                                    {
                                        S->ForcePreviewMeshForDebugUI = nullptr;
                                        FSlateApplication::Get().DismissAllMenus();
                                    }))
                                );
                                MeshMenu.AddMenuSeparator();
                                for (USkeletalMesh* M : ActiveMeshes)
                                {
                                    if (!M) continue;
                                    const FText Label = FText::FromString(M->GetName());
                                    MeshMenu.AddMenuEntry(Label, FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([S, M]()
                                    {
                                        S->ForcePreviewMeshForDebugUI = M;
                                        FSlateApplication::Get().DismissAllMenus();
                                    })));
                                }
                                return MeshMenu.MakeWidget();
                            })
                        ];
                        MenuBuilder.AddWidget(MeshRow, FText::GetEmpty(), false);
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
                    // Resolve target mesh: forced mesh or current auto selection
					USkeletalMesh* TargetMesh = S->ForcePreviewMeshForDebugUI.IsValid() ? S->ForcePreviewMeshForDebugUI.Get() : (S->CurrentPreviewMeshForDebugUI.IsValid() ? S->CurrentPreviewMeshForDebugUI.Get() : nullptr);
                    if (!TargetMesh)
                    {
						// Prefer focused asset editor's preview mesh if available
						if (USkeletalMesh* FocusedMesh = FDBSEditorPreviewDrawer::Get()->GetFocusedEditorPreviewMesh())
						{
							TargetMesh = FocusedMesh;
						}
					}
					if (!TargetMesh)
					{
                        TArray<USkeletalMesh*> ActiveMeshes; FDBSEditorPreviewDrawer::Get()->GetActivePreviewMeshes(ActiveMeshes);
                        if (ActiveMeshes.Num() > 0) { TargetMesh = ActiveMeshes[0]; }
                        if (!TargetMesh)
                        {
                            FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                            TArray<FAssetData> Selection; CBModule.Get().GetSelectedAssets(Selection);
                            for (const FAssetData& A : Selection)
                            {
                                if (USkeletalMesh* SM = Cast<USkeletalMesh>(A.GetAsset())) { TargetMesh = SM; break; }
                            }
                        }
                    }
                	// Lock current auto-selected mesh at menu open (only if not forced)
					if (TargetMesh && !S->ForcePreviewMeshForDebugUI.IsValid())
					{
					   S->CurrentPreviewMeshForDebugUI = TargetMesh;
					}
                   
                    // Ensure current is initialized lazily from defaults
                    if (TargetMesh && !S->CurrentDebugActorsForPreview.FindByKey(TargetMesh))
                    {
                        const FDBSDebugActorsForMesh* Def = S->DefaultDebugActorsForPreview.FindByKey(TargetMesh);
                        if (Def) { S->CurrentDebugActorsForPreview.Add(*Def); }
                        else { FDBSDebugActorsForMesh Tmp; Tmp.Mesh = TargetMesh; S->CurrentDebugActorsForPreview.Add(Tmp); }
                    }
                    const FDBSDebugActorsForMesh* MappingView = TargetMesh ? S->CurrentDebugActorsForPreview.FindByKey(TargetMesh) : nullptr;
                    for (const FString& Source : SourceNames)
                    {
                        const FDBSDebugActor* Found = nullptr;
                        if (MappingView)
                        {
                            Found = MappingView->DebugActors.FindByPredicate([&Source](const FDBSDebugActor& E){ return E.SourceName == Source; });
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
                                    + SHorizontalBox::Slot().AutoWidth().Padding(0,0).VAlign(VAlign_Center)
                                    [ SNew(STextBlock).Text(FText::FromString(TEXT("Class:"))) ]
                                    + SHorizontalBox::Slot().AutoWidth().Padding(8,0).VAlign(VAlign_Center)
                                    [ SNew(STextBlock).Text(FText::FromString(ClassLabel)) ]
                                    + SHorizontalBox::Slot().AutoWidth().Padding(12,0).VAlign(VAlign_Center)
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
                                                FDBSDebugActorsForMesh* Mapping = SettingsLocal->CurrentDebugActorsForPreview.FindByKey(TargetMesh);
                                                if (!Mapping) { const int32 NewIdx = SettingsLocal->CurrentDebugActorsForPreview.AddDefaulted(); Mapping = &SettingsLocal->CurrentDebugActorsForPreview[NewIdx]; Mapping->Mesh = TargetMesh; }
                                                FDBSDebugActor* Existing = Mapping->DebugActors.FindByPredicate([&Source](const FDBSDebugActor& E){ return E.SourceName == Source; });
                                                if (!Existing) { FDBSDebugActor NewEntry; NewEntry.SourceName = Source; NewEntry.Actor = BP->GeneratedClass; Mapping->DebugActors.Add(NewEntry); }
                                                else { Existing->Actor = BP->GeneratedClass; }
                                                SettingsLocal->SaveConfig();
                                                FSlateApplication::Get().DismissAllMenus();

                                                // Immediate apply: respawn just this source if enabled
                                                const FDBSDebugActor* Now = Mapping->DebugActors.FindByPredicate([&Source](const FDBSDebugActor& E){ return E.SourceName == Source; });
                                                if (Now && Now->bSpawnInPreview)
                                                {
                                                    FDBSPreviewDebugActorSpawnInfo Info; Info.SourceName = Now->SourceName; Info.Actor = Now->Actor; Info.bCustomSocketName = Now->bCustomSocketName; Info.SocketName = Now->SocketName;
                                                    TArray<FDBSPreviewDebugActorSpawnInfo> Infos; Infos.Add(Info);
                                                    FDBSEditorPreviewDrawer::Get()->ApplySpawnForMesh(TargetMesh, Infos);
                                                }
                                                else
                                                {
                                                    FDBSEditorPreviewDrawer::Get()->RemoveSpawnForMeshSource(TargetMesh, Source);
                                                }
                                            });
                                            return CBModule.Get().CreateAssetPicker(Picker);
                                        })
                                    ]
                                ]
                                // Row: Socket
                                + SVerticalBox::Slot().AutoHeight()
                                [
                                    SNew(SHorizontalBox)
                                    + SHorizontalBox::Slot().AutoWidth().Padding(0,0).VAlign(VAlign_Center)
                                    [ SNew(STextBlock).Text(FText::FromString(TEXT("Socket:"))) ]
                                    + SHorizontalBox::Slot().AutoWidth().Padding(8,0).VAlign(VAlign_Center)
                                    [
                                        SNew(SEditableTextBox)
                                        .Text(FText::FromString(SocketLabel))
                                        .OnTextCommitted_Lambda([Source, TargetMesh](const FText& NewText, ETextCommit::Type)
                                        {
                                            if (!TargetMesh) return;
                                            UDamageBehaviorsSystemSettings* SettingsLocal = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                                            FDBSDebugActorsForMesh* Mapping = SettingsLocal->CurrentDebugActorsForPreview.FindByKey(TargetMesh);
                                            if (!Mapping) { const int32 NewIdx = SettingsLocal->CurrentDebugActorsForPreview.AddDefaulted(); Mapping = &SettingsLocal->CurrentDebugActorsForPreview[NewIdx]; Mapping->Mesh = TargetMesh; }
                                            FDBSDebugActor* Existing = Mapping->DebugActors.FindByPredicate([&Source](const FDBSDebugActor& E){ return E.SourceName == Source; });
                                            if (!Existing) { FDBSDebugActor NewEntry; NewEntry.SourceName = Source; Mapping->DebugActors.Add(NewEntry); Existing = &Mapping->DebugActors.Last(); }
                                            const FString Str = NewText.ToString();
                                            Existing->bCustomSocketName = !Str.IsEmpty();
                                            Existing->SocketName = FName(Str);
                                            SettingsLocal->SaveConfig();

                                            // Immediate re-attach or respawn if enabled
                                            if (Existing->bSpawnInPreview)
                                            {
                                                FDBSPreviewDebugActorSpawnInfo Info; Info.SourceName = Existing->SourceName; Info.Actor = Existing->Actor; Info.bCustomSocketName = Existing->bCustomSocketName; Info.SocketName = Existing->SocketName;
                                                TArray<FDBSPreviewDebugActorSpawnInfo> Infos; Infos.Add(Info);
                                                FDBSEditorPreviewDrawer::Get()->ApplySpawnForMesh(TargetMesh, Infos);
                                            }
                                        })
                                    ]
                                ]
                                // Row: Spawn In Preview
                                + SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)
                                [
                                    SNew(SHorizontalBox)
                                    + SHorizontalBox::Slot().AutoWidth().Padding(0,0).VAlign(VAlign_Center)
                                    [ SNew(STextBlock).Text(FText::FromString(TEXT("Spawn In Preview:"))) ]
                                    + SHorizontalBox::Slot().AutoWidth().Padding(8,0).VAlign(VAlign_Center)
                                    [
                                        SNew(SCheckBox)
                                        .IsChecked_Lambda([MappingView, Source]()
                                        {
                                            if (!MappingView) return ECheckBoxState::Unchecked;
                                            const FDBSDebugActor* E = MappingView->DebugActors.FindByPredicate([&Source](const FDBSDebugActor& X){ return X.SourceName == Source; });
                                            return (E && E->bSpawnInPreview) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                                        })
                                        .OnCheckStateChanged_Lambda([TargetMesh, Source](ECheckBoxState NewState)
                                        {
                                            if (!TargetMesh) return;
                                            UDamageBehaviorsSystemSettings* SettingsLocal = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                                            FDBSDebugActorsForMesh* Mapping = SettingsLocal->CurrentDebugActorsForPreview.FindByKey(TargetMesh);
                                            if (!Mapping) { const int32 NewIdx = SettingsLocal->CurrentDebugActorsForPreview.AddDefaulted(); Mapping = &SettingsLocal->CurrentDebugActorsForPreview[NewIdx]; Mapping->Mesh = TargetMesh; }
                                            FDBSDebugActor* Existing = Mapping->DebugActors.FindByPredicate([&Source](const FDBSDebugActor& X){ return X.SourceName == Source; });
                                            if (!Existing) { FDBSDebugActor NewEntry; NewEntry.SourceName = Source; Mapping->DebugActors.Add(NewEntry); Existing = &Mapping->DebugActors.Last(); }
                                            Existing->bSpawnInPreview = (NewState == ECheckBoxState::Checked);
                                            SettingsLocal->SaveConfig();

                                            // Immediate apply: spawn or remove
                                            if (Existing->bSpawnInPreview)
                                            {
                                                FDBSPreviewDebugActorSpawnInfo Info; Info.SourceName = Existing->SourceName; Info.Actor = Existing->Actor; Info.bCustomSocketName = Existing->bCustomSocketName; Info.SocketName = Existing->SocketName;
                                                TArray<FDBSPreviewDebugActorSpawnInfo> Infos; Infos.Add(Info);
                                                FDBSEditorPreviewDrawer::Get()->ApplySpawnForMesh(TargetMesh, Infos);
                                            }
                                            else
                                            {
                                                FDBSEditorPreviewDrawer::Get()->RemoveSpawnForMeshSource(TargetMesh, Source);
                                            }
                                        })
                                    ]
                                ]
                            ]
                        ];
                    }
                    MenuBuilder.AddWidget(SourcesBox, FText::GetEmpty(), false);
                    MenuBuilder.EndSection();

                    // Actions section
                    MenuBuilder.BeginSection(NAME_None, FText::FromString(TEXT("Actions")));
                    // Removed explicit Apply; changes are now applied immediately

                    MenuBuilder.AddMenuEntry(
                        FText::FromString(TEXT("Save as Defaults")),
                        FText::FromString(TEXT("Copy current mesh mapping to defaults")),
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([]
                        {
                            UDamageBehaviorsSystemSettings* SettingsLocal = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                            // Resolve mesh robustly: forced, current auto (focused), CB selection, any active
                            USkeletalMesh* Mesh = SettingsLocal->ForcePreviewMeshForDebugUI.IsValid() ? SettingsLocal->ForcePreviewMeshForDebugUI.Get() : (SettingsLocal->CurrentPreviewMeshForDebugUI.IsValid() ? SettingsLocal->CurrentPreviewMeshForDebugUI.Get() : nullptr);
                            if (!Mesh) { Mesh = FDBSEditorPreviewDrawer::Get()->GetFocusedEditorPreviewMesh(); }
                            if (!Mesh)
                            {
                                FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                                TArray<FAssetData> Selection; CBModule.Get().GetSelectedAssets(Selection);
                                for (const FAssetData& A : Selection)
                                {
                                    if (USkeletalMesh* SM = Cast<USkeletalMesh>(A.GetAsset())) { Mesh = SM; break; }
                                }
                            }
                            if (!Mesh) Mesh = FDBSEditorPreviewDrawer::Get()->GetAnyActiveMesh();
                            if (!Mesh) return;

                            const FDBSDebugActorsForMesh* CurrPtr = SettingsLocal->CurrentDebugActorsForPreview.FindByKey(Mesh);
                            if (!CurrPtr) return;
                            FDBSDebugActorsForMesh Curr = *CurrPtr; // copy to ensure Mesh is set
                            Curr.Mesh = Mesh;

                            FDBSDebugActorsForMesh* Def = SettingsLocal->DefaultDebugActorsForPreview.FindByKey(Mesh);
                            if (Def) { *Def = Curr; }
                            else { SettingsLocal->DefaultDebugActorsForPreview.Add(Curr); }
                            SettingsLocal->SaveConfig();
                        }))
                    );

                    MenuBuilder.AddMenuEntry(
                        FText::FromString(TEXT("Reset to Default")),
                        FText::FromString(TEXT("Reset current mesh mapping to defaults and apply")),
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([]
                        {
                            UDamageBehaviorsSystemSettings* SettingsLocal = GetMutableDefault<UDamageBehaviorsSystemSettings>();
                            // Resolve target mesh: forced, then current auto (focused), then CB selection, finally any active
                            USkeletalMesh* Mesh = SettingsLocal->ForcePreviewMeshForDebugUI.IsValid() ? SettingsLocal->ForcePreviewMeshForDebugUI.Get() : (SettingsLocal->CurrentPreviewMeshForDebugUI.IsValid() ? SettingsLocal->CurrentPreviewMeshForDebugUI.Get() : nullptr);
                            if (!Mesh) { Mesh = FDBSEditorPreviewDrawer::Get()->GetFocusedEditorPreviewMesh(); }
                            if (!Mesh)
                            {
                                FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                                TArray<FAssetData> Selection; CBModule.Get().GetSelectedAssets(Selection);
                                for (const FAssetData& A : Selection)
                                {
                                    if (USkeletalMesh* SM = Cast<USkeletalMesh>(A.GetAsset())) { Mesh = SM; break; }
                                }
                            }
                            if (!Mesh) Mesh = FDBSEditorPreviewDrawer::Get()->GetAnyActiveMesh();
                            if (!Mesh) return;

                            const FDBSDebugActorsForMesh* Def = SettingsLocal->DefaultDebugActorsForPreview.FindByKey(Mesh);
                            FDBSDebugActorsForMesh* Curr = SettingsLocal->CurrentDebugActorsForPreview.FindByKey(Mesh);
                            if (Def)
                            {
                                if (Curr) { *Curr = *Def; }
                                else { SettingsLocal->CurrentDebugActorsForPreview.Add(*Def); }
                            }
                            else
                            {
                                if (Curr) { Curr->DebugActors.Reset(); Curr->Mesh = Mesh; }
                                else { FDBSDebugActorsForMesh Tmp; Tmp.Mesh = Mesh; SettingsLocal->CurrentDebugActorsForPreview.Add(Tmp); }
                            }

                            // Apply immediately: remove disabled, respawn enabled sources
                            const FDBSDebugActorsForMesh* Mapping = SettingsLocal->CurrentDebugActorsForPreview.FindByKey(Mesh);
                            TArray<FDBSPreviewDebugActorSpawnInfo> Infos;
                            if (Mapping)
                            {
                                for (const FDBSDebugActor& A : Mapping->DebugActors)
                                {
                                    if (A.SourceName == DEFAULT_DAMAGE_BEHAVIOR_SOURCE) continue;
                                    if (!A.bSpawnInPreview)
                                    {
                                        FDBSEditorPreviewDrawer::Get()->RemoveSpawnForMeshSource(Mesh, A.SourceName);
                                        continue;
                                    }
                                    FDBSPreviewDebugActorSpawnInfo Info; Info.SourceName = A.SourceName; Info.Actor = A.Actor; Info.bCustomSocketName = A.bCustomSocketName; Info.SocketName = A.SocketName; Infos.Add(Info);
                                }
                            }
                            if (Infos.Num() > 0)
                            {
                                FDBSEditorPreviewDrawer::Get()->ApplySpawnForMesh(Mesh, Infos);
                            }
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
                            // Route directly to the correct category (matches UDamageBehaviorsSystemSettings::GetCategoryName)
                            const FName CategoryName = FName(FApp::GetProjectName());
                            Settings.ShowViewer("Project", CategoryName, TEXT("DamageBehaviorsSystemSettings"));
                        }))
                    );
                    MenuBuilder.EndSection();

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
	if (GEditor)
	{
		if (UAssetEditorSubsystem* AES = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AES->OnAssetEditorOpened().RemoveAll(this);
			AES->OnAssetClosedInEditor().RemoveAll(this);
		}
	}
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

void FDBSEditorModule::HandleAssetEditorOpened(UObject* Asset)
{
	UAnimMontage* Montage = Cast<UAnimMontage>(Asset);
	if (!Montage)
	{
		if (UAnimSequenceBase* Seq = Cast<UAnimSequenceBase>(Asset))
		{
			if (USkeleton* Skel = Seq->GetSkeleton())
			{
				SpawnDebugActorsForMesh(Skel->GetPreviewMesh());
			}
		}
		return;
	}
	if (USkeleton* Skel = Montage->GetSkeleton())
	{
		SpawnDebugActorsForMesh(Skel->GetPreviewMesh());
	}
}

void FDBSEditorModule::HandleAssetEditorClosed(UObject* Asset, IAssetEditorInstance* AEI)
{
	UAnimMontage* Montage = Cast<UAnimMontage>(Asset);
	USkeletalMesh* Mesh = nullptr;
	if (Montage)
	{
		if (USkeleton* Skel = Montage->GetSkeleton())
		{
			Mesh = Skel->GetPreviewMesh();
		}
	}
	else if (UAnimSequenceBase* Seq = Cast<UAnimSequenceBase>(Asset))
	{
		if (USkeleton* Skel = Seq->GetSkeleton())
		{
			Mesh = Skel->GetPreviewMesh();
		}
	}
	if (!Mesh) return;
	if (FDBSEditorPreviewDrawer* Drawer = FDBSEditorPreviewDrawer::Get())
	{
		// Clear all spawned actors for this mesh
		USkeletalMeshComponent* Comp = Drawer->FindPreviewMeshCompFor(Mesh);
		if (Comp)
		{
			// Use internal clear helper via bridge: remove all sources by reading current mapping
			UDamageBehaviorsSystemSettings* SettingsLocal = GetMutableDefault<UDamageBehaviorsSystemSettings>();
			const FDBSDebugActorsForMesh* Mapping = SettingsLocal->CurrentDebugActorsForPreview.FindByKey(Mesh);
			if (Mapping)
			{
				for (const FDBSDebugActor& A : Mapping->DebugActors)
				{
					FDBSEditorPreviewDrawer::Get()->RemoveSpawnForMeshSource(Mesh, A.SourceName);
				}
			}
		}
	}
}

void FDBSEditorModule::SpawnDebugActorsForMesh(USkeletalMesh* Mesh)
{
	if (!Mesh) return;
	UDamageBehaviorsSystemSettings* S = GetMutableDefault<UDamageBehaviorsSystemSettings>();
	// Ensure current is initialized from defaults
	if (!S->CurrentDebugActorsForPreview.FindByKey(Mesh))
	{
		if (const FDBSDebugActorsForMesh* Def = S->DefaultDebugActorsForPreview.FindByKey(Mesh))
		{
			S->CurrentDebugActorsForPreview.Add(*Def);
		}
		else
		{
			FDBSDebugActorsForMesh Tmp; Tmp.Mesh = Mesh; S->CurrentDebugActorsForPreview.Add(Tmp);
		}
	}
	const FDBSDebugActorsForMesh* Mapping = S->CurrentDebugActorsForPreview.FindByKey(Mesh);
	TArray<FDBSPreviewDebugActorSpawnInfo> Infos;
	if (Mapping)
	{
		for (const FDBSDebugActor& A : Mapping->DebugActors)
		{
			if (A.SourceName == DEFAULT_DAMAGE_BEHAVIOR_SOURCE) continue;
			if (!A.bSpawnInPreview) continue;
			FDBSPreviewDebugActorSpawnInfo Info; Info.SourceName = A.SourceName; Info.Actor = A.Actor; Info.bCustomSocketName = A.bCustomSocketName; Info.SocketName = A.SocketName; Infos.Add(Info);
		}
	}
	if (Infos.Num() > 0)
	{
		FDBSEditorPreviewDrawer::Get()->ApplySpawnForMesh(Mesh, Infos);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDBSEditorModule, DBSEditor)
