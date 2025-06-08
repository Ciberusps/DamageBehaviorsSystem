// Fill out your copyright notice in the Description page of Project Settings.


#include "CapsuleHitRegistrator.h"

#include "DamageBehaviorsSystemSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/UHLTraceUtilsBPL.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CapsuleHitRegistrator)

const TCHAR* DBSHitBoxesCVarName = TEXT("DamageBehaviorsSystem.HitBoxes");
static TAutoConsoleVariable<int32> CVarDBSHitBoxes(
	DBSHitBoxesCVarName,
	0,
	TEXT("Show hitboxes"),
	ECVF_Default
);

static TAutoConsoleVariable<int32> CVarDBSHitBoxesHistory(
	TEXT("DamageBehaviorsSystem.HitBoxes.History"),
	0,
	TEXT("Show hitboxes history"),
	ECVF_Default
);

UCapsuleHitRegistrator::UCapsuleHitRegistrator()
{
	PrimaryComponentTick.TickInterval = 0.0f;
	PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
	CapsuleRadius = 5.0f;
    ShapeColor = FColor(255.0f, 0.0f, 0.0f);
	SetCollisionProfileName(FName("NoCollision"));
    SetTickGroup(TG_PostPhysics);
}

void UCapsuleHitRegistrator::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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
	// TODO: move on DeveloperSettingsBackedByCVars and just check CVar to remove UHLDebugSystemDependency
	// in UHLDebugSystem just turn on/off CVars to enabled/disable debug
	bIsDebugEnabled = CVarDBSHitBoxes.GetValueOnAnyThread() != 0;
	bIsHistoryEnabled = CVarDBSHitBoxesHistory.GetValueOnAnyThread() != 0;
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

	bool bHasMeleeHit = UUHLTraceUtilsBPL::SweepCapsuleMultiByChannel(
		GetWorld(),
		HitResults,
		PreviousComponentLocation,
		CurrentLocation,
		GetScaledCapsuleRadius(),
		GetScaledCapsuleHalfHeight(),
		GetComponentRotation().Quaternion(),
		// TODO replace by ECC_Melee later and remove ECC_MeleeAndSurfaces if all workes fine
		DamageBehaviorsSystemSettings->HitRegistratorsTraceChannel,
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
			IConsoleVariable* DBSHitBoxesCVar = IConsoleManager::Get().FindConsoleVariable(DBSHitBoxesCVarName);
			UpdateCapsuleVisibility(DBSHitBoxesCVar && DBSHitBoxesCVar->GetBool());

			if (bIsEnabled_In)
			{
				OnComponentBeginOverlap.AddUniqueDynamic(this, &UCapsuleHitRegistrator::OnBegingOverlap);
				OnComponentEndOverlap.AddUniqueDynamic(this, &UCapsuleHitRegistrator::OnEndOverlap);
				SetCollisionProfileName(CurrentHitDetectionSettings.CollisionProfileName.Name);

				if (DBSHitBoxesCVar)
				{
					DBSHitBoxesCVar->SetOnChangedCallback(
						FConsoleVariableDelegate::CreateUObject(this, &ThisClass::OnDebugCategoryChanged)
					);
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
			SetComponentTickEnabled(bIsEnabled_In);
			bIsHitRegistrationEnabled = bIsEnabled_In;
			break;
		}
	}
}

void UCapsuleHitRegistrator::AddActorsToIgnoreList(const TArray<AActor*>& Actors_In)
{
    IgnoredActors.Append(Actors_In);
}

void UCapsuleHitRegistrator::BeginPlay()
{
    Super::BeginPlay();

    UHLDebugSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UUHLDebugSystemSubsystem>();
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