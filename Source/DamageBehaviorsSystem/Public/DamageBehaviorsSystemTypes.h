// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageBehaviorsSystemTypes.generated.h"

UENUM(BlueprintType)
enum class EDamageBehaviorHitDetectionType : uint8
{
	ByTrace,
	ByEntering UMETA(DisplayName = "By Entering (Capsule)"),
	// TODO for "ticking" DamageBehaviors
	// WhileStandingInside,
};

USTRUCT(BlueprintType)
struct FDamageBehaviorHitDetectionSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	EDamageBehaviorHitDetectionType HitDetectionType = EDamageBehaviorHitDetectionType::ByTrace;
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "HitDetectionType == EDamageBehaviorHitDetectionType::ByEntering"))
	bool bCheckOverlappingActorsOnStart = true;
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "HitDetectionType == EDamageBehaviorHitDetectionType::ByEntering"))
	FCollisionProfileName CollisionProfileName = FCollisionProfileName(FName("VolumeHitRegistrator"));

	// call GetHit every time "WhileStandingInside"
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "HitDetectionType == EDamageBehaviorHitDetectionType::WhileStandingInside"))
	// bool bUseStacks = true;
};