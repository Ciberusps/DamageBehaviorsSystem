// Fill out your copyright notice in the Description page of Project Settings.


#include "CapsuleHitRegistrator.h"

#include "DamageBehaviorsSystemSettings.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CapsuleHitRegistrator)


UCapsuleHitRegistrator::UCapsuleHitRegistrator()
{
	PrimaryComponentTick.bCanEverTick = false;
	CapsuleRadius = 5.0f;
    ShapeColor = FColor(255.0f, 0.0f, 0.0f);
	SetCollisionProfileName(FName("NoCollision"));
}

void UCapsuleHitRegistrator::TickHitRegistration(float /*DeltaTime*/)
{
	if (bIsHitRegistrationEnabled
		&& CurrentHitDetectionSettings.HitDetectionType == EDamageBehaviorHitDetectionType::ByTrace)
	{
		ProcessHitRegistration();
	}
	PreviousComponentLocation = GetComponentLocation();
}

void UCapsuleHitRegistrator::ProcessHitRegistration()
{
	bool bIsDebugEnabled = false;
	bool bIsHistoryEnabled = false;
#if ENABLE_DRAW_DEBUG
	auto CVarDBSHitBoxes = IConsoleManager::Get().FindConsoleVariable(TEXT("DamageBehaviorsSystem.HitBoxes"));
	bIsDebugEnabled = CVarDBSHitBoxes ? CVarDBSHitBoxes->GetBool() : false;
	auto CVarDBSHitBoxesHistory = IConsoleManager::Get().FindConsoleVariable(TEXT("DamageBehaviorsSystem.HitBoxes.History"));
	bIsHistoryEnabled = CVarDBSHitBoxesHistory ? CVarDBSHitBoxesHistory->GetBool() : false;
#endif

	FVector CurrentLocation = GetComponentLocation();
	TArray<FHitResult> HitResults;

    // fix to correct impact point if CurrentLocation and PrevioutLocation are the same
    if (CurrentLocation == PreviousComponentLocation)
    {
        CurrentLocation += FVector(0.1f, 0.0f, 0.0f);
    }

    FCollisionQueryParams CollisionParams;
	CollisionParams.bReturnPhysicalMaterial = true;

    AActor* OwnerActor = GetOwner();
	CollisionParams.AddIgnoredActor(OwnerActor);
    TArray<AActor*> CharacterAttachedActors;
    // ignore Character
    CollisionParams.AddIgnoredActor(OwnerActor->GetOwner());
    CollisionParams.AddIgnoredActors(IgnoredActors);
    // ignore all childs of Character
    OwnerActor->GetAttachedActors(CharacterAttachedActors, false, true);
    AActor* OwnerCharacter = OwnerActor->GetOwner();
    if (OwnerCharacter != nullptr)
    {
        OwnerCharacter->GetAttachedActors(CharacterAttachedActors, false, true);
        CollisionParams.AddIgnoredActors(CharacterAttachedActors);
    }

	const UDamageBehaviorsSystemSettings* DamageBehaviorsSystemSettings = GetDefault<UDamageBehaviorsSystemSettings>();
	TEnumAsByte<ECollisionChannel> TraceChannel = DamageBehaviorsSystemSettings->HitRegistratorsTraceChannel;
	if (CurrentHitDetectionSettings.bUseCustomTraceChannel)
	{
		TraceChannel = CurrentHitDetectionSettings.CustomTraceChannel;
	}
	
	bool bHasMeleeHit = SweepCapsuleMultiByChannel(
		GetWorld(),
		HitResults,
		PreviousComponentLocation,
		CurrentLocation,
		GetScaledCapsuleRadius(),
		GetScaledCapsuleHalfHeight(),
		GetComponentRotation().Quaternion(),
		TraceChannel,
		CollisionParams,
		FCollisionResponseParams::DefaultResponseParam,
		bIsDebugEnabled,
		bIsHistoryEnabled ? 1.0f : -1.0f,
		ShapeColor,
		FColor::Yellow,
		bIsHistoryEnabled ? 0.2f : -1.0f
	);
	if (bHasMeleeHit)
	{
		FVector Direction = (CurrentLocation - PreviousComponentLocation).GetSafeNormal();
		if (OnHitRegistered.IsBound())
		{
			for (FHitResult& HitResult : HitResults)
			{
				FDBSHitRegistratorHitResult HitRegistratorHitResult;
				HitRegistratorHitResult.HitResult = HitResult;
				HitRegistratorHitResult.HitActor = HitResult.GetActor();
				HitRegistratorHitResult.Direction = Direction;
				// Default instigator is Character owning capsule, but don't forget to override it if needed
				HitRegistratorHitResult.Instigator = OwnerActor;
				HitRegistratorHitResult.PhysicalSurfaceType = UGameplayStatics::GetSurfaceType(HitResult);
				OnHitRegistered.Broadcast(HitRegistratorHitResult, this);
			}
		}
	}
}

