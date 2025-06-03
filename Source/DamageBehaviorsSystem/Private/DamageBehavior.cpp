// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageBehavior.h"

#include "Kismet/GameplayStatics.h"
#include "CapsuleHitRegistrator.h"
#include "DamageBehaviorsSystemSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DamageBehavior)

void UDamageBehavior::Init(
	AActor* Owner_In,
	const TArray<FCapsuleHitRegistratorsSource>& CapsuleHitRegistratorsSources_In
)
{
    OwnerActor = Owner_In;
	CapsuleHitRegistratorsSources = CapsuleHitRegistratorsSources_In;

	for (const FCapsuleHitRegistratorsSource& CapsuleHitRegistratorsSource : CapsuleHitRegistratorsSources)
	{
		if (IsValid(CapsuleHitRegistratorsSource.Actor) && CapsuleHitRegistratorsSource.CapsuleHitRegistrators.Num() > 0)
		{
			for (UCapsuleHitRegistrator* CapsuleHitRegistrator : CapsuleHitRegistratorsSource.CapsuleHitRegistrators)
			{
				// TODO check that "AddUniqueDynamic" correctly works
				CapsuleHitRegistrator->OnHitRegistered.AddUniqueDynamic(this, &UDamageBehavior::ProcessHit);
			}	
		}
		else
		{
			// TODO: validation failed message
		}		
	} 

	DebugSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UUHLDebugSystemSubsystem>();
}

TArray<UCapsuleHitRegistrator*> UDamageBehavior::GetCapsuleHitRegistratorsFromAllSources() const
{
	TArray<UCapsuleHitRegistrator*> Result = {};
	for (FCapsuleHitRegistratorsSource CapsuleHitRegistratorsSource : CapsuleHitRegistratorsSources)
	{
		Result.Append(CapsuleHitRegistratorsSource.CapsuleHitRegistrators);
	}
	return Result;
}

void UDamageBehavior::ProcessHit(const FCapsuleHitRegistratorHitResult& CapsuleHitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator)
{
    if (!bIsActive) return;

	// TODO: return debug
	// bool bIsDebugEnabled = DebugSubsystem->IsCategoryEnabled(BGameplayTags::FindTagByString(BGameplayTags::TAG_DebugCategory_HitLog));
	bool bIsDebugEnabled = true;
	
    AActor* HitActor = CapsuleHitRegistratorHitResult.HitActor.Get();
    if (!IsValid(HitActor) || this->HitActors.Contains(HitActor)) return;

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
			SurfaceName = EnumPtr->GetDisplayNameTextByValue((int64)CapsuleHitRegistratorHitResult.PhysicalSurfaceType).ToString();
		}
		
		DrawDebugString(
			OwnerActor->GetWorld(),
			CapsuleHitRegistratorHitResult.HitResult.Location + FVector(0.0f, 25.0f, 0.0f),
			FString::Printf(TEXT("%s try hit %s | Time: %fs | %s | HitActors %s"), *CapsuleHitRegistratorHitResult.Instigator->GetName(), *HitActor->GetName(), (Seconds + Milliseconds), *SurfaceName, *HitActorsStr),
			nullptr,
			DebugColor,
			1.0f,
			true
		);
		DrawDebugPoint(OwnerActor->GetWorld(), CapsuleHitRegistratorHitResult.HitResult.Location, 10,
			DebugColor, false, 1, -1);
	}

	const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();

	UClass* HittableInterfaceClass = DamageBehaviorsSystemSettings->HittableInterfaceRef.LoadSynchronous();
	if (!HittableInterfaceClass)
	{
		// TODO: error handling
		return;
	} 
	
	// TODO: looks like this SHOULD BE in HandleHit
    if (CanGetHit(CapsuleHitRegistratorHitResult, CapsuleHitRegistrator))
    {
    	// UObject* HitTarget = IHittableInterface::Execute_GetHitTarget(HitActor);
    	UObject* HitTarget = GetRootAttachedActor(HitActor);
    	if (IsValid(HitTarget) && IsValid(HitActor) && HitTarget != HitActor)
    	{
    		// HitTarget - это всегда BaseCharacter, поэтому его можно аттачить к капсуле
    		AActor* HitTargetActor = Cast<AActor>(HitTarget);
    		AddHittedActor(HitTargetActor, true, true);

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

	TArray<FInstancedStruct> Payload_Out = {};
	bool bResult = HandleHit(CapsuleHitRegistratorHitResult, CapsuleHitRegistrator, Payload_Out);

	if (bResult)
	{
		if (OnHitRegistered.IsBound())
		{
			OnHitRegistered.Broadcast(CapsuleHitRegistratorHitResult, this, CapsuleHitRegistrator, Payload_Out);
		}
	}
}

void UDamageBehavior::MakeActive_Implementation(bool bShouldActivate, const TArray<FInstancedStruct>& Payload)
{
    bIsActive = bShouldActivate;

	for (const FCapsuleHitRegistratorsSource& CapsuleHitRegistratorsSource : CapsuleHitRegistratorsSources)
	{
		if (IsValid(CapsuleHitRegistratorsSource.Actor) && CapsuleHitRegistratorsSource.CapsuleHitRegistrators.Num() > 0)
		{
			FSourceHitRegistratorsToActivate* HitRegistratorsToActivate = SourceHitRegistratorsToActivate.FindByPredicate([=](const FSourceHitRegistratorsToActivate& Source) {
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

bool UDamageBehavior::CanGetHit_Implementation(
	const FCapsuleHitRegistratorHitResult& CapsuleHitRegistratorHitResult,
	UCapsuleHitRegistrator* CapsuleHitRegistrator)
{
	return CapsuleHitRegistratorHitResult.HitActor.IsValid();
}

bool UDamageBehavior::HandleHit_Implementation(
	const FCapsuleHitRegistratorHitResult& CapsuleHitRegistratorHitResult,
	UCapsuleHitRegistrator* CapsuleHitRegistrator,
	TArray<FInstancedStruct>& Payload_Out)
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