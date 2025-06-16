// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_InvokeDamageBehavior.h"

#include "DamageBehaviorsComponent.h"
#include "DamageBehaviorsSystemSettings.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"

void FDBSInvokeDamageBehaviorDebugActor::FillData()
{
	// 1) Load the class (Synchronous; in async you'd use StreamableManager)
	UClass* CharClass = Actor.LoadSynchronous();
	if (!CharClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load Character class!"));
		return;
	}
			
	// 2) Get the CDO for that class
	ActorCDO = CharClass->GetDefaultObject<AActor>();
	if (!ActorCDO.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get CDO for Character class!"));
		return;
	}
			
	// 3) Gather components by traversing the class hierarchy
	TMap<FString, TTuple<UCapsuleHitRegistrator*, FName>> ComponentNameToInfo;
	
	// First, try to get components from the current blueprint class
	if (UBlueprintGeneratedClass* CurrentBPGC = Cast<UBlueprintGeneratedClass>(CharClass))
	{
		UE_LOG(LogTemp, Log, TEXT("Processing current class: %s"), *CharClass->GetName());
		
		// First check inherited components that might have been overridden
		if (UInheritableComponentHandler* InheritableComponentHandler = CurrentBPGC->GetInheritableComponentHandler())
		{
			TArray<UActorComponent*> Templates;
			InheritableComponentHandler->GetAllTemplates(Templates);
			for (UActorComponent* Template : Templates)
			{
				if (UCapsuleHitRegistrator* HitReg = Cast<UCapsuleHitRegistrator>(Template))
				{
					FString CompName = HitReg->GetName();
					UE_LOG(LogTemp, Log, TEXT("Found overridden hit registrator in current class: %s"), *CompName);
					
					// Try to find the original component's socket name from parent classes
					FName SocketName2 = NAME_None;
					UClass* ParentClass = CharClass->GetSuperClass();
					while (ParentClass && ParentClass != AActor::StaticClass())
					{
						if (UBlueprintGeneratedClass* ParentBPGC = Cast<UBlueprintGeneratedClass>(ParentClass))
						{
							if (USimpleConstructionScript* SCS = ParentBPGC->SimpleConstructionScript)
							{
								for (USCS_Node* Node : SCS->GetAllNodes())
								{
									if (Node->ComponentTemplate && Node->ComponentTemplate->GetName() == CompName)
									{
										SocketName2 = Node->AttachToName;
										break;
									}
								}
							}
						}
						if (!SocketName2.IsNone()) break;
						ParentClass = ParentClass->GetSuperClass();
					}
					
					ComponentNameToInfo.Add(CompName, MakeTuple(HitReg, SocketName2));
				}
			}
		}

		// Then check components added in this blueprint
		if (USimpleConstructionScript* SCS = CurrentBPGC->SimpleConstructionScript)
		{
			for (USCS_Node* Node : SCS->GetAllNodes())
			{
				if (UActorComponent* Component = Node->ComponentTemplate)
				{
					if (UCapsuleHitRegistrator* HitReg = Cast<UCapsuleHitRegistrator>(Component))
					{
						FString CompName = HitReg->GetName();
						if (!ComponentNameToInfo.Contains(CompName))
						{
							UE_LOG(LogTemp, Log, TEXT("Found hit registrator in current class: %s with socket: %s"),
								*CompName,
								*Node->AttachToName.ToString());
								
							ComponentNameToInfo.Add(CompName, MakeTuple(HitReg, Node->AttachToName));
						}
					}
				}
			}
		}
	}
	
	// If we didn't find any components in the current class, look in parent classes
	if (ComponentNameToInfo.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("No components found in current class, checking parent classes"));
		
		UClass* CurrentClass = CharClass->GetSuperClass();
		while (CurrentClass && CurrentClass != AActor::StaticClass())
		{
			UE_LOG(LogTemp, Log, TEXT("Processing parent class: %s"), *CurrentClass->GetName());
			
			if (UBlueprintGeneratedClass* CurrentBPGC = Cast<UBlueprintGeneratedClass>(CurrentClass))
			{
				if (USimpleConstructionScript* SCS = CurrentBPGC->SimpleConstructionScript)
				{
					for (USCS_Node* Node : SCS->GetAllNodes())
					{
						if (UActorComponent* Component = Node->ComponentTemplate)
						{
							if (UCapsuleHitRegistrator* HitReg = Cast<UCapsuleHitRegistrator>(Component))
							{
								FString CompName = HitReg->GetName();
								if (!ComponentNameToInfo.Contains(CompName))
								{
									UE_LOG(LogTemp, Log, TEXT("Found hit registrator in parent class %s: %s with socket: %s"),
										*CurrentClass->GetName(),
										*CompName,
										*Node->AttachToName.ToString());
										
									ComponentNameToInfo.Add(CompName, MakeTuple(HitReg, Node->AttachToName));
								}
							}
						}
					}
				}
			}
			
			CurrentClass = CurrentClass->GetSuperClass();
		}
	}
	
	// Now add all the components we found
	for (const auto& ComponentInfo : ComponentNameToInfo)
	{
		UCapsuleHitRegistrator* HitReg = ComponentInfo.Value.Get<0>();
		FName SocketName2 = ComponentInfo.Value.Get<1>();
		HitRegistrators.Add(HitReg);
		HitRegistratorsToAttachSocketsList.Add(HitReg, SocketName2);
	}
	
	// Add any native components from CDO
	TArray<UActorComponent*> NativeComponents;
	ActorCDO->GetComponents(UActorComponent::StaticClass(), NativeComponents);
	for (UActorComponent* Component : NativeComponents)
	{
		if (UCapsuleHitRegistrator* HitReg = Cast<UCapsuleHitRegistrator>(Component))
		{
			FString CompName = HitReg->GetName();
			if (!ComponentNameToInfo.Contains(CompName))
			{
				UE_LOG(LogTemp, Log, TEXT("Found native hit registrator: %s"), *CompName);
				HitRegistrators.Add(HitReg);
				HitRegistratorsToAttachSocketsList.Add(HitReg, NAME_None);
			}
		}
	}

	DBC = ActorCDO->GetComponentByClass<UDamageBehaviorsComponent>();
}

