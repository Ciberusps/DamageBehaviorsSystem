#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UAnimGraphNode_InvokeDamageBehavior;

class SGraphNode_InvokeDamageBehavior : public SGraphNode
{
public:
    SLATE_BEGIN_ARGS(SGraphNode_InvokeDamageBehavior) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UAnimGraphNode_InvokeDamageBehavior* InNode);

    // SGraphNode interface
    virtual void UpdateGraphNode() override;
    // End of SGraphNode interface

protected:
    UAnimGraphNode_InvokeDamageBehavior* Node;

    /** Gets the color of this node's border */
    virtual FSlateColor GetBorderColor() const;

    /** Creates the title area of the node */
    virtual TSharedRef<SWidget> CreateTitleWidget();

    /** Creates the content area of the node */
    virtual TSharedRef<SWidget> CreateNodeContentArea();
}; 