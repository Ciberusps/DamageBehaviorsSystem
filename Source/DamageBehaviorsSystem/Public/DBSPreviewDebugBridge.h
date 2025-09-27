// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ANS_InvokeDamageBehavior.h"
#include "UObject/WeakObjectPtr.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"

class USkeletalMeshComponent;

enum class EDBSPreviewDebugEvent : uint8
{
	Begin,
	Tick,
	End
};

struct FDBSPreviewDebugActorSpawnInfo
{
	FString SourceName;
	TSoftClassPtr<AActor> Actor;
	bool bCustomSocketName = false;
	FName SocketName = NAME_None;
};

struct FDBSPreviewDebugPayload
{
	TWeakObjectPtr<USkeletalMeshComponent> MeshComp;
	TMap<FString, FDBSDebugHitRegistratorDescription> HitRegistratorsDescription;
	TArray<FDBSPreviewDebugActorSpawnInfo> DebugActorsToSpawn;
	EDBSPreviewDebugEvent Event = EDBSPreviewDebugEvent::Tick;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FDBSOnPreviewDebug, const FDBSPreviewDebugPayload&);

// Global accessor to the preview debug delegate
DAMAGEBEHAVIORSSYSTEM_API FDBSOnPreviewDebug& DBS_GetOnPreviewDebugDelegate();



