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

	// Now activate the ones that need to start active
	for (UDamageBehavior* DamageBehavior : DamageBehaviorsList)
	{
		if (!DamageBehavior) continue;

		DamageBehavior->Init(
			GetOwningActor(),
			CapsuleHitRegistratorsSources
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
	TObjectPtr<UDamageBehavior> const* DamageBehaviorSearch = DamageBehaviorsList.FindByPredicate(
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
	
	// find capsules on additional DebugActors
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

void UDamageBehaviorsComponent::PostLoad()
{
	Super::PostLoad();
	SyncAllBehaviorSources();
}

#if WITH_EDITOR
void UDamageBehaviorsComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bool bOne = HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject);
	bool bOneAndHalf = (GetOwner() && GetOwner()->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject));
	bool bTwo = PropertyChangedEvent.Property != nullptr;
	bool bThree = PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UDamageBehaviorsComponent, DamageBehaviorsList);
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("bOne %hhd, bTwo %hhd, bThree %hhd"), bOne, bTwo, bThree
	);
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Magenta, FString::Printf(TEXT("[MovingRightCameraYawAlign] RelativeDirection %hhd %hhd %hhd"), bOne, bTwo, bThree));
	// Only run on the Class Default Object
	if ((bOne || bOneAndHalf) && bTwo && bThree)
	{
		// Find the array property for DamageBehaviorsList
        FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(
            UDamageBehaviorsComponent::StaticClass(),
            GET_MEMBER_NAME_CHECKED(UDamageBehaviorsComponent, DamageBehaviorsList)
        );
        if (!ArrayProp)
        {
            UE_LOG(LogTemp, Warning, TEXT("UDamageBehaviorsComponent: Failed to find FArrayProperty for DamageBehaviorsList"));
            return;
        }

		// PropertyChangedEvent.Property

        // Get the blueprint CDO (handles native and blueprint defaults)
        // UDamageBehaviorsComponent* CDO = Cast<UDamageBehaviorsComponent>(GetClass()->GetDefaultObject());
        // if (!CDO)
        // {
        //     UE_LOG(LogTemp, Warning, TEXT("UDamageBehaviorsComponent: Failed to get class default object"));
        //     return;
        // }

        // Access the default list
        const TArray<TObjectPtr<UDamageBehavior>>* SrcList = ArrayProp->ContainerPtrToValuePtr<const TArray<TObjectPtr<UDamageBehavior>>>(this);
        if (!SrcList)
        {
            UE_LOG(LogTemp, Warning, TEXT("UDamageBehaviorsComponent: SrcList is null"));
            return;
        }

        // Iterate all component instances
        for (TObjectIterator<UDamageBehaviorsComponent> It; It; ++It)
        {
            UDamageBehaviorsComponent* Comp = *It;
            // Skip templates, non-world instances, and any TRASH_ prefixed components
            const bool bIsTemplate = Comp->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
                || (Comp->GetOwner() && Comp->GetOwner()->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject));
            const FString CompName = Comp->GetName();
            if (!Comp->GetOwner() || !Comp->GetWorld() || bIsTemplate
                || CompName.StartsWith(TEXT("TRASH_"))
                || Comp->GetOwner()->GetName().StartsWith(TEXT("TRASH_")))
            {
                continue;
            }

            Comp->Modify();
            TArray<TObjectPtr<UDamageBehavior>>* DestList = ArrayProp->ContainerPtrToValuePtr<TArray<TObjectPtr<UDamageBehavior>>>(Comp);
            if (DestList)
            {
                // Copy default list into instance
                *DestList = *SrcList;

                // Notify editor UI of change
                FPropertyChangedEvent ChangeEvent(ArrayProp);
                ChangeEvent.ChangeType = EPropertyChangeType::ValueSet;
                Comp->PostEditChangeProperty(ChangeEvent);
            }
        }
	}

	SyncAllBehaviorSources();
}
#endif

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
