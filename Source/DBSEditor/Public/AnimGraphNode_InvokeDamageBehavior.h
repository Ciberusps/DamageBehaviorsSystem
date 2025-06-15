#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "ANS_InvokeDamageBehavior.h"
#include "AnimGraphNode_InvokeDamageBehavior.generated.h"

class FPrimitiveDrawInterface;

UCLASS()
class DBSEDITOR_API UAnimGraphNode_InvokeDamageBehavior : public UAnimGraphNode_Base
{
    GENERATED_BODY()

public:
    UAnimGraphNode_InvokeDamageBehavior(const FObjectInitializer& ObjectInitializer);

    virtual void Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const override;

    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetTooltipText() const override;
    virtual FString GetNodeCategory() const override;
    virtual FLinearColor GetNodeTitleColor() const override;

    // The notify state instance that we're editing
    UPROPERTY(EditAnywhere, Category = "Settings")
    UANS_InvokeDamageBehavior* NotifyState;
}; 