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
		UDamageBehavior* DamageBehavior = NewObject<UDamageBehavior>(this, DamageBehaviorInstanced->StaticClass(), FName(DamageBehaviorInstanced->Name));
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
		if (DamageBehavior->bAutoHandleDamage)
		{
			DamageBehavior->OnHitRegistered.AddUniqueDynamic(this, &ThisClass::DefaultOnHitAnything);
		}
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
	
    bool bIsDebugEnabled = false;
// #if ENABLE_DRAW_DEBUG
//     UDebugSubsystem* DebugSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>();
//     bIsDebugEnabled = DebugSubsystem->IsCategoryEnabled(EDebugCategory::HitBoxes);
// #endif

	const FString LocalDamageBehaviorName = DamageBehaviorName; // Create a local copy to ensure string stability

	for (const FString& BehaviorsSourcesToUse : ResultDamageBehaviorsSourcesToUse)
	{
		FDamageBehaviorsSource* DamageBehaviorsSource = DamageBehaviorsSources.FindByKey(BehaviorsSourcesToUse);
		if (!DamageBehaviorsSource || !DamageBehaviorsSource->DamageBehaviorsComponent)
		{
			continue;
		}

		if (DamageBehaviorsSource->SourceName == DEFAULT_DAMAGE_BEHAVIOR_SOURCE)
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
		else if (DamageBehaviorsSource->DamageBehaviorsComponent != this)  // Prevent recursion to self
		{
			// When forwarding to another component, we only want to try the default source there
			// to prevent potential cycles between components
			DamageBehaviorsSource->DamageBehaviorsComponent->InvokeDamageBehavior(
				LocalDamageBehaviorName,
				bShouldActivate,
				{ DEFAULT_DAMAGE_BEHAVIOR_SOURCE },
				Payload);
		}
	}
}

UDamageBehavior* UDamageBehaviorsComponent::GetDamageBehavior(const FString Name) const
{
	UDamageBehavior* const* DamageBehaviorSearch = DamageBehaviors.FindByPredicate(
		[&](const UDamageBehavior* Behavior)
		{
			if (!IsValid(Behavior))
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
	
	// find capsules on additional Sources
	for (const UDamageBehaviorsSourceEvaluator* SourceEvaluator : SourceEvaluators_In)
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

TArray<UDamageBehaviorsSourceEvaluator*> UDamageBehaviorsComponent::SpawnEvaluators()
{
    TArray<UDamageBehaviorsSourceEvaluator*> Result = {};

    const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
    if (!DamageBehaviorsSystemSettings)
    {
        return Result;
    }

    for (const TSubclassOf<UDamageBehaviorsSourceEvaluator>& AdditionalCapsulesSourceEvaluator : DamageBehaviorsSystemSettings->AdditionalDamageBehaviorsSourcesEvaluators)
    {
        if (AdditionalCapsulesSourceEvaluator)
        {
            UDamageBehaviorsSourceEvaluator* Evaluator = NewObject<UDamageBehaviorsSourceEvaluator>(GetTransientPackage(), AdditionalCapsulesSourceEvaluator);
            if (Evaluator)
            {
                Result.Add(Evaluator);
            }
        }
    }
    DamageBehaviorsSourceEvaluators = Result;
    return Result;
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

void UDamageBehaviorsComponent::DefaultOnHitAnything(
	const FDBSHitRegistratorHitResult& HitRegistratorHitResult,
	const class UDamageBehavior* DamageBehavior,
	const UCapsuleHitRegistrator* CapsuleHitRegistrator,
	// TODO: use one InstancedStruct, we can put in one struct another InstancedStructs
	// no need to use TArray
	const FInstancedStruct& Payload)
{
	if (OnHitAnything.IsBound())
	{
		OnHitAnything.Broadcast(DamageBehavior, DamageBehavior->Name, HitRegistratorHitResult, CapsuleHitRegistrator, Payload);
	}
}
