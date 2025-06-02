// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageBehavior.h"
#include "CapsuleHitRegistrator.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"
#include "DamageBehaviorsComponent.generated.h"

class ABPlayerCharacter;
class UCapsuleHitRegistrator;
class ABBaseEnemy;
class ABBaseCharacter;
class UCapsuleHitRegistratorUDamageBehavior;

DECLARE_LOG_CATEGORY_EXTERN(LogDamageBehaviorsSystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDamageBehaviorOnProcessedHit,
	const UDamageBehavior*, DamageBehavior,
	const FString, DamageBehaviorName,
	const FCapsuleHitRegistratorHitResult&, CapsuleHitRegistratorHitResult
);

const FString DEFAULT_DAMAGE_BEHAVIOR_SOURCE = FString(TEXT("OwnerActor"));

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
class DAMAGEBEHAVIORSSYSTEM_API UAdditionalDamageBehaviorsSourceEvaluator : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SourceName;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	AActor* GetActorWithCapsules() const;
};

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DAMAGEBEHAVIORSSYSTEM_API UDamageBehaviorsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    // TODO PhysMaterial like in MeleeWeaponItem
	// DEPRECATED use custom GodreaperDamageBehavior
    UPROPERTY(BlueprintAssignable)
    FDamageBehaviorOnProcessedHit OnHitAnything;
	// DEPRECATED use custom GodreaperDamageBehavior
    UPROPERTY(BlueprintAssignable)
    FDamageBehaviorOnProcessedHit OnHitCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta=(TitleProperty="Description.Name"))
	TArray<UDamageBehavior*> DamageBehaviorsInstancedTest;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    AActor* GetOwningActor() const;
    virtual AActor* GetOwningActor_Implementation() const;

	// DamageBehaviorsSourceToUse - is source actor on which we will activate DamageBehavior
	// its required when we want to invoke attack for example on shield in LeftHand
	// in such case we should set "DamageBehaviorsSourceToUse" = "LeftHandWeapon"
	// by default all attacks use Character as source of DamageBehaviors it fits only for
	// attacks by body parts (hands/footstomps and so on)
    UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm="DamageBehaviorsSourcesToUse,Payload"))
	void InvokeDamageBehavior(
		FString DamageBehaviorName,
		bool bShouldActivate,
		const TArray<FString>& DamageBehaviorsSourcesToUse,
		const TArray<FInstancedStruct>& Payload
	);
    UFUNCTION()
	TArray<FString> GetHitRegistratorsNameOptions() const;
    UFUNCTION(BlueprintCallable)
    const TMap<FString, UDamageBehavior*>& GetDamageBehaviors() const { return DamageBehaviors; };
    UFUNCTION(BlueprintCallable)
    UDamageBehavior* GetDamageBehavior(FString Name) const;

	// TODO: попытаться вернуть, но хз зач все это можно в DamageBehavior'ах ловить
    // for cases there ProcessHit is custom, e.g. BProjectileComponent, MeleeWeaponItem, ...
    // UFUNCTION(BlueprintCallable)
    // void DefaultOnHitAnything(const UDamageBehavior* DamageBehavior, const FGetHitResult& GetHitResult, const bool bSpawnFX = false);

protected:
    virtual void BeginPlay() override;

	UPROPERTY()
	TArray<FDamageBehaviorsSource> DamageBehaviorsSources = {};
	
	UPROPERTY()
	TMap<FString, UDamageBehavior*> DamageBehaviors = {};
	
private:
    UPROPERTY()
    TObjectPtr<AActor> OwnerActor;

    UFUNCTION()
    TMap<FString, UCapsuleHitRegistrator*> FindCapsuleHitRegistrators(AActor* Actor) const;
};
