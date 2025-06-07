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

AActor* UAdditionalDamageBehaviorsSourceEvaluator::GetActorWithCapsules_Implementation(AActor* OwnerActor) const
{
	return nullptr;
}

void UDamageBehaviorsComponent::BeginPlay()
{
	Super::BeginPlay();

    OwnerActor = GetOwningActor();

	// fill DamageBehaviorsSources
	DamageBehaviorsSources.Add(FDamageBehaviorsSource(DEFAULT_DAMAGE_BEHAVIOR_SOURCE, OwnerActor)); 

	DamageBehaviorsSourceEvaluators = SpawnEvaluators();
	
	const TArray<FDBSHitRegistratorsSource>& CapsuleHitRegistratorsSources = GetHitRegistratorsSources(DamageBehaviorsSourceEvaluators);
	for (const FDBSHitRegistratorsSource& CapsuleHitRegistratorsSource : CapsuleHitRegistratorsSources)
	{
		// create DamageBehaviorsSource
		DamageBehaviorsSources.Add(FDamageBehaviorsSource(CapsuleHitRegistratorsSource.SourceName, CapsuleHitRegistratorsSource.Actor));
	}

	// Create a new array for the initialized behaviors
	TArray<UDamageBehavior*> InitializedBehaviors;
	InitializedBehaviors.Reserve(DamageBehaviors.Num());

	// Initialize all behaviors first
	for (UDamageBehavior* DamageBehaviorInstanced : DamageBehaviors)
	{
		UDamageBehavior* DamageBehavior = NewObject<UDamageBehavior>(this, DamageBehaviorInstanced->StaticClass());
		DamageBehavior->Init(
			GetOwningActor(),
			CapsuleHitRegistratorsSources
		);
		InitializedBehaviors.Add(DamageBehavior);
	}

	// Replace the old behaviors with the initialized ones
	DamageBehaviors = MoveTemp(InitializedBehaviors);

	// Now activate the ones that need to start active
	for (UDamageBehavior* DamageBehavior : DamageBehaviors)
	{
		if (DamageBehavior->bInvokeDamageBehaviorOnStart)
		{
			InvokeDamageBehavior(DamageBehavior->Name, true, {}, {});
		}
	}
}

AActor* UDamageBehaviorsComponent::GetOwningActor_Implementation() const
{
    return GetOwner();
}

void UDamageBehaviorsComponent::InvokeDamageBehavior(
	const FString& DamageBehaviorName,
	const bool bShouldActivate,
	const TArray<FString>& DamageBehaviorsSourcesToUse,
	const FInstancedStruct& Payload
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
				UDamageBehavior** DamageBehaviorSearch = DamageBehaviors.FindByPredicate(
				[&](const UDamageBehavior* Behavior)
				{
					return Behavior && Behavior->GetName() == DamageBehaviorName; // or whatever property you're comparing
				});;
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

UDamageBehavior* UDamageBehaviorsComponent::GetDamageBehavior(const FString& Name) const
{
	return *DamageBehaviors.FindByPredicate(
		[&](const UDamageBehavior* Behavior)
		{
			return Behavior && Behavior->GetName() == Name; // or whatever property you're comparing
		});;
}

const TArray<FDBSHitRegistratorsSource> UDamageBehaviorsComponent::GetHitRegistratorsSources(
	TArray<UAdditionalDamageBehaviorsSourceEvaluator*> SourceEvaluators_In
) const
{
	TArray<FDBSHitRegistratorsSource> Result = {};
	
	FDBSHitRegistratorsSource OwnerCapsulesSource = {};
	OwnerCapsulesSource.SourceName = DEFAULT_DAMAGE_BEHAVIOR_SOURCE;
	OwnerCapsulesSource.Actor = GetOwner();
	// TODO: don't need to find TMap only capsules is enough
	FindCapsuleHitRegistrators(OwnerCapsulesSource.Actor).GenerateValueArray(OwnerCapsulesSource.CapsuleHitRegistrators);
	Result.Add(OwnerCapsulesSource);

	const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
	
	// find capsules on additional Sources
	for (const UAdditionalDamageBehaviorsSourceEvaluator* SourceEvaluator : SourceEvaluators_In)
	{
		if (SourceEvaluator)
		{
			// fill capsules
			FDBSHitRegistratorsSource CapsulesSource = {};
			CapsulesSource.SourceName = SourceEvaluator->SourceName;
			CapsulesSource.Actor = SourceEvaluator->GetActorWithCapsules(GetOwningActor());
			// TODO: don't need to find TMap only capsules is enough
			if (CapsulesSource.Actor)
			{
				FindCapsuleHitRegistrators(CapsulesSource.Actor).GenerateValueArray(CapsulesSource.CapsuleHitRegistrators);
				Result.Add(CapsulesSource);
			}
		}
	}

	return Result;
}

TArray<UAdditionalDamageBehaviorsSourceEvaluator*> UDamageBehaviorsComponent::SpawnEvaluators()
{
    TArray<UAdditionalDamageBehaviorsSourceEvaluator*> Result = {};

    const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
    if (!DamageBehaviorsSystemSettings)
    {
        return Result;
    }

    for (const TSubclassOf<UAdditionalDamageBehaviorsSourceEvaluator>& AdditionalCapsulesSourceEvaluator : DamageBehaviorsSystemSettings->AdditionalDamageBehaviorsSourcesEvaluators)
    {
        if (AdditionalCapsulesSourceEvaluator)
        {
            UAdditionalDamageBehaviorsSourceEvaluator* Evaluator = NewObject<UAdditionalDamageBehaviorsSourceEvaluator>(GetTransientPackage(), AdditionalCapsulesSourceEvaluator);
            if (Evaluator)
            {
                Result.Add(Evaluator);
            }
        }
    }
    DamageBehaviorsSourceEvaluators = Result;
    return Result;
}

// TODO: попытаться вернуть, но хз зач все это можно в DamageBehavior'ах ловить
// void UDamageBehaviorsComponent::DefaultOnHitAnything(
// 	const UDamageBehavior* DamageBehavior,
// 	const FDBSHitRegistratorHitResult& GetHitResult, const bool bSpawnFX)
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

#if WITH_EDITOR
void UDamageBehaviorsComponent::PostLoad()
{
	Super::PostLoad();
	SyncAllBehaviorSources();
}

void UDamageBehaviorsComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// Whenever anything changes, re-sync all behaviors
	SyncAllBehaviorSources();
}
#endif

void UDamageBehaviorsComponent::SyncAllBehaviorSources()
{
	for (UDamageBehavior* Behavior : DamageBehaviors)
	{
		if (Behavior)
		{
			Behavior->SyncSourcesFromSettings();
		}
	}
}
