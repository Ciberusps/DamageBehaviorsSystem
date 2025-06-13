// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageBehavior.h"

#include "Kismet/GameplayStatics.h"
#include "CapsuleHitRegistrator.h"
#include "DamageBehaviorsSystemSettings.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DamageBehavior)

void UDamageBehavior::Init(
	AActor* Owner_In,
	const TArray<FDBSHitRegistratorsSource>& CapsuleHitRegistratorsSources_In
)
{
    OwnerActor = Owner_In;
	HitRegistratorsSources = CapsuleHitRegistratorsSources_In;

	for (const FDBSHitRegistratorsSource& CapsuleHitRegistratorsSource : HitRegistratorsSources)
	{
		if (IsValid(CapsuleHitRegistratorsSource.Actor) && CapsuleHitRegistratorsSource.CapsuleHitRegistrators.Num() > 0)
		{
			for (UCapsuleHitRegistrator* CapsuleHitRegistrator : CapsuleHitRegistratorsSource.CapsuleHitRegistrators)
			{
				// TODO check that "AddUniqueDynamic" correctly works
				CapsuleHitRegistrator->OnHitRegistered.AddUniqueDynamic(this, &UDamageBehavior::HandleHitInternally);
			}	
		}
		else
		{
			// TODO: validation failed message
		}		
	} 
}

AActor* UDamageBehavior::GetHitTarget_Implementation(
	AActor* HitActor_In,
	const FDBSHitRegistratorHitResult& HitRegistratorHitResult,
	UCapsuleHitRegistrator* CapsuleHitRegistrator) const
{
	return GetRootAttachedActor(HitActor_In);
}

TArray<UCapsuleHitRegistrator*> UDamageBehavior::GetCapsuleHitRegistratorsFromAllSources() const
{
	TArray<UCapsuleHitRegistrator*> Result = {};
	for (FDBSHitRegistratorsSource CapsuleHitRegistratorsSource : HitRegistratorsSources)
	{
		Result.Append(CapsuleHitRegistratorsSource.CapsuleHitRegistrators);
	}
	return Result;
}

#if WITH_EDITOR
void UDamageBehavior::PostInitProperties()
{
	Super::PostInitProperties();
	// Only sync on *instances*, not the CDO
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		SyncSourcesFromSettings();
	}
}

void UDamageBehavior::PostLoad()
{
	Super::PostLoad();
	SyncSourcesFromSettings();
}

void UDamageBehavior::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SyncSourcesFromSettings();
}
#endif

void UDamageBehavior::SyncSourcesFromSettings()
{
	const UDamageBehaviorsSystemSettings* Settings = GetDefault<UDamageBehaviorsSystemSettings>();
    if (!Settings) return;

    // 1) Build the canonical list of source names
    TArray<FString> DesiredNames = { DEFAULT_DAMAGE_BEHAVIOR_SOURCE };
    for (auto& EvalClass : Settings->DamageBehaviorsSourcesEvaluators)
    {
        if (EvalClass)
        {
            if (auto* CDO = EvalClass->GetDefaultObject<UDamageBehaviorsSourceEvaluator>())
            {
                if (!CDO->SourceName.IsEmpty())
                {
                    DesiredNames.AddUnique(CDO->SourceName);
                }
            }
        }
    }

    // 2) Rebuild the DebugActors array to match DesiredNames exactly
    bool bChanged = false;
    TArray<FDBSHitRegistratorsToActivateSource> NewSources = {};

    for (const FString& SourceName : DesiredNames)
    {
        // find existing entry
        int32 FoundIdx = HitRegistratorsToActivateBySource.IndexOfByPredicate(
            [&](const FDBSHitRegistratorsToActivateSource& E)
            {
                return E.SourceName == SourceName;
            });

        if (FoundIdx != INDEX_NONE)
        {
            // move the old entry into the new array (preserving HitRegistratorsNames)
            NewSources.Add(MoveTemp(HitRegistratorsToActivateBySource[FoundIdx]));
        }
        else
        {
            // create a fresh entry
            FDBSHitRegistratorsToActivateSource E;
            E.SourceName = SourceName;
            NewSources.Add(E);
        }
    }

    // detect if anything actually changed (length or order or content pointers)
    if (NewSources.Num() != HitRegistratorsToActivateBySource.Num())
    {
        bChanged = true;
    }
    else
    {
        for (int32 i = 0; i < NewSources.Num(); ++i)
        {
            if (NewSources[i].SourceName != HitRegistratorsToActivateBySource[i].SourceName ||
                &NewSources[i].HitRegistratorsNames != &HitRegistratorsToActivateBySource[i].HitRegistratorsNames)
            {
                bChanged = true;
                break;
            }
        }
    }

    // 3) If changed, apply it
    if (bChanged)
    {
        Modify();
        HitRegistratorsToActivateBySource = MoveTemp(NewSources);
    }
}

