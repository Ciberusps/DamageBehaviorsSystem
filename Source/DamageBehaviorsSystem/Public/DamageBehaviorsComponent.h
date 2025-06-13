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
    UPROPERTY(BlueprintAssignable)
    FDamageBehaviorOnProcessedHit OnHitAnything;

	// EditDefaultsOnly used on purpose we don't want to have bugs related to
	// blueprint instances placed in world(on the map), unreal sometimes don't
	// reset values to defaults in blueprint even if you not changed them
	// so better to just give ONLY one way to use
	// DamageBehaviorsComponent - by modifing Blueprint Defaults and if you need runtime
	// DamageBehaviors creation - use DamageBehaviorsComponent api to add them
	// IF you want to override some DamageDescriptions or whatever
	// just
	// - create new Array with overrides
	// - or hide overriding under flag bOverrideDamageBehaviorsDefaults
	// - or create validation that checks that
	//	 Blueprint defaults not match in-world Instance defaults
	//
	// UPD 11.06.25 it was unreal serialization problem
	// I used same variable name for DamageBehaviors (it was TMap<TString, ...>)
	// so all instances of blueprints already had this property serialized ofc
	// and every time I changed DamageBehavior in blueprint
	// it ofc not synced to instances because they have this property
	// and its not null its contains TMap values thats why DamageBehaviors
	// never gets reseted/synced to blueprint defaults
	// DON't rename to DamageBehaviors
	//
	// UPD 13.06.25 - its unreal syncing issue with Instanced TArray/TSet
	// its also reproduces with MoverV2 epics plugin -> change instanced value in mover component
	// and it wont be synced on World Instances of this blueprint
	// FIX - for now its synced in DBSEditor reason why not on PostEditChangeProperty is
	// when we compile blueprint values gets reseted, tried PostCDOCompiled/PostThisAndThat,
	// they not even get called on ActorComponents so DBSEditor solution works - stopped on it
	// for now TODO: check if it still reproduces in UE5.6
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Instanced,
		Category="DamageBehaviorsComponent",
		DisplayName="DamageBehaviors",
		meta=(TitleProperty="Name", ShowOnlyInnerProperties)
	)
	TArray<TObjectPtr<UDamageBehavior>> DamageBehaviorsList;

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
    const TArray<UDamageBehavior*>& GetDamageBehaviors() const { return DamageBehaviorsList; };

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
	virtual void PostLoad() override;
	void SyncAllBehaviorSources();

	UPROPERTY()
	TArray<FDamageBehaviorsSource> DamageBehaviorsSources = {};
	
private:
    UPROPERTY()
    TObjectPtr<AActor> OwnerActor;

	UPROPERTY()
	TArray<TObjectPtr<UDamageBehaviorsSourceEvaluator>> DamageBehaviorsSourceEvaluators = {};

	UFUNCTION()
	void PrepareDamageBehaviorsSources();
	
    UFUNCTION()
    TMap<FString, UCapsuleHitRegistrator*> FindCapsuleHitRegistrators(AActor* Actor) const;

	UFUNCTION()
	void DefaultOnHitAnything(
		const FDBSHitRegistratorHitResult& HitRegistratorHitResult,
		const UDamageBehavior* DamageBehavior,
		const UCapsuleHitRegistrator* CapsuleHitRegistrator,
		const FInstancedStruct& Payload
	);
};
