// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_InvokeDamageBehavior.h"

#include "DamageBehaviorsComponent.h"
#include "DamageBehaviorsSystemSettings.h"
#include "Engine/SCS_Node.h"

UANS_InvokeDamageBehavior::UANS_InvokeDamageBehavior()
{
	TargetSources = {
		{DEFAULT_DAMAGE_BEHAVIOR_SOURCE, true }
	};

	const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
	for (TSubclassOf<UDamageBehaviorsSourceEvaluator> DamageBehaviorsSourcesEvaluator : DamageBehaviorsSystemSettings->DamageBehaviorsSourcesEvaluators)
	{
		TargetSources.Add(DamageBehaviorsSourcesEvaluator.GetDefaultObject()->SourceName, false);
	}
}

void UANS_InvokeDamageBehavior::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp->GetWorld()->IsPreviewWorld())
	{
		const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
		const FDBSInvokeDamageBehaviorDebugForMesh* InvokeDamageBehaviorDebugForMeshSearch = DamageBehaviorsSystemSettings->DebugActors.FindByKey(MeshComp->GetSkeletalMeshAsset());
		if (!InvokeDamageBehaviorDebugForMeshSearch) return;
		
		const FDBSInvokeDamageBehaviorDebugForMesh& InvokeDamageBehaviorDebugForMesh = *InvokeDamageBehaviorDebugForMeshSearch;
		auto DebugActors = InvokeDamageBehaviorDebugForMesh.DebugActors;
		if (DebugActors.IsEmpty()) return;
		
		TArray<FString> DamageBehaviorsSourcesList = GetDamageBehaviorSourcesList();
		for (const FDBSInvokeDamageBehaviorDebugActor& DebugActor : DebugActors)
		{
			// 1) Load the class (Synchronous; in async you'd use StreamableManager)
			UClass* CharClass = DebugActor.Actor.LoadSynchronous();
			if (!CharClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to load Character class!"));
				continue;
			}
			
			// 2) Get the CDO for that class
			AActor* CDO = CharClass->GetDefaultObject<AActor>();
			if (!CDO)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to get CDO for Character class!"));
				continue;
			}
			
			// 3) Gather components off the CDO
			TArray<UCapsuleHitRegistrator*> HitRegistrators;
			TMap<UCapsuleHitRegistrator*, FName> HitRegistratorsToAttachSocketsList = {};
			CDO->GetComponents<UCapsuleHitRegistrator>(HitRegistrators, true);
			
			// collect blueprint created components also
			if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(CharClass))
			{
				USimpleConstructionScript* SCS = BPGC->SimpleConstructionScript;
				if (SCS)
				{
					for (USCS_Node* Node : SCS->GetAllNodes())
					{
						if (UActorComponent* TemplateComp = Node->ComponentTemplate)
						{
							UE_LOG(LogTemp, Log, TEXT("BP-added component: %s (%s)"),
								   *TemplateComp->GetName(),
								   *TemplateComp->GetClass()->GetName());
							if (TemplateComp->GetClass()->IsChildOf(UCapsuleHitRegistrator::StaticClass()))
							{
								UCapsuleHitRegistrator* HitReg = Cast<UCapsuleHitRegistrator>(TemplateComp); 
								HitRegistrators.Add(HitReg);
								HitRegistratorsToAttachSocketsList.Add(HitReg, Node->AttachToName);
								UE_LOG(LogTemp, Log, TEXT("Added socket name: %s at index %d"), *Node->AttachToName.ToString(), HitRegistratorsToAttachSocketsList.Num() - 1);
							}
						}
					}
				}
			}

			UDamageBehaviorsComponent* DBC = CDO->GetComponentByClass<UDamageBehaviorsComponent>();
			if (!DBC) continue;

			UDamageBehavior* DamageBehavior = DBC->GetDamageBehavior(Name);
			if (!DamageBehavior) continue;

			for (const FDBSHitRegistratorsToActivateSource& HitRegistratorsToActivateBySource : DamageBehavior->HitRegistratorsToActivateBySource)
			{
				// if (HitRegistratorsToActivateBySource.SourceName != DebugActor.SourceName) continue;
		
				for (FString HitRegistratorsName : HitRegistratorsToActivateBySource.HitRegistratorsNames)
				{
					for (int32 i = 0; i < HitRegistrators.Num(); i++)
					{
						UCapsuleHitRegistrator* HitReg = HitRegistrators[i];
						FString ActorCompName = HitReg->GetName();
						ActorCompName.RemoveFromEnd(TEXT("_GEN_VARIABLE")); // for blueprint created components
						if (ActorCompName == HitRegistratorsName)
						{
							UE_LOG(LogTemp, Log, TEXT("Found matching hit registrator '%s' at index %d. Socket list size: %d"), *ActorCompName, i, HitRegistratorsToAttachSocketsList.Num());
							FDBSDebugHitRegistratorDescription HitRegistratorDescription = {};
							FName SocketNameFromBPNode = HitRegistratorsToAttachSocketsList.FindRef(HitReg);
							HitRegistratorDescription.SocketNameAttached = DebugActor.bCustomSocketName
								? DebugActor.SocketName
								: SocketNameFromBPNode;
							HitRegistratorDescription.Location = HitReg->GetRelativeLocation();
							HitRegistratorDescription.Rotation = HitReg->GetRelativeRotation();
							HitRegistratorDescription.CapsuleRadius = HitReg->GetScaledCapsuleRadius();
							HitRegistratorDescription.CapsuleHalfHeight = HitReg->GetScaledCapsuleHalfHeight();
							HitRegistratorsDescription.Add(DebugActor.SourceName, HitRegistratorDescription);
						}
					}
				}
			}
		}
		
		DrawCapsules(MeshComp->GetWorld(), MeshComp);
	}
	else
	{
		UActorComponent* Component = MeshComp->GetOwner()->GetComponentByClass(UDamageBehaviorsComponent::StaticClass());
		if (!Component) return;
	
		UDamageBehaviorsComponent* DmgBehaviorComponent = StaticCast<UDamageBehaviorsComponent*>(Component);
		if (!DmgBehaviorComponent) return;
		
		DmgBehaviorComponent->InvokeDamageBehavior(Name, true, GetDamageBehaviorSourcesList(), Payload);
	}
}