UANS_InvokeDamageBehavior::UANS_InvokeDamageBehavior()
{
	TargetSources = {
		{DEFAULT_DAMAGE_BEHAVIOR_SOURCE, true }
	};

	const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
	for (const TSubclassOf<UDamageBehaviorsSourceEvaluator> DamageBehaviorsSourcesEvaluator : DamageBehaviorsSystemSettings->DamageBehaviorsSourcesEvaluators)
	{
		if (!DamageBehaviorsSourcesEvaluator) continue;
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
		
		// If no specific debug actors found for this mesh, use the fallback mesh
		if (!InvokeDamageBehaviorDebugForMeshSearch)
		{
			FilledDebugActors = DamageBehaviorsSystemSettings->FallbackDebugMesh.DebugActors;
		}
		else
		{
			const FDBSInvokeDamageBehaviorDebugForMesh& InvokeDamageBehaviorDebugForMesh = *InvokeDamageBehaviorDebugForMeshSearch;
			FilledDebugActors = InvokeDamageBehaviorDebugForMesh.DebugActors;
		}
		
		if (FilledDebugActors.IsEmpty()) return;
		
		TArray<FString> DamageBehaviorsSourcesList = GetDamageBehaviorSourcesList();

		for (FDBSInvokeDamageBehaviorDebugActor& DebugActor : FilledDebugActors)
		{
			DebugActor.FillData();
		}
		
		for (FString DamageBehaviorsSource : DamageBehaviorsSourcesList)
		{
			FDBSInvokeDamageBehaviorDebugActor DebugActor = GetFilledDebugActor(DamageBehaviorsSource);
			if (!DebugActor.IsValid()) continue;

			UDamageBehavior* DamageBehavior = DebugActor.DBC->GetDamageBehavior(Name);
			if (!DamageBehavior) continue;

			for (const FDBSHitRegistratorsToActivateSource& HitRegistratorsToActivateBySource : DamageBehavior->HitRegistratorsToActivateBySource)
			{
				FDBSInvokeDamageBehaviorDebugActor DebugActorForHitRegistrator = GetFilledDebugActor(HitRegistratorsToActivateBySource.SourceName);
				if (!DebugActorForHitRegistrator.IsValid()) continue;

				for (FString HitRegistratorsName : HitRegistratorsToActivateBySource.HitRegistratorsNames)
				{
					for (int32 i = 0; i < DebugActorForHitRegistrator.HitRegistrators.Num(); i++)
					{
						UCapsuleHitRegistrator* HitReg = DebugActorForHitRegistrator.HitRegistrators[i].IsValid()
							? DebugActorForHitRegistrator.HitRegistrators[i].Get()
							: nullptr;
						if (!HitReg) continue;

						FString ActorCompName = HitReg->GetName();
						ActorCompName.RemoveFromEnd(TEXT("_GEN_VARIABLE")); // for blueprint created components
						if (ActorCompName == HitRegistratorsName)
						{
							UE_LOG(LogTemp, Log, TEXT("Found matching hit registrator '%s' at index %d. Socket list size: %d"), *ActorCompName, i, DebugActorForHitRegistrator.HitRegistratorsToAttachSocketsList.Num());
							FDBSDebugHitRegistratorDescription HitRegistratorDescription = {};
							FName SocketNameFromBPNode = DebugActorForHitRegistrator.HitRegistratorsToAttachSocketsList.FindRef(HitReg);
							HitRegistratorDescription.SocketNameAttached = DebugActorForHitRegistrator.bCustomSocketName
								? DebugActorForHitRegistrator.SocketName
								: SocketNameFromBPNode;
							HitRegistratorDescription.Location = HitReg->GetRelativeLocation();
							HitRegistratorDescription.Rotation = HitReg->GetRelativeRotation();
							HitRegistratorDescription.CapsuleRadius = HitReg->GetScaledCapsuleRadius();
							HitRegistratorDescription.CapsuleHalfHeight = HitReg->GetScaledCapsuleHalfHeight();
							HitRegistratorDescription.Color = HitReg->ShapeColor;
							HitRegistratorDescription.Thickness = HitReg->GetLineThickness();
							HitRegistratorsDescription.Add(DebugActorForHitRegistrator.SourceName, HitRegistratorDescription);
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
	if (MeshComp->GetWorld()->IsPreviewWorld())
	{
		DrawCapsules(MeshComp->GetWorld(), MeshComp);
	}
}

void UANS_InvokeDamageBehavior::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp->GetWorld()->IsPreviewWorld())
	{
		DrawCapsules(MeshComp->GetWorld(), MeshComp);
		FilledDebugActors = {};
		HitRegistratorsDescription = {};
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

FDBSInvokeDamageBehaviorDebugActor UANS_InvokeDamageBehavior::GetFilledDebugActor(FString SourceName)
{
	FDBSInvokeDamageBehaviorDebugActor* DebugActorSearch = FilledDebugActors.FindByPredicate([&](const FDBSInvokeDamageBehaviorDebugActor& DebugActor)
		{
			return DebugActor.SourceName == SourceName;
		});
	if (!DebugActorSearch) return {};

	FDBSInvokeDamageBehaviorDebugActor& DebugActor = *DebugActorSearch;
	return DebugActor;
}

void UANS_InvokeDamageBehavior::DrawCapsules(UWorld* WorldContextObject, USkeletalMeshComponent* MeshComp)
{
	TMap<FString, FDBSDebugHitRegistratorDescription> LocalHitRegistratorsDescription = HitRegistratorsDescription;
	const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
	const bool bUsingFallback = !DamageBehaviorsSystemSettings->DebugActors.FindByKey(MeshComp->GetSkeletalMeshAsset());

	for (TPair<FString, FDBSDebugHitRegistratorDescription> RegistratorsDescription : LocalHitRegistratorsDescription)
	{
		FName SocketName = RegistratorsDescription.Value.SocketNameAttached;
		FVector SocketLocation = MeshComp->GetSocketLocation(SocketName);
		FRotator SocketRotation = MeshComp->GetSocketRotation(SocketName);
		
		// First rotate by socket rotation, then apply the component's local rotation
		FRotator FinalRotation = SocketRotation + RegistratorsDescription.Value.Rotation;
		FVector FinalLocation = SocketLocation + FinalRotation.RotateVector(RegistratorsDescription.Value.Location);
		
		// Draw each frame without persistence to handle pauses correctly
		DrawDebugCapsule(
			WorldContextObject,
			FinalLocation,
			RegistratorsDescription.Value.CapsuleHalfHeight,
			RegistratorsDescription.Value.CapsuleRadius,
			FinalRotation.Quaternion(),
			RegistratorsDescription.Value.Color.ToFColor(true),
			false,  // bPersistentLines - redraw each tick instead
			-1.0f,  // LifeTime - will be cleared next frame
			0,      // DepthPriority
			RegistratorsDescription.Value.Thickness  // Use the component's shape thickness
		);

		if (bUsingFallback)
		{
			// Draw a "F" letter above the capsule to indicate fallback
			const float TextHeight = 20.0f;
			const float TextWidth = 10.0f;
			const FVector TextBase = FinalLocation + FVector(0, 0, RegistratorsDescription.Value.CapsuleHalfHeight + 20.0f);
			
			// Vertical line of F
			DrawDebugLine(
				WorldContextObject,
				TextBase,
				TextBase + FVector(0, 0, TextHeight),
				FColor::Yellow,
				false,  // bPersistentLines
				-1.0f,  // LifeTime
				0,      // DepthPriority
				2.0f    // Thickness
			);
			
			// Top horizontal line of F
			DrawDebugLine(
				WorldContextObject,
				TextBase + FVector(0, 0, TextHeight),
				TextBase + FVector(TextWidth, 0, TextHeight),
				FColor::Yellow,
				false,
				-1.0f,
				0,
				2.0f
			);
			
			// Middle horizontal line of F
			DrawDebugLine(
				WorldContextObject,
				TextBase + FVector(0, 0, TextHeight * 0.6f),
				TextBase + FVector(TextWidth * 0.8f, 0, TextHeight * 0.6f),
				FColor::Yellow,
				false,
				-1.0f,
				0,
				2.0f
			);
		}
	}	
}
