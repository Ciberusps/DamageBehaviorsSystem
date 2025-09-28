// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditorPreviewDrawer.h"

#include "DBSPreviewDebugBridge.h"
#include "DamageBehaviorsSystemSettings.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UObjectIterator.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
// Forward declare without including unavailable headers in some engine builds
class IAssetEditorInstance;
#endif

FDBSEditorPreviewDrawer::FDBSEditorPreviewDrawer()
{
	DBS_GetOnPreviewDebugDelegate().AddRaw(this, &FDBSEditorPreviewDrawer::OnPreviewDebug);
}

FDBSEditorPreviewDrawer::~FDBSEditorPreviewDrawer()
{
	DBS_GetOnPreviewDebugDelegate().RemoveAll(this);
}

void FDBSEditorPreviewDrawer::OnPreviewDebug(const FDBSPreviewDebugPayload& Payload)
{
	if (!Payload.MeshComp.IsValid()) return;

	UDamageBehaviorsSystemSettings* Settings = GetMutableDefault<UDamageBehaviorsSystemSettings>();
	if (!Settings->bEnableEditorDebugDraw) return;

    switch (Payload.Event)
	{
	case EDBSPreviewDebugEvent::Begin:
		bActiveForMesh.Add(Payload.MeshComp, true);
		LastDescriptions.Add(Payload.MeshComp, Payload.HitRegistratorsDescription);

		// Despawn previous and spawn new DebugActors from payload
		if (USkeletalMeshComponent* Comp = Payload.MeshComp.Get())
		{
			ClearSpawnForMeshComp(Comp);
			if (USkeletalMesh* Mesh = Comp->GetSkeletalMeshAsset())
			{
				if (Payload.DebugActorsToSpawn.Num() > 0)
				{
					ApplySpawnForMesh(Mesh, Payload.DebugActorsToSpawn);
				}
			}
		}
		break;
	case EDBSPreviewDebugEvent::Tick:
		LastDescriptions.FindOrAdd(Payload.MeshComp) = Payload.HitRegistratorsDescription;
		break;
	case EDBSPreviewDebugEvent::End:
		bActiveForMesh.Add(Payload.MeshComp, false);
		if (!Settings->bPreserveDrawOnPause)
		{
			LastDescriptions.Remove(Payload.MeshComp);
		}

		// Despawn on End to avoid stale actors between montages
		if (USkeletalMeshComponent* Comp = Payload.MeshComp.Get())
		{
			ClearSpawnForMeshComp(Comp);
		}
		break;
	}
}

