// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageBehaviorsComponent.h"

#include "CapsuleHitRegistrator.h"
#include "DamageBehaviorsSystemSettings.h"
#include "Utils/UnrealHelperLibraryBPL.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DamageBehaviorsComponent)

DEFINE_LOG_CATEGORY(LogDamageBehaviorsSystem);

FDamageBehaviorsSource::FDamageBehaviorsSource(FString SourceName_In, AActor* Actor_In)
{
	Actor = Actor_In;
	SourceName = SourceName_In;
	
	if (Actor)
	{
		DamageBehaviorsComponent = Actor->FindComponentByClass<UDamageBehaviorsComponent>();
		if (!DamageBehaviorsComponent)
		{
			// TODO: validation error	
		}
	}
	else
	{
		// TODO: validation error
	}
}

AActor* UAdditionalDamageBehaviorsSourceEvaluator::GetActorWithCapsules_Implementation() const
{
	return nullptr;
}

void UDamageBehaviorsComponent::BeginPlay()
{
	Super::BeginPlay();

    DamageBehaviors = TMap<FString, UDamageBehavior*>();
    DamageBehaviors.Empty();

    OwnerActor = GetOwningActor();

	// fill DamageBehaviorsSources
	DamageBehaviorsSources.Add(FDamageBehaviorsSource(DEFAULT_DAMAGE_BEHAVIOR_SOURCE, OwnerActor)); 

	TArray<FCapsuleHitRegistratorsSource> CapsuleHitRegistratorsSources = {};

	// find capsules on owner actor and add them to Sources
	FCapsuleHitRegistratorsSource OwnerCapsulesSource = {};
	OwnerCapsulesSource.Actor = GetOwner();
	// TODO: don't need to find TMap only capsules is enough
	FindCapsuleHitRegistrators(OwnerCapsulesSource.Actor).GenerateValueArray(OwnerCapsulesSource.CapsuleHitRegistrators);
	CapsuleHitRegistratorsSources.Add(OwnerCapsulesSource);

	const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
	
	// find capsules on additional Sources
	for (const TSubclassOf<UAdditionalDamageBehaviorsSourceEvaluator> AdditionalCapsulesSourceEvaluator : DamageBehaviorsSystemSettings->AdditionalDamageBehaviorsSourcesEvaluators)
	{
		if (AdditionalCapsulesSourceEvaluator)
		{
			UAdditionalDamageBehaviorsSourceEvaluator* Evaluator = NewObject<UAdditionalDamageBehaviorsSourceEvaluator>(OwnerActor, AdditionalCapsulesSourceEvaluator->StaticClass());
			if (Evaluator)
			{
				// fill capsules
				FCapsuleHitRegistratorsSource CapsulesSource = {};
				CapsulesSource.Actor = Evaluator->GetActorWithCapsules();
				// TODO: don't need to find TMap only capsules is enough
				FindCapsuleHitRegistrators(CapsulesSource.Actor).GenerateValueArray(CapsulesSource.CapsuleHitRegistrators);
				CapsuleHitRegistratorsSources.Add(CapsulesSource);

				// create DamageBehaviorsSource 
				DamageBehaviorsSources.Add(FDamageBehaviorsSource(Evaluator->SourceName, Evaluator->GetActorWithCapsules())); 
			}	
		}
	}

	// TODO перенести это в DamageBehavior

	for (UDamageBehavior* DamageBehaviorInstanced : DamageBehaviorsInstancedTest)
	{
		// UDamageBehavior* NewDamageBehavior = NewObject<UDamageBehavior>(GetOwningActor());
		DamageBehaviorInstanced->Init(
			GetOwningActor(),
			CapsuleHitRegistratorsSources
		);
		DamageBehaviors.Add(DamageBehaviorInstanced->Name, DamageBehaviorInstanced);

		// if (DamageBehaviorInstanced->DamageBehaviorDescription.bAutoHandleDamage)
		// {
		// 	DamageBehaviorInstanced->OnHitRegistered.AddUniqueDynamic(this, &UDamageBehaviorsComponent::DefaultProcessHit);
		// }

		if (DamageBehaviorInstanced->bInvokeDamageBehaviorOnStart)
		{
			InvokeDamageBehavior(DamageBehaviorInstanced->Name, true, {}, {});
		}	
	}
}

AActor* UDamageBehaviorsComponent::GetOwningActor_Implementation() const
{
    return GetOwner();
}

