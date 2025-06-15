// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageBehaviorsComponent.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_InvokeDamageBehavior.generated.h"

class UDamageBehaviorsComponent;

USTRUCT()
struct FDBSInvokeDamageBehaviorDebugActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString SourceName;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AActor> Actor;
	UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle))
	bool bCustomSocketName = false;
	UPROPERTY(EditAnywhere, meta=(EditCondition="bCustomSocketName"))
	FName SocketName = "";
};

USTRUCT()
struct FDBSInvokeDamageBehaviorDebugForMesh
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere)
	TArray<FDBSInvokeDamageBehaviorDebugActor> DebugActors;

	bool operator==(const USkeletalMesh* Mesh_In) const
	{
		return Mesh.Get() == Mesh_In;
	}
};

// // TODO: this settings should be stored on "ThisActor" DamageBehavior
// // only on DebugActor should be here as reference to settings
// // probably its even better to use UDamageBehaviorSettings to not edit MeshComponents at all
// UCLASS()
// class UInvokeDamageBehaviorDebugActors : public UAssetUserData
// {
// 	GENERATED_BODY()
// public:
// 	// TODO: ActorsBySourceName - RightHandActor, LeftHandActor
// 	UPROPERTY(EditAnywhere, Category="SoftRefs")
// 	TArray<FDBSInvokeDamageBehaviorDebugForMesh> DebugActors;
// };

USTRUCT()
struct FDBSDebugHitRegistratorDescription
{
	GENERATED_BODY()

	UPROPERTY()
	FName SocketNameAttached = "";

	UPROPERTY()
	FVector Location;
	UPROPERTY()
	FRotator Rotation;
	
	UPROPERTY()
	float CapsuleRadius = 0.0f;
	UPROPERTY()
	float CapsuleHalfHeight = 0.0f;
};


/**
 *
 */
UCLASS(Blueprintable, BlueprintType, meta=(ToolTip="Activates DamageBehavior, window should be small"))
class DAMAGEBEHAVIORSSYSTEM_API UANS_InvokeDamageBehavior : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_InvokeDamageBehavior();
	
#if WITH_EDITOR
	/** Override this to prevent firing this notify state type in animation editors */
	virtual bool ShouldFireInEditor() { return true; }
#endif

	virtual FLinearColor GetEditorColor() override { return FLinearColor(1.0f, 0.491021f, 0.0f); };
	virtual FString GetNotifyName_Implementation() const override { return FString("InvokeDamageBehavior"); };

	// TODO: field of improvement can Invoke DamageBehaviorSource->DamageBehaviorComponent directly
	// for now its indirect through DamageBehaviorsComponent on MeshOwner
	// direct invoke requires to spawn DamageBehaviorSourceEvaluator
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
    FString Name = FString("DmgBehDefault");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	TMap<FString, bool> TargetSources;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	FInstancedStruct Payload;

private:
	UPROPERTY()
	TMap<FString, FDBSDebugHitRegistratorDescription> HitRegistratorsDescription = {};
	
	void DrawCapsules(UWorld* WorldContextObject, USkeletalMeshComponent* MeshComp);

	TArray<FString> GetDamageBehaviorSourcesList() const;
};
