// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageBehaviorsSystemTypes.h"
#include "Components/CapsuleComponent.h"
#include "CapsuleHitRegistrator.generated.h"

USTRUCT(BlueprintType)
struct FDBSHitRegistratorHitResult
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> HitActor = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitResult HitResult;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Direction = FVector::ZeroVector;
	// Default instigator is Character that owns CapsuleHitRegistrator,
	// but don't forget to override it if needed
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> Instigator = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPhysicalSurface> PhysicalSurfaceType = EPhysicalSurface::SurfaceType_Default;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitRegistered,
	const FDBSHitRegistratorHitResult&, HitRegistratorHitResult,
	class UCapsuleHitRegistrator*, CapsuleHitRegistrator
);

/**
 *
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DAMAGEBEHAVIORSSYSTEM_API UCapsuleHitRegistrator : public UCapsuleComponent
{
	GENERATED_BODY()

public:
    UCapsuleHitRegistrator();

	UPROPERTY(BlueprintAssignable)
    FOnHitRegistered OnHitRegistered;

    UFUNCTION(BlueprintCallable)
    void SetIsHitRegistrationEnabled(bool bIsEnabled_In, FDamageBehaviorHitDetectionSettings HitDetectionSettings);
    // probably with multitrace its not required to ignore some actors
    UFUNCTION(BlueprintCallable)
    void AddActorsToIgnoreList(const TArray<AActor*>& Actors_In);

	UFUNCTION(BlueprintCallable)
	float GetLineThickness() const { return LineThickness; };
	
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee hit registration")
    bool bIsHitRegistrationEnabled = false;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    FVector PreviousComponentLocation = FVector::ZeroVector;
	FDamageBehaviorHitDetectionSettings CurrentHitDetectionSettings;
    UPROPERTY()
    TArray<AActor*> IgnoredActors;

	void ProcessHitRegistration();
	UFUNCTION()
	void OnBegingOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnDebugCategoryChanged(IConsoleVariable* Var);
	void UpdateCapsuleVisibility(bool bIsVisible_In);

	// full copy of UnrealHelperLibrary TraceUtils function
	static bool SweepCapsuleMultiByChannel(const UWorld* World, TArray<FHitResult>& OutHits, const FVector& Start,
		const FVector& End, float Radius, float HalfHeight, const FQuat& Rot,
		ECollisionChannel TraceChannel, const FCollisionQueryParams& Params,
		const FCollisionResponseParams& ResponseParam, bool bDrawDebug = false,
		float DrawTime = -1.0f, FColor TraceColor = FColor::Black,
		FColor HitColor = FColor::Red, float FailDrawTime = -1.0f);
};
