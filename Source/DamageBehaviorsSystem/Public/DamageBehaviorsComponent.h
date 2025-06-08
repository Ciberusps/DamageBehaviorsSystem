// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageBehavior.h"
#include "CapsuleHitRegistrator.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"
#include "DamageBehaviorsComponent.generated.h"

class UCapsuleHitRegistrator;
class UCapsuleHitRegistratorUDamageBehavior;

DECLARE_LOG_CATEGORY_EXTERN(LogDamageBehaviorsSystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDamageBehaviorOnProcessedHit,
	const UDamageBehavior*, DamageBehavior,
	const FString, DamageBehaviorName,
	const FDBSHitRegistratorHitResult&, HitRegistratorHitResult
);

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
	AActor* GetActorWithCapsules(AActor* OwnerActor) const;
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

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Instanced,
		Category="DamageBehaviorsComponent",
		meta=(TitleProperty="Name", ShowOnlyInnerProperties)
	)
	TArray<UDamageBehavior*> DamageBehaviors;

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
		const FString DamageBehaviorName,
		const bool bShouldActivate,
		const TArray<FString>& DamageBehaviorsSourcesToUse,
		const FInstancedStruct& Payload
	);
    
    UFUNCTION(BlueprintCallable)
    const TArray<UDamageBehavior*>& GetDamageBehaviors() const { return DamageBehaviors; };

    UFUNCTION(BlueprintCallable)
    UDamageBehavior* GetDamageBehavior(const FString Name) const;

	// TODO: попытаться вернуть, но хз зач все это можно в DamageBehavior'ах ловить
    // for cases there ProcessHit is custom, e.g. BProjectileComponent, MeleeWeaponItem, ...
    // UFUNCTION(BlueprintCallable)
    // void DefaultOnHitAnything(const UDamageBehavior* DamageBehavior, const FGetHitResult& GetHitResult, const bool bSpawnFX = false);

	
	UFUNCTION()
	TArray<UAdditionalDamageBehaviorsSourceEvaluator*> SpawnEvaluators();
	
	UFUNCTION()
	TArray<UAdditionalDamageBehaviorsSourceEvaluator*> GetDamageBehaviorsSourceEvaluators() const { return DamageBehaviorsSourceEvaluators; };

	UFUNCTION()
	const TArray<FDBSHitRegistratorsSource> GetHitRegistratorsSources(TArray<UAdditionalDamageBehaviorsSourceEvaluator*> SourceEvaluators_In) const;
	
protected:
    virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
#endif
	void SyncAllBehaviorSources();

	UPROPERTY()
	TArray<FDamageBehaviorsSource> DamageBehaviorsSources = {};
	
private:
    UPROPERTY()
    TObjectPtr<AActor> OwnerActor;

	UPROPERTY()
	TArray<TObjectPtr<UAdditionalDamageBehaviorsSourceEvaluator>> DamageBehaviorsSourceEvaluators = {};

    UFUNCTION()
    TMap<FString, UCapsuleHitRegistrator*> FindCapsuleHitRegistrators(AActor* Actor) const;
};
