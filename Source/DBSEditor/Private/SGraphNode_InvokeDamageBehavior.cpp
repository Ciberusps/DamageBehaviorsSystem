#include "SGraphNode_InvokeDamageBehavior.h"
#include "AnimGraphNode_InvokeDamageBehavior.h"
#include "EditorStyleSet.h"
#include "Widgets/SBoxPanel.h"
#include "SGraphPin.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "DBSEditor.h"
#include "EdGraphUtilities.h"
#include "DBSEditorDamageBehaviorDetails.h"

void SGraphNode_InvokeDamageBehavior::Construct(const FArguments& InArgs, UAnimGraphNode_InvokeDamageBehavior* InNode)
{
    Node = InNode;
    SetCursor(EMouseCursor::CardinalCross);
    UpdateGraphNode();
}

void SGraphNode_InvokeDamageBehavior::UpdateGraphNode()
{
    InputPins.Empty();
    OutputPins.Empty();

    // Reset variables that are going to be exposed, in case we are refreshing an already setup node.
    RightNodeBox.Reset();
    LeftNodeBox.Reset();

    // Create the widget
    this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
    this->GetOrAddSlot(ENodeZone::Center)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
            .Padding(0.0f)
            .BorderBackgroundColor(this, &SGraphNode_InvokeDamageBehavior::GetBorderColor)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        CreateTitleWidget()
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        CreateNodeContentArea()
                    ]
                ]
            ]
        ];

    CreatePinWidgets();
}

TSharedRef<SWidget> SGraphNode_InvokeDamageBehavior::CreateTitleWidget()
{
    // Create the standard title bar
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Center)
        .FillWidth(1.0f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("Graph.StateNode.ColorSpill"))
            .Padding(FMargin(10, 5, 30, 3))
            .BorderBackgroundColor(this, &SGraphNode_InvokeDamageBehavior::GetBorderColor)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Invoke Damage Behavior"))
                .TextStyle(FAppStyle::Get(), "Graph.StateNode.NodeTitle")
                .Justification(ETextJustify::Center)
            ]
        ];
}

TSharedRef<SWidget> SGraphNode_InvokeDamageBehavior::CreateNodeContentArea()
{
    // Create pin areas
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("NoBorder"))
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .HAlign(HAlign_Left)
            .VAlign(VAlign_Center)
            .Padding(10.0f, 0.0f)
            [
                SAssignNew(LeftNodeBox, SVerticalBox)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .HAlign(HAlign_Right)
            .VAlign(VAlign_Center)
            .Padding(10.0f, 0.0f)
            [
                SAssignNew(RightNodeBox, SVerticalBox)
            ]
        ];
}

FSlateColor SGraphNode_InvokeDamageBehavior::GetBorderColor() const
{
    // Get the default color from the node
    FLinearColor DefaultColor = FLinearColor(1.0f, 0.491021f, 0.0f); // Orange color matching ANS_InvokeDamageBehavior's GetEditorColor()

    // If the node is selected, brighten the color
    if (GraphNode && GraphNode->IsSelected())
    {
        DefaultColor = DefaultColor.LinearRGBToHSV();
        DefaultColor.G = FMath::Min(1.0f, DefaultColor.G + 0.1f); // Increase saturation
        DefaultColor.B = FMath::Min(1.0f, DefaultColor.B + 0.2f); // Increase brightness
        DefaultColor = DefaultColor.HSVToLinearRGB();
    }

    return FSlateColor(DefaultColor);
}