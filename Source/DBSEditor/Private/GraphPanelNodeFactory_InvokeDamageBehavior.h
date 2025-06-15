#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"
#include "AnimGraphNode_InvokeDamageBehavior.h"
#include "SGraphNode_InvokeDamageBehavior.h"

class FGraphPanelNodeFactory_InvokeDamageBehavior : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override
	{
		if (UAnimGraphNode_InvokeDamageBehavior* InvokeDamageBehaviorNode = Cast<UAnimGraphNode_InvokeDamageBehavior>(Node))
		{
			return SNew(SGraphNode_InvokeDamageBehavior, InvokeDamageBehaviorNode);
		}
		return nullptr;
	}
}; 