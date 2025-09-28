// Pavel Penkov 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
	void SpawnDebugActorsForMesh(USkeletalMesh* Mesh);
};
