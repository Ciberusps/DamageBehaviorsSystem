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
#if WITH_EDITOR
	/** Override this to prevent firing this notify state type in animation editors */
	virtual bool ShouldFireInEditor() { return false; }
#endif

	virtual FLinearColor GetEditorColor() override { return FLinearColor(1.0f, 0.491021f, 0.0f); };
	virtual FString GetNotifyName_Implementation() const override { return FString("InvokeDamageBehavior"); };

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	// EDamageBehavior DamageBehaviorName = EDamageBehavior::DmgBehDefault;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
    FString DamageBehaviorName = FString("DmgBehDefault");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	TArray<FString> DamageBehaviorSource = { DEFAULT_DAMAGE_BEHAVIOR_SOURCE };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	TArray<FInstancedStruct> Payload;
};
