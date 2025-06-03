// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CapsuleHitRegistrator.h"
#include "DamageBehavior.generated.h"

class UCapsuleHitRegistrator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageBehaviorHitRegistered,
    const FCapsuleHitRegistratorHitResult&, CapsuleHitRegistratorHitResult,
    const class UDamageBehavior*, DamageBehavior,
    const UCapsuleHitRegistrator*, CapsuleHitRegistrator,
    const TArray<FInstancedStruct>&, Payload
);

USTRUCT(BlueprintType)
struct FCapsuleHitRegistratorsSource
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
struct FSourceHitRegistratorsToActivate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SourceName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> HitRegistratorsNames = {};
};

// TODO: make instanced UObject
/**
 * DamageBehavior entity that handles all hits from dumb "CapsuleHitRegistrators"
 * and filter hitted objects by adding them in "HitActors".
 * When "InvokeDamageBehavior" ends, all "HitActors" cleanup
 */
UCLASS(BlueprintType, DefaultToInstanced, EditInlineNew, AutoExpandCategories = ("Default"))
class DAMAGEBEHAVIORSSYSTEM_API UDamageBehavior : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnDamageBehaviorHitRegistered OnHitRegistered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehaviorDescription", meta=(ShowOnlyInnerProperties))
	FDamageBehaviorHitDetectionSettings HitDetectionSettings;

    /**
     * @brief If enabled DamageBehaviorsComponent will DealDamage to enemy automatically
     * Enable it only on AI enemies or in cases when DamageBehavior on Character(like DmgBeh_Roll, DmgBeh_Backstep)...
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAutoHandleDamage = false;

	// used for attack GameplayAbilities mostly
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bInvokeDamageBehaviorOnStart = false;

	// используется для атак где враг должен нас "протащить"
	// пример - "укус" у Дракона, который knockdown'ит игрока и
    // отталкивают игрока на протяжении N кол-ва секунд, пока атака не закончится
	// пример - атака "таран" где враг бежит и может нас насадить на меч на какое-то время
	// пример - атака "дракон дешится" при этом крыло находится низко к земле, на крыле должен
	// протащить несколько секунд
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAttachEnemiesToCapsuleWhileActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Comment = FString("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSourceHitRegistratorsToActivate> SourceHitRegistratorsToActivate = {
		{ DEFAULT_DAMAGE_BEHAVIOR_SOURCE }
	};
	
    UPROPERTY()
    TArray<FCapsuleHitRegistratorsSource> CapsuleHitRegistratorsSources = {};

	// TODO now is instanced uobject Init not required, probably constructor required - refactor
    void Init(
    	AActor* Owner_In,
    	const TArray<FCapsuleHitRegistratorsSource>& CapsuleHitRegistratorsSources_In = {}
    );

	UFUNCTION(BlueprintNativeEvent)
    void MakeActive(bool bShouldActivate, const TArray<FInstancedStruct>& Payload);
    // void MakeActive(bool bShouldActivate, EPhysDamageType OverridePhysDamageType_In);

	// make your checks for IHittableInterface or whatever you check that actor is hittable 
	UFUNCTION(BlueprintNativeEvent)
	bool CanGetHit(const FCapsuleHitRegistratorHitResult& CapsuleHitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator);
	
	// Result - is hit should be registered
	UFUNCTION(BlueprintNativeEvent)
	bool HandleHit(const FCapsuleHitRegistratorHitResult& CapsuleHitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator, TArray<FInstancedStruct>& Payload_Out);

	UFUNCTION(BlueprintNativeEvent)
    void AddHittedActor(AActor* Actor_In, bool bCanBeAttached, bool bAddAttachedActorsToActorAlso);

	UFUNCTION(BlueprintNativeEvent)
    void ClearHittedActors();

	// TODO: probably not required at all after refactoring
	TArray<UCapsuleHitRegistrator*> GetCapsuleHitRegistratorsFromAllSources() const;
	
private:
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> HitActors = {};
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> AttachedActors = {}; 
    UPROPERTY()
    bool bIsActive = false;
	UPROPERTY()
	TWeakObjectPtr<UUHLDebugSystemSubsystem> DebugSubsystem; 
    TWeakObjectPtr<AActor> OwnerActor = nullptr;

	UFUNCTION()
    void ProcessHit(const FCapsuleHitRegistratorHitResult& CapsuleHitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator);

	AActor* GetRootAttachedActor(AActor* Actor_In) const;
};
