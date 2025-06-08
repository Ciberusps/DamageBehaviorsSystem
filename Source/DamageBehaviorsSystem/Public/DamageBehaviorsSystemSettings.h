// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageBehaviorsComponent.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "DamageBehaviorsSystemSettings.generated.h"

/**
 *
 */
UCLASS(config="Default", defaultconfig)
class DAMAGEBEHAVIORSSYSTEM_API UDamageBehaviorsSystemSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category="DamageBehaviorsSystemSettings")
	TArray<TSubclassOf<UDamageBehaviorsSourceEvaluator>> AdditionalDamageBehaviorsSourcesEvaluators = {};

	UPROPERTY(config, EditAnywhere, Category="DamageBehaviorsSystemSettings", meta=(AllowAbstract))  
	TSoftClassPtr<UInterface> HittableInterfaceRef;

	UPROPERTY(config, EditAnywhere, Category="Trace|Collision")
	TEnumAsByte<ECollisionChannel> HitRegistratorsTraceChannel;
	
protected:
	//~UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface
};