void FDBSEditorPreviewDrawer::Tick(float DeltaTime)
{
	UDamageBehaviorsSystemSettings* Settings = GetMutableDefault<UDamageBehaviorsSystemSettings>();
	if (!Settings->bEnableEditorDebugDraw) return;

	// Auto-spawn per-source defaults for any preview skeletal mesh
	for (TObjectIterator<USkeletalMeshComponent> It; It; ++It)
	{
		USkeletalMeshComponent* Comp = *It;
		if (!Comp || !Comp->GetWorld() || !Comp->GetWorld()->IsPreviewWorld()) continue;
		USkeletalMesh* Mesh = Comp->GetSkeletalMeshAsset();
		if (!Mesh) continue;

		// Subscribe once per anim instance to detect montage changes and clear spawned actors
		if (UAnimInstance* AnimInst = Comp->GetAnimInstance())
		{
			UAnimMontage* Current = AnimInst->GetCurrentActiveMontage();
			UAnimMontage* Last = LastSeenMontageByComp.FindRef(Comp).Get();
			if (Current != Last)
			{
				// Montage changed: clear existing debug actors; Begin payload will repopulate
				ClearSpawnForMeshComp(Comp);
				LastSeenMontageByComp.Add(Comp, Current);
			}
		}

		UDamageBehaviorsSystemSettings* S = GetMutableDefault<UDamageBehaviorsSystemSettings>();
		// Ensure current is initialized
		if (Mesh && !S->CurrentDebugActorsForPreview.FindByKey(Mesh))
		{
			const FDBSDebugActorsForMesh* Def = S->DefaultDebugActorsForPreview.FindByKey(Mesh);
			if (Def) { S->CurrentDebugActorsForPreview.Add(*Def); }
			else { FDBSDebugActorsForMesh Tmp; Tmp.Mesh = Mesh; S->CurrentDebugActorsForPreview.Add(Tmp); }
		}
		const FDBSDebugActorsForMesh* Mapping = S->CurrentDebugActorsForPreview.FindByKey(Mesh);
		if (!Mapping) continue;
		// Skip auto-spawn during active preview session to avoid duplicates
		if (bActiveForMesh.FindRef(Comp)) { continue; }
		TArray<FDBSPreviewDebugActorSpawnInfo> Infos;
		for (const FDBSDebugActor& A : Mapping->DebugActors)
		{
			if (A.SourceName == DEFAULT_DAMAGE_BEHAVIOR_SOURCE) continue;
			if (!A.bSpawnInPreview) continue;
			FDBSPreviewDebugActorSpawnInfo Info; Info.SourceName = A.SourceName; Info.Actor = A.Actor; Info.bCustomSocketName = A.bCustomSocketName; Info.SocketName = A.SocketName; Infos.Add(Info);
		}
		if (Infos.Num() > 0)
		{
			ApplySpawnForMesh(Mesh, Infos);
		}
	}

	for (auto& Pair : LastDescriptions)
	{
		USkeletalMeshComponent* MeshComp = Pair.Key.Get();
		if (!MeshComp) continue;
		if (!MeshComp->GetWorld() || !MeshComp->GetWorld()->IsPreviewWorld()) continue;

		for (auto& DescPair : Pair.Value)
		{
			const FDBSDebugHitRegistratorDescription& Desc = DescPair.Value;
			const FName SocketName = Desc.SocketNameAttached;
			const FVector SocketLocation = MeshComp->GetSocketLocation(SocketName);
			const FRotator SocketRotation = MeshComp->GetSocketRotation(SocketName);

			const FRotator FinalRotation = SocketRotation + Desc.Rotation;
			const FVector FinalLocation = SocketLocation + FinalRotation.RotateVector(Desc.Location);

			DrawDebugCapsule(
				MeshComp->GetWorld(),
				FinalLocation,
				Desc.CapsuleHalfHeight,
				Desc.CapsuleRadius,
				FinalRotation.Quaternion(),
				Desc.Color.ToFColor(true),
				false,
				-1.0f,
				0,
				Desc.Thickness
			);
		}
	}
}

TStatId FDBSEditorPreviewDrawer::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FDBSEditorPreviewDrawer, STATGROUP_Tickables);
}

FDBSEditorPreviewDrawer* FDBSEditorPreviewDrawer::Get()
{
	static FDBSEditorPreviewDrawer* Singleton = nullptr;
	if (!Singleton)
	{
		Singleton = new FDBSEditorPreviewDrawer();
	}
	return Singleton;
}

USkeletalMeshComponent* FDBSEditorPreviewDrawer::FindPreviewMeshCompFor(USkeletalMesh* Mesh) const
{
	for (const auto& Pair : LastDescriptions)
	{
		if (USkeletalMeshComponent* Comp = Pair.Key.Get())
		{
			if (Comp->GetSkeletalMeshAsset() == Mesh)
			{
				return Comp;
			}
		}
	}
	return nullptr;
}

USkeletalMesh* FDBSEditorPreviewDrawer::GetAnyActiveMesh() const
{
	for (const auto& Pair : LastDescriptions)
	{
		if (USkeletalMeshComponent* Comp = Pair.Key.Get())
		{
			if (USkeletalMesh* Mesh = Comp->GetSkeletalMeshAsset())
			{
				return Mesh;
			}
		}
	}
	return nullptr;
}

