// Test doubles for the current damage behavior contract.
#pragma once

#include "DamageBehavior.h"
#include "BehaviorTestObjects.generated.h"

UCLASS()
class UDBSBehaviorTestDouble : public UDamageBehavior
{
	GENERATED_BODY()
public:
	bool bAllowAdd = true;
	bool bProcessResult = true;
	bool bSetOutput = false;
	int32 ProcessCount = 0;

	virtual bool CanBeAddedToHittedActors_Implementation(const FDBSHitRegistratorHitResult& Hit, UCapsuleHitRegistrator* Registrator) override
	{
		return bAllowAdd;
	}

	virtual bool ProcessHit_Implementation(const FDBSHitRegistratorHitResult& Hit, UCapsuleHitRegistrator* Registrator, FInstancedStruct& PayloadOut) override
	{
		++ProcessCount;
		if (bSetOutput)
		{
			FDBSNoiseEventOnDamageSettings Value;
			Value.Loudness = 42.0f;
			PayloadOut.InitializeAs<FDBSNoiseEventOnDamageSettings>(Value);
		}
		return bProcessResult;
	}
};

UCLASS()
class UDBSBehaviorTestObserver : public UObject
{
	GENERATED_BODY()
public:
	int32 HitCount = 0;
	int32 EndCount = 0;
	bool bLastEndHit = false;
	FInstancedStruct LastPayload;
	TWeakObjectPtr<AActor> LastActor;

	UFUNCTION()
	void OnHit(const FDBSHitRegistratorHitResult& Hit, const UDamageBehavior* Behavior, const UCapsuleHitRegistrator* Registrator, const FInstancedStruct& Payload)
	{
		++HitCount;
		LastActor = Hit.HitActor;
		LastPayload = Payload;
	}

	UFUNCTION()
	void OnComponentHit(const UDamageBehavior* Behavior, const FString Name, const FDBSHitRegistratorHitResult& Hit, const UCapsuleHitRegistrator* Registrator, const FInstancedStruct& Payload)
	{
		++HitCount;
		LastActor = Hit.HitActor;
		LastPayload = Payload;
	}

	UFUNCTION()
	void OnEnd(bool bHitAnything)
	{
		++EndCount;
		bLastEndHit = bHitAnything;
	}
};