void UCapsuleHitRegistrator::SetIsHitRegistrationEnabled(bool bIsEnabled_In, FDamageBehaviorHitDetectionSettings HitDetectionSettings)
{
	// IgnoredActors used for projectiles to avoid low-level(CapsuleHitRegistrator) hitting enemies
	// if "AttachToCharacter"/"AttachToActors" not set
	IgnoredActors.Empty();

	CurrentHitDetectionSettings = HitDetectionSettings;
	
	switch (CurrentHitDetectionSettings.HitDetectionType)
	{
		case EDamageBehaviorHitDetectionType::ByEntering:
		{
			bIsHitRegistrationEnabled = bIsEnabled_In;
			IConsoleVariable* DBSHitBoxesCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("DamageBehaviorsSystem.HitBoxes"));
			UpdateCapsuleVisibility(DBSHitBoxesCVar && DBSHitBoxesCVar->GetBool());

			if (bIsEnabled_In)
			{
				OnComponentBeginOverlap.AddUniqueDynamic(this, &UCapsuleHitRegistrator::OnBegingOverlap);
				OnComponentEndOverlap.AddUniqueDynamic(this, &UCapsuleHitRegistrator::OnEndOverlap);
				SetCollisionProfileName(CurrentHitDetectionSettings.CollisionProfileName.Name);

				if (DBSHitBoxesCVar)
				{
					DBSHitBoxesCVar->OnChangedDelegate().AddUObject(this, &ThisClass::OnDebugCategoryChanged);
				}
				
				if (CurrentHitDetectionSettings.bCheckOverlappingActorsOnStart)
				{
					// TODO check that UpdateOverlapsUpdates it on start
					UpdateOverlaps();
				}
			}
			else
			{
				OnComponentBeginOverlap.RemoveDynamic(this, &UCapsuleHitRegistrator::OnBegingOverlap);
				OnComponentEndOverlap.RemoveDynamic(this, &UCapsuleHitRegistrator::OnEndOverlap);
				SetCollisionProfileName(FName("NoCollision"));
			}
			break;
		}
		case EDamageBehaviorHitDetectionType::ByTrace:
		{
			PreviousComponentLocation = GetComponentLocation();
			SetComponentTickEnabled(false);
			bIsHitRegistrationEnabled = bIsEnabled_In;
			break;
		}
	}
}

void UCapsuleHitRegistrator::AddActorsToIgnoreList(const TArray<AActor*>& Actors_In)
{
    IgnoredActors.Append(Actors_In);
}

void UCapsuleHitRegistrator::OnBegingOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OnHitRegistered.IsBound())
	{
		FDBSHitRegistratorHitResult HitRegistratorHitResult;
		HitRegistratorHitResult.HitResult = SweepResult;
		HitRegistratorHitResult.HitActor = OtherActor;
		// Default instigator is Character owning capsule, but don't forget to override it if needed
		HitRegistratorHitResult.Instigator = GetOwner();
		HitRegistratorHitResult.PhysicalSurfaceType = UGameplayStatics::GetSurfaceType(SweepResult);
		OnHitRegistered.Broadcast(HitRegistratorHitResult, this);
	}
}

void UCapsuleHitRegistrator::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

void UCapsuleHitRegistrator::OnDebugCategoryChanged(IConsoleVariable* Var)
{
	UpdateCapsuleVisibility(Var->GetBool());
}

void UCapsuleHitRegistrator::UpdateCapsuleVisibility(bool bIsVisible_In)
{
	if (CurrentHitDetectionSettings.HitDetectionType == EDamageBehaviorHitDetectionType::ByEntering)
	{
		bool bIsVisible = bIsHitRegistrationEnabled && bIsVisible_In;
		SetHiddenInGame(!bIsVisible);
	}
}

bool UCapsuleHitRegistrator::SweepCapsuleMultiByChannel(const UWorld* World, TArray<FHitResult>& OutHits, const FVector& Start, const FVector& End, float Radius, float HalfHeight, const FQuat& Rot,
	ECollisionChannel TraceChannel, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParam, bool bDrawDebug, float DrawTime, FColor TraceColor, FColor HitColor, float FailDrawTime)
{
	bool bResult = false;

	FailDrawTime = FailDrawTime == -1.0f ? DrawTime : FailDrawTime;

	FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	bResult = World->SweepMultiByChannel(OutHits, Start, End, Rot, TraceChannel, CollisionShape, Params, ResponseParam);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugCapsule(World, Start, HalfHeight, Radius, Rot, TraceColor, false, bResult ? DrawTime : FailDrawTime);
		DrawDebugCapsule(World, End, HalfHeight, Radius, Rot, TraceColor, false, bResult ? DrawTime : FailDrawTime);
		DrawDebugLine(World, Start, End, TraceColor, false, bResult ? DrawTime : FailDrawTime);

		if (bResult)
		{
			float Thickness = FMath::Clamp(HalfHeight / 100, 1.25, 5);
			for (const FHitResult& OutHit : OutHits)
			{
				// UUnrealHelperLibraryBPLibrary::DebugPrintStrings(FString::Printf(TEXT("%f"), Thickness));
				DrawDebugPoint(World, OutHit.ImpactPoint, 10.0f, HitColor, false, DrawTime, 0);
				DrawDebugCapsule(World, OutHit.Location, HalfHeight, Radius, Rot, TraceColor, false, DrawTime, 0, Thickness);	
			}
		}
	}
#endif

	return bResult;
}