// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CapsuleHitRegistrator.h"
#include "GameplayTagContainer.h"
#include "HitRegistratorsSource.h"
#include "StructUtils/InstancedStruct.h"
#include "Tickable.h"
#include "DamageBehavior.generated.h"

class UCapsuleHitRegistrator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageBehaviorHitRegistered,
    const FDBSHitRegistratorHitResult&, HitRegistratorHitResult,
    const class UDamageBehavior*, DamageBehavior,
    const UCapsuleHitRegistrator*, CapsuleHitRegistrator,
    const FInstancedStruct&, Payload
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvokeEnd, bool, bHitAnything);

USTRUCT(BlueprintType)
struct FDBSNoiseEventOnDamageSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Loudness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxRange = 10 * 100; // 10m

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Tag = NAME_None;
};

/**
 * DamageBehavior entity that handles all hits from dumb "CapsuleHitRegistrators"
 * and filter hitted objects by adding them in "HitActors".
 * When "InvokeDamageBehavior" ends, all "HitActors" cleanup
 */
UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew, AutoExpandCategories = ("Default,DamageBehavior"), meta=(DisplayName=""))
class DAMAGEBEHAVIORSSYSTEM_API UDamageBehavior : public UObject, public FTickableGameObject
{
    GENERATED_BODY()

public:
	UDamageBehavior();

    UPROPERTY(BlueprintAssignable)
    FOnDamageBehaviorHitRegistered OnHitRegistered;

	UPROPERTY(BlueprintAssignable)
	FOnInvokeEnd OnInvokeEnd;

	// Name was DEPRECATED use "Tags"
	// Description должен совпадать с названием атаки в дереве поведения для удобства 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior", DisplayName="Description (Name DEPRECATED use Tags)")
	FString Description = "";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior", meta=(Categories = "DamageBehaviors"))
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior", meta=(ShowOnlyInnerProperties))
	FDamageBehaviorHitDetectionSettings HitDetectionSettings;

    /**
     * @brief If enabled DamageBehaviorsComponent will DealDamage to enemy automatically
     * Enable it only on AI enemies or in cases when DamageBehavior on Character(like DmgBeh_Roll, DmgBeh_Backstep)...
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamageBehavior")
    bool bAutoHandleDamage = false;

	//If true checks obstacles between enemy and hit capsule and if there are any - enemy won't be hit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageBehavior")
	bool bMakeValidationTrace = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
	bool bReportNoiseEventOnHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bReportNoiseEventOnHit"))
	FDBSNoiseEventOnDamageSettings NoiseEventOnHitSettings;

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
	bool CanMakeValidationTrace(const FDBSHitRegistratorHitResult& HitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator);
	
	UFUNCTION(BlueprintNativeEvent)
	bool MakeValidationTrace(const FDBSHitRegistratorHitResult& HitRegistratorHitResult, UCapsuleHitRegistrator* CapsuleHitRegistrator);

	UFUNCTION(BlueprintNativeEvent)
    void ClearHittedActors();

	UFUNCTION(BlueprintNativeEvent)
	AActor* GetInstigator() const;

	UFUNCTION(BlueprintCallable)
	AActor* GetOwningActor() const { return OwnerActor.Get(); };

	// TODO: probably not required at all after refactoring
	TArray<UCapsuleHitRegistrator*> GetCapsuleHitRegistratorsFromAllSources() const;

	void SyncSourcesFromSettings();

	UFUNCTION()
	TArray<FString> GetHitRegistratorsNameOptions() const;

	UFUNCTION(BlueprintCallable)
	const FInstancedStruct& GetCurrentInvokePayload() const { return CurrentInvokePayload; };

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; };
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDamageBehavior, STATGROUP_Tickables); };

	bool operator==(const FString& OtherName) const
	{
		return Description == OtherName;
	}
	bool operator==(const UDamageBehavior* Other) const
	{
		if (!Other)
		{
			return false;
		}

		if (Tags.HasAnyExact(Other->Tags))
		{
			return true;
		}

		return Description == Other->Description;
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
	void ApplyLegacyMigration();

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
