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
	FDamageBehaviorsSource(FString SourceName_In, AActor* OwnerActor_In, class UDamageBehaviorsSourceEvaluator* Evaluator_In);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SourceName;

	AActor* GetSourceActor() const;

	class UDamageBehaviorsComponent* GetDamageBehaviorsComponent() const;

	UPROPERTY()
	class UDamageBehaviorsSourceEvaluator* Evaluator = nullptr;

	bool operator==(const FDamageBehaviorsSource& Other) const
	{
		return SourceName == Other.SourceName;
	}
	bool operator==(const FString& Other) const
	{
		return SourceName == Other;
	}

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> OwnerActor;
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
	AActor* GetActorWithDamageBehaviors(AActor* OwnerActor) const;
};