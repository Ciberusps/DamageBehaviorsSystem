// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageBehaviorsComponent.h"

#include "CapsuleHitRegistrator.h"
#include "DamageBehaviorsSource.h"
#include "DamageBehaviorsSystemSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DamageBehaviorsComponent)

DEFINE_LOG_CATEGORY(LogDamageBehaviorsSystem);


void UDamageBehaviorsComponent::BeginPlay()
{
	Super::BeginPlay();

    OwnerActor = GetOwningActor();

	PrepareDamageBehaviorsSources();
	
	// Now activate the ones that need to start active
	for (UDamageBehavior* DamageBehavior : DamageBehaviorsList)
	{
		if (!DamageBehavior) continue;

		const TArray<FDBSHitRegistratorsSource>& HitRegistratorsSources = GetHitRegistratorsSources(DamageBehaviorsSourceEvaluators);
		DamageBehavior->Init(
			GetOwningActor(),
			HitRegistratorsSources
		);
		
		// if (DamageBehavior->bAutoHandleDamage)
		// {
			DamageBehavior->OnHitRegistered.AddUniqueDynamic(this, &ThisClass::DefaultOnHitAnything);
		// }
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
	const FString DamageBehaviorName,
	const bool bShouldActivate,
	const TArray<FString>& DamageBehaviorsSourcesToUse,
	const FInstancedStruct& Payload
)
{
	if (DamageBehaviorName.IsEmpty())
	{
		UE_LOG(LogDamageBehaviorsSystem, Warning, TEXT("UDamageBehaviorsComponent::InvokeDamageBehavior() called with empty DamageBehaviorName"));
		return;
	}

	TArray<FString> ResultDamageBehaviorsSourcesToUse = DamageBehaviorsSourcesToUse;
	if (DamageBehaviorsSourcesToUse.Num() == 0)
	{
		ResultDamageBehaviorsSourcesToUse = {
			DEFAULT_DAMAGE_BEHAVIOR_SOURCE,
		};
	}
	
	auto CVarDBSHitBoxes = IConsoleManager::Get().FindConsoleVariable(TEXT("DamageBehaviorsSystem.HitBoxes"));
    bool bIsDebugEnabled = CVarDBSHitBoxes ? CVarDBSHitBoxes->GetBool() : false;

	const FString LocalDamageBehaviorName = DamageBehaviorName; // Create a local copy to ensure string stability

	for (const FString& BehaviorsSourcesToUse : ResultDamageBehaviorsSourcesToUse)
	{
		if (BehaviorsSourcesToUse == DEFAULT_DAMAGE_BEHAVIOR_SOURCE)
		{
			UDamageBehavior* DamageBehavior = GetDamageBehavior(LocalDamageBehaviorName);
			if (!DamageBehavior)
			{
				if (bIsDebugEnabled)
				{
					UE_LOG(LogDamageBehaviorsSystem, Warning, TEXT("UDamageBehaviorsComponent::InvokeDamageBehavior() DamageBehavior \"%s\" not found on character looking for weapon"), *LocalDamageBehaviorName);
				}
			}
			else
			{
				DamageBehavior->MakeActive(bShouldActivate, Payload);
			}
		}
		else
		{
			FDamageBehaviorsSource* DamageBehaviorsSource = DamageBehaviorsSources.FindByKey(BehaviorsSourcesToUse);
			UDamageBehaviorsComponent* DBSSourceDBComponent = DamageBehaviorsSource->GetDamageBehaviorsComponent();
			if (!DamageBehaviorsSource || !DBSSourceDBComponent)
			{
				continue;
			}
			if (DBSSourceDBComponent != this)  // Prevent recursion to self
			{
				// When forwarding to another component, we only want to try the default source there
				// to prevent potential cycles between components
				DBSSourceDBComponent->InvokeDamageBehavior(
					LocalDamageBehaviorName,
					bShouldActivate,
					{ DEFAULT_DAMAGE_BEHAVIOR_SOURCE },
					Payload);
			}
		}
	}
}

UDamageBehavior* UDamageBehaviorsComponent::GetDamageBehavior(const FString Name) const
{
	TObjectPtr<UDamageBehavior> const* DamageBehaviorSearch = DamageBehaviorsList.FindByPredicate(
		[&](const UDamageBehavior* Behavior)
		{
			if (!Behavior)
			{
				return false;
			}
			return Behavior->Name == Name;
		});
	return DamageBehaviorSearch ? *DamageBehaviorSearch : nullptr;
}

const TArray<FDBSHitRegistratorsSource> UDamageBehaviorsComponent::GetHitRegistratorsSources(
	TArray<UDamageBehaviorsSourceEvaluator*> SourceEvaluators_In
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
	
	// find capsules on additional DebugActors
	for (const UDamageBehaviorsSourceEvaluator* SourceEvaluator : SourceEvaluators_In)
	{
		if (SourceEvaluator)
		{
			// fill capsules
			FDBSHitRegistratorsSource CapsulesSource = {};
			CapsulesSource.SourceName = SourceEvaluator->SourceName;
			CapsulesSource.Actor = SourceEvaluator->GetActorWithDamageBehaviors(GetOwningActor());
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

TArray<UDamageBehaviorsSourceEvaluator*> UDamageBehaviorsComponent::SpawnEvaluators()
{
    TArray<UDamageBehaviorsSourceEvaluator*> Result = {};

    const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
    if (!DamageBehaviorsSystemSettings)
    {
        return Result;
    }

    for (const TSubclassOf<UDamageBehaviorsSourceEvaluator>& DBSEvaluator : DamageBehaviorsSystemSettings->DamageBehaviorsSourcesEvaluators)
    {
        if (DBSEvaluator)
        {
            UDamageBehaviorsSourceEvaluator* Evaluator = NewObject<UDamageBehaviorsSourceEvaluator>(GetTransientPackage(), DBSEvaluator);
            if (Evaluator)
            {
                Result.Add(Evaluator);
            }
        }
    }
    DamageBehaviorsSourceEvaluators = Result;
    return Result;
}

void UDamageBehaviorsComponent::PrepareDamageBehaviorsSources()
{
	DamageBehaviorsSources.Add(FDamageBehaviorsSource(DEFAULT_DAMAGE_BEHAVIOR_SOURCE, OwnerActor, nullptr)); 
	DamageBehaviorsSourceEvaluators = SpawnEvaluators();
	for (const TObjectPtr<UDamageBehaviorsSourceEvaluator> DamageBehaviorsSourceEvaluator : DamageBehaviorsSourceEvaluators)
	{
		DamageBehaviorsSources.Add(FDamageBehaviorsSource(DamageBehaviorsSourceEvaluator->SourceName, OwnerActor, DamageBehaviorsSourceEvaluator));
	}
}

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

void UDamageBehaviorsComponent::PostLoad()
{
	Super::PostLoad();
	SyncAllBehaviorSources();
}

void UDamageBehaviorsComponent::SyncAllBehaviorSources()
{
	for (UDamageBehavior* Behavior : DamageBehaviorsList)
	{
		if (Behavior)
		{
			Behavior->SyncSourcesFromSettings();
		}
	}
}

void UDamageBehaviorsComponent::DefaultOnHitAnything(
	const FDBSHitRegistratorHitResult& HitRegistratorHitResult,
	const class UDamageBehavior* DamageBehavior,
	const UCapsuleHitRegistrator* CapsuleHitRegistrator,
	const FInstancedStruct& Payload)
{
	if (OnHitAnything.IsBound())
	{
		OnHitAnything.Broadcast(DamageBehavior, DamageBehavior->Name, HitRegistratorHitResult, CapsuleHitRegistrator, Payload);
	}
}
