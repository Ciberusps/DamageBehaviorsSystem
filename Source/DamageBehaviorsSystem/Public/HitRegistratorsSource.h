// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CapsuleHitRegistrator.h"
#include "HitRegistratorsSource.generated.h"


USTRUCT(BlueprintType)
struct FDBSHitRegistratorsSource
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SourceName;
	
	UPROPERTY()
	AActor* Actor = nullptr;

	UPROPERTY()
	TArray<UCapsuleHitRegistrator*> CapsuleHitRegistrators;
};

USTRUCT(BlueprintType)
struct FDBSHitRegistratorsToActivateSource
{
	GENERATED_BODY()

	FDBSHitRegistratorsToActivateSource() = default;
	FDBSHitRegistratorsToActivateSource(
		FString SourceName_In, TArray<FString> HitRegistratorsNames_In)
		: SourceName(SourceName_In), HitRegistratorsNames(HitRegistratorsNames_In) {};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SourceName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> HitRegistratorsNames = {};

	bool operator==(const FString& SourceName_In) const
	{
		return SourceName == SourceName_In;
	}
};
