// Pavel Penkov 2025 All Rights Reserved.

#include "DBSEditorPreviewDrawer.h"

#include "DBSPreviewDebugBridge.h"
#include "DamageBehaviorsSystemSettings.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

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

		if (Settings->bSpawnDebugActorsInPreview)
		{
			UWorld* World = Payload.MeshComp->GetWorld();
			if (World && World->IsPreviewWorld())
			{
				TMap<FString, TWeakObjectPtr<AActor>>& PerSource = SpawnedActors.FindOrAdd(Payload.MeshComp);
				for (const FDBSPreviewDebugActorSpawnInfo& Info : Payload.DebugActorsToSpawn)
				{
					if (PerSource.Contains(Info.SourceName) && PerSource[Info.SourceName].IsValid())
					{
						continue; // already spawned
					}
					UClass* Cls = Info.Actor.LoadSynchronous();
					if (!Cls) continue;
					if (UClass* BaseFilter = Settings->DebugActorFilterBaseClass.LoadSynchronous())
					{
						if (!Cls->IsChildOf(BaseFilter))
						{
							continue; // does not pass filter
						}
					}
					FActorSpawnParameters Params;
					Params.ObjectFlags |= RF_Transient;
					AActor* Spawned = World->SpawnActor<AActor>(Cls, FTransform::Identity, Params);
					if (!Spawned) continue;

					PerSource.Add(Info.SourceName, Spawned);

					if (Payload.MeshComp.IsValid())
					{
						const FName Socket = Info.bCustomSocketName ? Info.SocketName : NAME_None;
						if (Settings->bAttachDebugActorsToSockets && Socket != NAME_None)
						{
							Spawned->AttachToComponent(Payload.MeshComp.Get(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
						}
						else
						{
							// Preserve socket world location if no attach
							if (Socket != NAME_None)
							{
								const FTransform SocketWorld = Payload.MeshComp->GetSocketTransform(Socket, RTS_World);
								Spawned->SetActorTransform(SocketWorld);
							}
							else
							{
								Spawned->SetActorTransform(Payload.MeshComp->GetComponentTransform());
							}
						}
					}
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

		// Destroy spawned actors for this mesh
		if (SpawnedActors.Contains(Payload.MeshComp))
		{
			for (auto& KV : SpawnedActors[Payload.MeshComp])
			{
				if (AActor* A = KV.Value.Get())
				{
					A->Destroy();
				}
			}
			SpawnedActors.Remove(Payload.MeshComp);
		}
		break;
	}
}

void FDBSEditorPreviewDrawer::Tick(float DeltaTime)
{
	UDamageBehaviorsSystemSettings* Settings = GetMutableDefault<UDamageBehaviorsSystemSettings>();
	if (!Settings->bEnableEditorDebugDraw) return;

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



