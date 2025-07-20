// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CapsuleHitRegistrator.h"
#include "HitRegistratorsSource.h"
#include "StructUtils/InstancedStruct.h"
#include "DamageBehavior.generated.h"

class UCapsuleHitRegistrator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageBehaviorHitRegistered,
    const FDBSHitRegistratorHitResult&, HitRegistratorHitResult,
    const class UDamageBehavior*, DamageBehavior,
    const UCapsuleHitRegistrator*, CapsuleHitRegistrator,
    const FInstancedStruct&, Payload
);

/**
 * DamageBehavior entity that handles all hits from dumb "CapsuleHitRegistrators"
 * and filter hitted objects by adding them in "HitActors".
 * When "InvokeDamageBehavior" ends, all "HitActors" cleanup
 */
UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew, AutoExpandCategories = ("Default,DamageBehavior"), meta=(DisplayName=""))
class DAMAGEBEHAVIORSSYSTEM_API UDamageBehavior : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnDamageBehaviorHitRegistered OnHitRegistered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior")
	FString Name = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior", meta=(ShowOnlyInnerProperties))
	FDamageBehaviorHitDetectionSettings HitDetectionSettings;

    /**
     * @brief If enabled DamageBehaviorsComponent will DealDamage to enemy automatically
     * Enable it only on AI enemies or in cases when DamageBehavior on Character(like DmgBeh_Roll, DmgBeh_Backstep)...
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior")
    bool bAutoHandleDamage = false;

	// used for attack GameplayAbilities mostly
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior")
    bool bInvokeDamageBehaviorOnStart = false;

	// используется для атак где враг должен нас "протащить"
	// пример - "укус" у Дракона, который knockdown'ит игрока и
    // отталкивают игрока на протяжении N кол-ва секунд, пока атака не закончится
	// пример - атака "таран" где враг бежит и может нас насадить на меч на какое-то время
	// пример - атака "дракон дешится" при этом крыло находится низко к земле, на крыле должен
	// протащить несколько секунд
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior")
    bool bAttachEnemiesToCapsuleWhileActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior")
    FString Comment = FString("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior", DisplayName="HitRegistrators to Activate")
	TArray<FDBSHitRegistratorsToActivateSource> HitRegistratorsToActivateBySource = {
		{ DEFAULT_DAMAGE_BEHAVIOR_SOURCE, {} }
	};
	
    UPROPERTY()
    TArray<FDBSHitRegistratorsSource> HitRegistratorsSources = {};

	// TODO now is instanced uobject Init not required, probably constructor required - refactor
    void Init(
    	AActor* Owner_In,
    	const TArray<FDBSHitRegistratorsSource>& CapsuleHitRegistratorsSources_In = {}
    );

	UFUNCTION(BlueprintNativeEvent)
    void MakeActive(bool bShouldActivate, const FInstancedStruct& Payload);

	// by default HitTarget is most top actor in "attach" hierarchy
	// TODO: probably in GrabAttacks it might cause problems
	UFUNCTION(BlueprintNativeEvent)
	AActor* GetHitTarget(AActor* HitActor_In, const FDBSHitRegistratorHitResult& HitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator) const;

	// ex. CanGetHit
	// if can - added to hitted actors array and should be ignored by other capsules in DamageBehavior
	// make your checks for IHittableInterface or whatever you check that actor is hittable 
	UFUNCTION(BlueprintNativeEvent)
	bool CanBeAddedToHittedActors(const FDBSHitRegistratorHitResult& HitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator);

	// Main function for ProcessingHit, if you lasy use OnHitRegistered
	// Result - is hit should be registered
	UFUNCTION(BlueprintNativeEvent)
	bool ProcessHit(const FDBSHitRegistratorHitResult& HitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator, FInstancedStruct& Payload_Out);

	UFUNCTION(BlueprintNativeEvent)
    void AddHittedActor(AActor* Actor_In, bool bCanBeAttached, bool bAddAttachedActorsToActorAlso);

	UFUNCTION(BlueprintNativeEvent)
    void ClearHittedActors();

	UFUNCTION(BlueprintCallable)
	AActor* GetOwningActor() const { return OwnerActor.Get(); };

	// TODO: probably not required at all after refactoring
	TArray<UCapsuleHitRegistrator*> GetCapsuleHitRegistratorsFromAllSources() const;

	void SyncSourcesFromSettings();

	UFUNCTION()
	TArray<FString> GetHitRegistratorsNameOptions() const;

	UFUNCTION(BlueprintCallable)
	const FInstancedStruct& GetCurrentInvokePayload() const { return CurrentInvokePayload; };

	bool operator==(const FString& OtherName) const
	{
		return Name == OtherName;
	}
	bool operator==(const UDamageBehavior* Other) const
	{
		return Name == Other->Name;
	}

protected:
	UPROPERTY()
	FInstancedStruct CurrentInvokePayload = {};
	
#if WITH_EDITOR
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
private:
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> HitActors = {};
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> AttachedActors = {}; 
    UPROPERTY()
    bool bIsActive = false;
    TWeakObjectPtr<AActor> OwnerActor = nullptr;

	UFUNCTION()
    void HandleHitInternally(const FDBSHitRegistratorHitResult& HitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator);

	AActor* GetRootAttachedActor(AActor* Actor_In) const;

	// copy of UnrealHelperLibrary function
	TArray<FString> GetNamesOfComponentsOnObject(UObject* OwnerObject, UClass* Class) const;
};
