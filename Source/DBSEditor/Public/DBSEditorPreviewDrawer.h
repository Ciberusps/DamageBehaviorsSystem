// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ANS_InvokeDamageBehavior.h"
#include "Tickable.h"

class USkeletalMeshComponent;
struct FDBSPreviewDebugPayload;

class FDBSEditorPreviewDrawer : public FTickableGameObject
{
public:
	FDBSEditorPreviewDrawer();
	virtual ~FDBSEditorPreviewDrawer();

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }

private:
	void OnPreviewDebug(const FDBSPreviewDebugPayload& Payload);

private:
	// Last received data per mesh
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TMap<FString, FDBSDebugHitRegistratorDescription>> LastDescriptions;
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, bool> bActiveForMesh;

	// Spawned debug actors per mesh and source name
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TMap<FString, TWeakObjectPtr<AActor>>> SpawnedActors;
};