USkeletalMesh* FDBSEditorPreviewDrawer::GetFocusedEditorPreviewMesh() const
{
#if WITH_EDITOR
	if (!GEditor) return nullptr;
	UAssetEditorSubsystem* AES = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AES) return nullptr;

	TArray<UObject*> EditedAssets = AES->GetAllEditedAssets();
	if (EditedAssets.Num() == 0) return nullptr;

	TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();

	// Prefer the asset whose editor owns the active top-level window
	for (int32 Index = EditedAssets.Num() - 1; Index >= 0; --Index)
	{
		UObject* Asset = EditedAssets[Index];
		if (!Asset) continue;
		IAssetEditorInstance* Instance = AES->FindEditorForAsset(Asset, false);
		FAssetEditorToolkit* Toolkit = static_cast<FAssetEditorToolkit*>(Instance);
		if (!Toolkit) continue;
		const TSharedRef<IToolkitHost> Host = Toolkit->GetToolkitHost();
		TSharedPtr<SWindow> ThisWindow = FSlateApplication::Get().FindWidgetWindow(Host->GetParentWidget());
		if (ThisWindow.IsValid() && ThisWindow == ActiveWindow)
		{
			// Derive mesh from the focused asset
			if (const UAnimSequenceBase* Anim = Cast<UAnimSequenceBase>(Asset))
			{
				if (USkeleton* Skel = Anim->GetSkeleton())
				{
					if (USkeletalMesh* PreviewMesh = Skel->GetPreviewMesh())
					{
						return PreviewMesh;
					}
				}
			}
			if (const UAnimMontage* Montage = Cast<UAnimMontage>(Asset))
			{
				if (USkeleton* Skel = Montage->GetSkeleton())
				{
					if (USkeletalMesh* PreviewMesh = Skel->GetPreviewMesh())
					{
						return PreviewMesh;
					}
				}
			}
			if (const UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(Asset))
			{
				if (USkeleton* Skel = AnimBP->TargetSkeleton)
				{
					if (USkeletalMesh* PreviewMesh = Skel->GetPreviewMesh())
					{
						return PreviewMesh;
					}
				}
			}
			if (const USkeleton* Skel = Cast<USkeleton>(Asset))
			{
				if (USkeletalMesh* PreviewMesh = const_cast<USkeleton*>(Skel)->GetPreviewMesh())
				{
					return PreviewMesh;
				}
			}
			if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Asset))
			{
				return Mesh;
			}
		}
	}

	// Fallback: scan edited assets in reverse order for a resolvable mesh
	for (int32 Index = EditedAssets.Num() - 1; Index >= 0; --Index)
	{
		UObject* Asset = EditedAssets[Index];
		if (!Asset) continue;
		if (const UAnimSequenceBase* Anim = Cast<UAnimSequenceBase>(Asset))
		{
			if (USkeleton* Skel = Anim->GetSkeleton())
			{
				if (USkeletalMesh* PreviewMesh = Skel->GetPreviewMesh())
				{
					return PreviewMesh;
				}
			}
		}
		if (const UAnimMontage* Montage = Cast<UAnimMontage>(Asset))
		{
			if (USkeleton* Skel = Montage->GetSkeleton())
			{
				if (USkeletalMesh* PreviewMesh = Skel->GetPreviewMesh())
				{
					return PreviewMesh;
				}
			}
		}
		if (const UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(Asset))
		{
			if (USkeleton* Skel = AnimBP->TargetSkeleton)
			{
				if (USkeletalMesh* PreviewMesh = Skel->GetPreviewMesh())
				{
					return PreviewMesh;
				}
			}
		}
		if (const USkeleton* Skel = Cast<USkeleton>(Asset))
		{
			if (USkeletalMesh* PreviewMesh = const_cast<USkeleton*>(Skel)->GetPreviewMesh())
			{
				return PreviewMesh;
			}
		}
		if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Asset))
		{
			return Mesh;
		}
	}
#endif
	return nullptr;
}

void FDBSEditorPreviewDrawer::GetActivePreviewMeshes(TArray<USkeletalMesh*>& OutMeshes) const
{
    OutMeshes.Reset();
    for (const auto& Pair : LastDescriptions)
    {
        if (USkeletalMeshComponent* Comp = Pair.Key.Get())
        {
            if (Comp->GetWorld() && Comp->GetWorld()->IsPreviewWorld())
            {
                if (USkeletalMesh* Mesh = Comp->GetSkeletalMeshAsset())
                {
                    OutMeshes.AddUnique(Mesh);
                }
            }
        }
    }
}

