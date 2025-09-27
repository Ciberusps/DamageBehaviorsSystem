// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ANS_InvokeDamageBehavior.h"
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
	TEnumAsByte<ECollisionChannel> HitRegistratorsTraceChannel;
	
	UPROPERTY(config, EditAnywhere, Category="DamageBehaviorsSystemSettings")
	TArray<TSubclassOf<UDamageBehaviorsSourceEvaluator>> DamageBehaviorsSourcesEvaluators = {};

	// TODO: ActorsBySourceName - RightHandActor, LeftHandActor
	UPROPERTY(config, EditAnywhere, Category="DamageBehaviorsSystemSettings")
	TArray<FDBSInvokeDamageBehaviorDebugForMesh> DebugActors = {};

	// Can't work we always need blueprint with DamageBehaviors to take DamageBehaviors for ThisActor
	// Works only when enemy don't have weapons and we need to display capsules from default weapons
	// Fallback debug mesh used when no specific debug actors are found for a mesh
	UPROPERTY(config, EditAnywhere, Category="DamageBehaviorsSystemSettings", meta=(DisplayName="Fallback Debug Mesh"))
	FDBSInvokeDamageBehaviorDebugForMesh FallbackDebugMesh;

    // Class filter for selecting DebugActor classes in all pickers
    UPROPERTY(config, EditAnywhere, Category="Debug")
    TSoftClassPtr<AActor> DebugActorFilterBaseClass;

    // Editor debug drawing controls
    UPROPERTY(config, EditAnywhere, Category="Debug")
    bool bEnableEditorDebugDraw = true;

    // If true, keep last drawn state when montage preview is paused
    UPROPERTY(config, EditAnywhere, Category="Debug")
    bool bPreserveDrawOnPause = true;

    // If true, spawn selected DebugActors in the preview world
    UPROPERTY(config, EditAnywhere, Category="Debug")
    bool bSpawnDebugActorsInPreview = false;

    // If true and a socket is provided on DebugActor, attach actor to that socket
    UPROPERTY(config, EditAnywhere, Category="Debug")
    bool bAttachDebugActorsToSockets = true;
	
protected:
	//~UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface
};