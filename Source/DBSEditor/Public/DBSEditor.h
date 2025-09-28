// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DBSPreviewDebugBridge.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class UAnimMontage;
class USkeletalMesh;

class FDBSEditorModule final : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);

private:
	void HandleAssetEditorOpened(UObject* Asset);
	void HandleAssetEditorClosed(UObject* Asset, IAssetEditorInstance* AEI);
	void HandleAssetOpendInEditor(UObject* Asset, IAssetEditorInstance* AEI);
	void ClearDebugActorsForMesh(USkeletalMesh* Mesh);
	void SpawnDebugActorsForMesh(USkeletalMesh* Mesh, USkeletalMeshComponent* PreferredComp = nullptr);
	void RespawnDebugActorsForMeshDeferred(USkeletalMesh* Mesh);
	void RespawnForAssetDeferred(UObject* Asset);
	TArray<FDBSPreviewDebugActorSpawnInfo> CollectSpawnInfosForMesh(USkeletalMesh* Mesh) const;

private:
	TMap<TWeakObjectPtr<USkeletalMesh>, int32> PendingRespawnAttempts;
	TMap<TWeakObjectPtr<UObject>, int32> PendingAssetOpenAttempts;
};