void FDBSEditorPreviewDrawer::ApplySpawnForMesh(USkeletalMesh* Mesh, const TArray<FDBSPreviewDebugActorSpawnInfo>& SpawnInfos)
{
	if (!Mesh) return;
    USkeletalMeshComponent* Comp = this->FindPreviewMeshCompFor(Mesh);
	if (!Comp) return;

	UDamageBehaviorsSystemSettings* Settings = GetMutableDefault<UDamageBehaviorsSystemSettings>();
    // No global spawn gate; respect per-source flags via caller

	UWorld* World = Comp->GetWorld();
	if (!World || !World->IsPreviewWorld()) return;

    TMap<FString, TWeakObjectPtr<AActor>>& PerSource = SpawnedActors.FindOrAdd(Comp);
	for (const FDBSPreviewDebugActorSpawnInfo& Info : SpawnInfos)
	{
		UClass* DesiredClass = Info.Actor.LoadSynchronous();
		if (!DesiredClass) continue;
		if (UClass* BaseFilter = Settings->DebugActorFilterBaseClass.LoadSynchronous())
		{
			if (!DesiredClass->IsChildOf(BaseFilter)) continue;
		}
		AActor* Existing = PerSource.FindRef(Info.SourceName).Get();
		// If previously spawned with a different class, destroy and respawn
		if (Existing && Existing->GetClass() != DesiredClass)
		{
			Existing->Destroy();
			PerSource.Remove(Info.SourceName);
			Existing = nullptr;
		}
		if (!Existing || Existing->IsActorBeingDestroyed() || !IsValid(Existing))
		{
			FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
			Existing = World->SpawnActor<AActor>(DesiredClass, FTransform::Identity, Params);
			if (!IsValid(Existing))
			{
				PerSource.Remove(Info.SourceName);
				continue;
			}
			PerSource.Add(Info.SourceName, Existing);
		}

		const FName Socket = Info.bCustomSocketName ? Info.SocketName : NAME_None;
		if (!IsValid(Comp) || Comp->IsBeingDestroyed()) { continue; }
		if (!IsValid(Existing) || Existing->IsActorBeingDestroyed()) { continue; }
		Existing->AttachToComponent(Comp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);

		// Apply HitRegistrator shapes visibility on initial spawn/respawn
		{
			const bool bHide = Settings->bHideDebugActorHitRegistratorShapes;
			TInlineComponentArray<UCapsuleHitRegistrator*> Registrators(Existing);
			for (UCapsuleHitRegistrator* Reg : Registrators)
			{
				if (!Reg) continue;
				// Prefer component visibility toggle for immediate effect
				Reg->SetVisibility(!bHide, true);
				Reg->UpdateComponentToWorld();
			}
		}
	}
}

void FDBSEditorPreviewDrawer::UpdateHitRegistratorShapesVisibilityForAll()
{
    UDamageBehaviorsSystemSettings* Settings = GetMutableDefault<UDamageBehaviorsSystemSettings>();
    const bool bHide = Settings->bHideDebugActorHitRegistratorShapes;
    for (auto& Pair : SpawnedActors)
    {
        TMap<FString, TWeakObjectPtr<AActor>>& PerSource = Pair.Value;
        for (auto& SourcePair : PerSource)
        {
            if (AActor* Actor = SourcePair.Value.Get())
            {
                TInlineComponentArray<UCapsuleHitRegistrator*> Registrators(Actor);
                for (UCapsuleHitRegistrator* Reg : Registrators)
                {
                    if (!Reg) continue;
                    if (bHide)
                    {
						Reg->SetVisibility(false);
                    }
                    else
                    {
                        // Restore a visible default if needed
						Reg->SetVisibility(true);
                    }
                    Reg->UpdateComponentToWorld();
                }
            }
        }
    }
}

void FDBSEditorPreviewDrawer::RemoveSpawnForMeshSource(USkeletalMesh* Mesh, const FString& SourceName)
{
    if (!Mesh) return;
    USkeletalMeshComponent* Comp = this->FindPreviewMeshCompFor(Mesh);
    if (!Comp) return;
    TMap<FString, TWeakObjectPtr<AActor>>* PerSourcePtr = SpawnedActors.Find(Comp);
    if (!PerSourcePtr) return;
    TWeakObjectPtr<AActor>& ActorPtr = (*PerSourcePtr).FindOrAdd(SourceName);
    if (ActorPtr.IsValid())
    {
        if (AActor* Existing = ActorPtr.Get())
        {
            Existing->Destroy();
        }
        (*PerSourcePtr).Remove(SourceName);
    }
}

void FDBSEditorPreviewDrawer::ClearSpawnForMeshComp(USkeletalMeshComponent* Comp)
{
	if (!Comp) return;
	TMap<FString, TWeakObjectPtr<AActor>>* PerSourcePtr = SpawnedActors.Find(Comp);
	if (!PerSourcePtr) return;
	for (auto& Pair : *PerSourcePtr)
	{
		if (AActor* Existing = Pair.Value.Get())
		{
			Existing->Destroy();
		}
	}
	SpawnedActors.Remove(Comp);
}