TArray<FString> UDamageBehavior::GetHitRegistratorsNameOptions() const
{
	TArray<FString> Result = {};
	UObject* Outer = GetOuter();
	Result = GetNamesOfComponentsOnObject(Outer, UCapsuleHitRegistrator::StaticClass());
	return Result;
}

void UDamageBehavior::HandleHitInternally(const FDBSHitRegistratorHitResult& HitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator)
{
    if (!bIsActive) return;

    AActor* HitActor = HitRegistratorHitResult.HitActor.Get();
    if (!IsValid(HitActor) || this->HitActors.Contains(HitActor)) return;

	auto CVarDBSHitLog = IConsoleManager::Get().FindConsoleVariable(TEXT("DamageBehaviorsSystem.HitLog"));
	bool bIsDebugEnabled = CVarDBSHitLog ? CVarDBSHitLog->GetBool() : false;
	if (bIsDebugEnabled && OwnerActor.IsValid())
	{
		FString HitActorsStr = "";
		for (TWeakObjectPtr<AActor> Actor : this->HitActors)
		{
			if (Actor.IsValid())
			{
				HitActorsStr += Actor.Get()->GetName() + ", ";
			}
		}

		int32 Seconds = 0;
		double Milliseconds = 0;
		UGameplayStatics::GetAccurateRealTime(Seconds, Milliseconds);
		FColor DebugColor = FColor::MakeRandomColor();
		FString SurfaceName = "Surface_Error";
		if (const UEnum* EnumPtr = StaticEnum<EPhysicalSurface>())
		{
			SurfaceName = EnumPtr->GetDisplayNameTextByValue((int64)HitRegistratorHitResult.PhysicalSurfaceType).ToString();
		}
		
		DrawDebugString(
			OwnerActor->GetWorld(),
			HitRegistratorHitResult.HitResult.Location + FVector(0.0f, 25.0f, 0.0f),
			FString::Printf(TEXT("%s try hit %s | Time: %fs | %s | HitActors %s"), *HitRegistratorHitResult.Instigator->GetName(), *HitActor->GetName(), (Seconds + Milliseconds), *SurfaceName, *HitActorsStr),
			nullptr,
			DebugColor,
			1.0f,
			true
		);
		DrawDebugPoint(OwnerActor->GetWorld(), HitRegistratorHitResult.HitResult.Location, 10,
			DebugColor, false, 1, -1);
	}

	AActor* HitTargetActor = HitActor;
    if (CanBeAddedToHittedActors(HitRegistratorHitResult, CapsuleHitRegistrator))
    {
    	// UObject* HitTarget = IHittableInterface::Execute_GetHitTarget(HitActor);
    	HitTargetActor = GetHitTarget(HitActor, HitRegistratorHitResult, CapsuleHitRegistrator);
    	if (IsValid(HitTargetActor) && IsValid(HitActor) && HitTargetActor != HitActor)
    	{
    		// HitTarget - это всегда BaseCharacter, поэтому его можно аттачить к капсуле
    		AddHittedActor(HitTargetActor, true, true);

    		// TODO: remove and test
    		// and current HitActor, mostly it should be attached
    		// but for sure/safety adding explicitly
    		AddHittedActor(HitActor, false, true);
    	}
    	else
    	{
    		// если нету HitTarget'а(BaseCharacter'а) значит можно аттачить к капсуле
    		AddHittedActor(HitActor, true, true);
    	}
    }

	FInstancedStruct Payload_Out = {};
	bool bResult = ProcessHit(HitRegistratorHitResult, CapsuleHitRegistrator, Payload_Out);

	if (bResult)
	{
		if (OnHitRegistered.IsBound())
		{
			OnHitRegistered.Broadcast(HitRegistratorHitResult, this, CapsuleHitRegistrator, Payload_Out);
		}
	}
}

