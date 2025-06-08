// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageBehaviorsSource.h"
#include "DamageBehavior.h"
#include "CapsuleHitRegistrator.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"
#include "DamageBehaviorsComponent.generated.h"

class UCapsuleHitRegistrator;
class UCapsuleHitRegistratorUDamageBehavior;

DECLARE_LOG_CATEGORY_EXTERN(LogDamageBehaviorsSystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FDamageBehaviorOnProcessedHit,
	const UDamageBehavior*, DamageBehavior,
	const FString, DamageBehaviorName,
	const FDBSHitRegistratorHitResult&, HitRegistratorHitResult,
	const UCapsuleHitRegistrator*, CapsuleHitRegistrator,
	const FInstancedStruct&, Payload
);

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DAMAGEBEHAVIORSSYSTEM_API UDamageBehaviorsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    // TODO PhysMaterial like in MeleeWeaponItem
	// DEPRECATED use custom GodreaperDamageBehavior
    UPROPERTY(BlueprintAssignable)
    FDamageBehaviorOnProcessedHit OnHitAnything;

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
    // for cases there HandleHitInternally is custom, e.g. BProjectileComponent, MeleeWeaponItem, ...
    // UFUNCTION(BlueprintCallable)
    // void DefaultOnHitAnything(const UDamageBehavior* DamageBehavior, const FGetHitResult& GetHitResult, const bool bSpawnFX = false);

	
	UFUNCTION()
	TArray<UDamageBehaviorsSourceEvaluator*> SpawnEvaluators();
	
	UFUNCTION()
	TArray<UDamageBehaviorsSourceEvaluator*> GetDamageBehaviorsSourceEvaluators() const { return DamageBehaviorsSourceEvaluators; };

	UFUNCTION()
	const TArray<FDBSHitRegistratorsSource> GetHitRegistratorsSources(TArray<UDamageBehaviorsSourceEvaluator*> SourceEvaluators_In) const;
	
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
	TArray<TObjectPtr<UDamageBehaviorsSourceEvaluator>> DamageBehaviorsSourceEvaluators = {};

    UFUNCTION()
    TMap<FString, UCapsuleHitRegistrator*> FindCapsuleHitRegistrators(AActor* Actor) const;

	UFUNCTION()
	void DefaultOnHitAnything(
		const FDBSHitRegistratorHitResult& HitRegistratorHitResult,
		const class UDamageBehavior* DamageBehavior,
		const UCapsuleHitRegistrator* CapsuleHitRegistrator,
		// TODO: use one InstancedStruct, we can put in one struct another InstancedStructs
		// no need to use TArray
		const FInstancedStruct& Payload
	);
};