void UANS_InvokeDamageBehavior::NotifyTick(
	USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	DrawCapsules(MeshComp->GetWorld(), MeshComp);
}

void UANS_InvokeDamageBehavior::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp->GetWorld()->IsPreviewWorld())
	{
		DrawCapsules(MeshComp->GetWorld(), MeshComp);
		HitRegistratorsDescription= {};
	}
	else
	{
		UActorComponent* Component = MeshComp->GetOwner()->GetComponentByClass(UDamageBehaviorsComponent::StaticClass());
		if (!Component) return;
	
		UDamageBehaviorsComponent* DmgBehaviorComponent = StaticCast<UDamageBehaviorsComponent*>(Component);
		if (!DmgBehaviorComponent) return;
	
		DmgBehaviorComponent->InvokeDamageBehavior(Name, false, GetDamageBehaviorSourcesList(), Payload);	
	}
}

TArray<FString> UANS_InvokeDamageBehavior::GetDamageBehaviorSourcesList() const
{
	TArray<FString> Result = {};
	for (TPair<FString, bool> DamageBehaviorSource : TargetSources)
	{
		if (DamageBehaviorSource.Value)
		{
			Result.Add(DamageBehaviorSource.Key);
		}
	}
	return Result;
}

void UANS_InvokeDamageBehavior::DrawCapsules(UWorld* WorldContextObject, USkeletalMeshComponent* MeshComp)
{
	TMap<FString, FDBSDebugHitRegistratorDescription> LocalHitRegistratorsDescription = HitRegistratorsDescription;
	for (TPair<FString, FDBSDebugHitRegistratorDescription> RegistratorsDescription : LocalHitRegistratorsDescription)
	{
		FName SocketName = RegistratorsDescription.Value.SocketNameAttached;
		FVector SocketLocation = MeshComp->GetSocketLocation(SocketName);
		
		DrawDebugCapsule(
			WorldContextObject,
			SocketLocation + RegistratorsDescription.Value.Location,
			RegistratorsDescription.Value.CapsuleHalfHeight,
			RegistratorsDescription.Value.CapsuleRadius,
			RegistratorsDescription.Value.Rotation.Quaternion(),
			FLinearColor(1.0f, 0.491021f, 0.0f).ToFColor(true),
			false,
			0,
			0,
			1
		);
	}	
}
