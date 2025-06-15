#include "AnimGraphNode_InvokeDamageBehavior.h"
#include "Animation/AnimInstance.h"
#include "AnimationGraphSchema.h"

#define LOCTEXT_NAMESPACE "AnimGraphNode_InvokeDamageBehavior"

UAnimGraphNode_InvokeDamageBehavior::UAnimGraphNode_InvokeDamageBehavior(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NotifyState = ObjectInitializer.CreateDefaultSubobject<UANS_InvokeDamageBehavior>(this, TEXT("NotifyState"));
}

void UAnimGraphNode_InvokeDamageBehavior::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const
{
    if (PreviewSkelMeshComp && PreviewSkelMeshComp->GetAnimInstance() && NotifyState)
    {
        NotifyState->ConditionalDebugDraw(PDI, PreviewSkelMeshComp);
    }
}

FText UAnimGraphNode_InvokeDamageBehavior::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return LOCTEXT("InvokeDamageBehavior", "Invoke Damage Behavior");
}

FText UAnimGraphNode_InvokeDamageBehavior::GetTooltipText() const
{
    return LOCTEXT("InvokeDamageBehaviorTooltip", "Activates DamageBehavior, window should be small");
}

FString UAnimGraphNode_InvokeDamageBehavior::GetNodeCategory() const
{
    return TEXT("DamageBehaviorsSystem");
}

FLinearColor UAnimGraphNode_InvokeDamageBehavior::GetNodeTitleColor() const
{
    return FLinearColor(1.0f, 0.491021f, 0.0f);
}   

#undef LOCTEXT_NAMESPACE 