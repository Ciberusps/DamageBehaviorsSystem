// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DamageBehaviorsComponent.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_InvokeDamageBehavior.generated.h"

class UDamageBehaviorsComponent;

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
	virtual bool ShouldFireInEditor() { return false; }
#endif

	virtual FLinearColor GetEditorColor() override { return FLinearColor(1.0f, 0.491021f, 0.0f); };
	virtual FString GetNotifyName_Implementation() const override { return FString("InvokeDamageBehavior"); };

	// TODO: field of improvement can Invoke DamageBehaviorSource->DamageBehaviorComponent directly
	// for now its indirect through DamageBehaviorsComponent on MeshOwner
	// direct invoke requires to spawn DamageBehaviorSourceEvaluator
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
    FString Name = FString("DmgBehDefault");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	TMap<FString, bool> TargetSources;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	FInstancedStruct Payload;

private:
	TArray<FString> GetDamageBehaviorSourcesList() const;
};
