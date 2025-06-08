// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DamageBehaviorsSource.generated.h"

USTRUCT(BlueprintType)
struct FDamageBehaviorsSource
{
	GENERATED_BODY()

	FDamageBehaviorsSource() = default;
	FDamageBehaviorsSource(FString SourceName_In, AActor* Actor);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SourceName;
	
	UPROPERTY()
	AActor* Actor = nullptr;

	UPROPERTY()
	class UDamageBehaviorsComponent* DamageBehaviorsComponent = nullptr;

	bool operator==(const FDamageBehaviorsSource& Other) const
	{
		return SourceName == Other.SourceName;
	}
	bool operator==(const FString& Other) const
	{
		return SourceName == Other;
	}
};

// TODO: move to DamageBehaviorsSystem settings and instantiate only once
UCLASS(Blueprintable, BlueprintType)
class DAMAGEBEHAVIORSSYSTEM_API UDamageBehaviorsSourceEvaluator : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SourceName;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	AActor* GetActorWithCapsules(AActor* OwnerActor) const;
};