void UDamageBehaviorsComponent::InvokeDamageBehavior(
	FString DamageBehaviorName,
	bool bShouldActivate,
	const TArray<FString>& DamageBehaviorsSourcesToUse,
	const TArray<FInstancedStruct>& Payload
)
{
	TArray<FString> ResultDamageBehaviorsSourcesToUse = DamageBehaviorsSourcesToUse;
	if (DamageBehaviorsSourcesToUse.Num() == 0)
	{
		ResultDamageBehaviorsSourcesToUse = {
			DEFAULT_DAMAGE_BEHAVIOR_SOURCE,
			// DBS_Source_RightHandWeapon
		};
	}
	
    bool bIsDebugEnabled = false;
// #if ENABLE_DRAW_DEBUG
//     UDebugSubsystem* DebugSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>();
//     bIsDebugEnabled = DebugSubsystem->IsCategoryEnabled(EDebugCategory::HitBoxes);
// #endif

	for (FString BehaviorsSourcesToUse : ResultDamageBehaviorsSourcesToUse)
	{
		FDamageBehaviorsSource* DamageBehaviorsSource = DamageBehaviorsSources.FindByKey(BehaviorsSourcesToUse);
		if (DamageBehaviorsSource && DamageBehaviorsSource->DamageBehaviorsComponent)
		{
			if (DamageBehaviorsSource->SourceName == DEFAULT_DAMAGE_BEHAVIOR_SOURCE)
			{
				// TODO: if not exists throw error
				UDamageBehavior** DamageBehaviorSearch = DamageBehaviors.Find(DamageBehaviorName);
				UDamageBehavior* DamageBehavior = nullptr;
				if (DamageBehaviorSearch != nullptr)
				{
					DamageBehavior = *DamageBehaviorSearch;
				}
				else
				{
					if (bIsDebugEnabled)
					{
						UE_LOG(LogDamageBehaviorsSystem, Warning, TEXT("UDamageBehaviorsComponent::InvokeDamageBehavior() DamageBehavior \"%s\" not found on character looking for weapon"), *DamageBehaviorName);
					}
				}
				if (DamageBehavior)
				{
					DamageBehavior->MakeActive(bShouldActivate, Payload);
				}
			}
			else
			{
				DamageBehaviorsSource->DamageBehaviorsComponent->InvokeDamageBehavior(
					DamageBehaviorName, bShouldActivate,
					{ DEFAULT_DAMAGE_BEHAVIOR_SOURCE },
					Payload);	
			}
		}
	}
}

TArray<FString> UDamageBehaviorsComponent::GetHitRegistratorsNameOptions() const
{
    TArray<FString> Result = {};
    UObject* Outer = GetOuter();
    Result = UUnrealHelperLibraryBPL::GetNamesOfComponentsOnObject(Outer, UCapsuleHitRegistrator::StaticClass());
    return Result;
}

UDamageBehavior* UDamageBehaviorsComponent::GetDamageBehavior(FString Name) const
{
    return DamageBehaviors[Name];
}

// TODO: попытаться вернуть, но хз зач все это можно в DamageBehavior'ах ловить
// void UDamageBehaviorsComponent::DefaultOnHitAnything(
// 	const UDamageBehavior* DamageBehavior,
// 	const FCapsuleHitRegistratorHitResult& GetHitResult, const bool bSpawnFX)
// {
// 	if (bSpawnFX)
// 	{
// 		UBUtilsBPL::SpawnDefaultImpactVFX(
// 			GetHitResult.GodreaperHitResult,
// 			GetHitResult.GodreaperHitResult.Instigator.Get(),
// 			DamageBehavior->DamageBehaviorDescription.ImpactCueTag,
// 			GetHitResult.GodreaperHitResult.AttackType);
// 	}
// 	
//     if (GetHitResult.bHittableActorGotHit && OnHitAnything.IsBound())
//     {
//         OnHitAnything.Broadcast(DamageBehavior, DamageBehavior->Name, GetHitResult.GodreaperHitResult);
//     }
//     if (GetHitResult.GodreaperHitResult.bHitCharacter && OnHitCharacter.IsBound())
//     {
//         OnHitCharacter.Broadcast(DamageBehavior, DamageBehavior->Name, GetHitResult.GodreaperHitResult);
//     }
// }

TMap<FString, UCapsuleHitRegistrator*> UDamageBehaviorsComponent::FindCapsuleHitRegistrators(AActor* Actor) const
{
	TMap<FString, UCapsuleHitRegistrator*> Result;

	TArray<UActorComponent*> CapsuleHitRegistratorsActorComponents;
	Actor->GetComponents(UCapsuleHitRegistrator::StaticClass(), CapsuleHitRegistratorsActorComponents, true);

	for (UActorComponent* ActorComponent : CapsuleHitRegistratorsActorComponents)
	{
		UCapsuleHitRegistrator* CapsuleHitRegistrator = StaticCast<UCapsuleHitRegistrator*>(ActorComponent);
		Result.Add(CapsuleHitRegistrator->GetName(), CapsuleHitRegistrator);
	}

	return Result;
}