void UDamageBehavior::MakeActive_Implementation(bool bShouldActivate, const FInstancedStruct& Payload)
{
    bIsActive = bShouldActivate;

	for (const FDBSHitRegistratorsSource& CapsuleHitRegistratorsSource : HitRegistratorsSources)
	{
		if (IsValid(CapsuleHitRegistratorsSource.Actor) && CapsuleHitRegistratorsSource.CapsuleHitRegistrators.Num() > 0)
		{
			FDBSHitRegistratorsToActivateSource* HitRegistratorsToActivate = HitRegistratorsToActivateBySource.FindByPredicate([=](const FDBSHitRegistratorsToActivateSource& Source) {
				return Source.SourceName == CapsuleHitRegistratorsSource.SourceName;
			});
			if (!HitRegistratorsToActivate) continue;
			
			for (UCapsuleHitRegistrator* CapsuleHitRegistrator : CapsuleHitRegistratorsSource.CapsuleHitRegistrators)
			{
				if (HitRegistratorsToActivate->HitRegistratorsNames.Contains(CapsuleHitRegistrator->GetName()))
				{
					CapsuleHitRegistrator->SetIsHitRegistrationEnabled(bShouldActivate, HitDetectionSettings);
				}
			}	
		}
		else
		{
			// TODO: validation failed message
		}
	}

    if (!bShouldActivate)
    {
        ClearHittedActors();
    }
}

bool UDamageBehavior::CanBeAddedToHittedActors_Implementation(
	const FDBSHitRegistratorHitResult& HitRegistratorHitResult,
	UCapsuleHitRegistrator* CapsuleHitRegistrator)
{
	return HitRegistratorHitResult.HitActor.IsValid();
}

bool UDamageBehavior::ProcessHit_Implementation(
	const FDBSHitRegistratorHitResult& HitRegistratorHitResult,
	UCapsuleHitRegistrator* CapsuleHitRegistrator,
	FInstancedStruct& Payload_Out)
{
	return true;
}

void UDamageBehavior::AddHittedActor_Implementation(AActor* Actor_In, bool bCanBeAttached, bool bAddAttachedActorsToActorAlso = true)
{
	if (HitDetectionSettings.HitDetectionType == EDamageBehaviorHitDetectionType::ByEntering) return;

    this->HitActors.Add(Actor_In);

	if (bAddAttachedActorsToActorAlso)
	{
		// add all attached actors also
		TArray<AActor*> AttachedActorsToActor;
		Actor_In->GetAttachedActors(AttachedActorsToActor);
		for (AActor* AttachedActor : AttachedActorsToActor)
		{
			this->HitActors.Add(AttachedActor);
		}	
	}
	
    if (bCanBeAttached && this->bAttachEnemiesToCapsuleWhileActive)
    {
        Actor_In->AttachToActor(OwnerActor.Get(), FAttachmentTransformRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, true), NAME_None);
        this->AttachedActors.Add(Actor_In);
    }
}

void UDamageBehavior::ClearHittedActors_Implementation()
{
    for (TWeakObjectPtr<AActor> HitActor : this->AttachedActors)
    {
    	if (!HitActor.IsValid()) continue;
        HitActor->K2_DetachFromActor(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld);
    }
    this->HitActors.Empty();
    this->AttachedActors.Empty();
}

AActor* UDamageBehavior::GetRootAttachedActor(AActor* Actor_In) const
{
	AActor* Current = Actor_In;
	// Loop until there is no longer an AttachParent
	while (AActor* Parent = Current->GetAttachParentActor())
	{
		Current = Parent;
	}
	return Current;
}

TArray<FString> UDamageBehavior::GetNamesOfComponentsOnObject(UObject* OwnerObject, UClass* Class) const
{
	TArray<FString> Result = {};

	UBlueprintGeneratedClass* BlueprintGeneratedClass =
		OwnerObject->IsA<UBlueprintGeneratedClass>() ? Cast<UBlueprintGeneratedClass>(OwnerObject) : Cast<UBlueprintGeneratedClass>(OwnerObject->GetClass());
	if (!BlueprintGeneratedClass)
		return Result;

	TArray<UObject*> DefaultObjectSubobjects;
	BlueprintGeneratedClass->GetDefaultObjectSubobjects(DefaultObjectSubobjects);

	// Search for ActorComponents created from C++
	for (UObject* DefaultSubObject : DefaultObjectSubobjects)
	{
		if (DefaultSubObject->IsA(Class))
		{
			Result.Add(DefaultSubObject->GetName());
		}
	}

	// Search for ActorComponents created in Blueprint
	for (USCS_Node* Node : BlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes())
	{
		if (Node->ComponentClass->IsChildOf(Class))
		{
			Result.Add(Node->GetVariableName().ToString());
		}
	}

	return Result;
}