// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/SOSEGenericGraphEdNode.h"
#include "OSEGenericGraphEditorPCH.h"
#include "GenericGraphAssetEditor/OSEGenericGraphColors.h"
#include "SLevelOfDetailBranchNode.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SCommentBubble.h"
#include "SlateOptMacros.h"
#include "SGraphPin.h"
#include "GraphEditorSettings.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"
#include "GenericGraphAssetEditor/OSEGenericGraphDragConnection.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraphEdNode"

//////////////////////////////////////////////////////////////////////////
class SGenericGraphPin : public SGraphPin
{
public:
   SLATE_BEGIN_ARGS(SGenericGraphPin) {}
   SLATE_END_ARGS()

   void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
   {
      this->SetCursor(EMouseCursor::Default);

      bShowLabel = true;

      GraphPinObj = InPin;
      check(GraphPinObj != nullptr);

      const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
      check(Schema);

      SBorder::Construct(SBorder::FArguments()
         .BorderBackgroundColor(this, &SGenericGraphPin::GetPinColor)
         .OnMouseButtonDown(this, &SGenericGraphPin::OnPinMouseDown)
         .Cursor(this, &SGenericGraphPin::GetPinCursor)
         .Padding(FMargin(5.0f))
      );
   }

protected:
   virtual FSlateColor GetPinColor() const override
   {
      return OSEGenericGraphColors::Pin::Default;
   }

   virtual TSharedRef<SWidget> GetDefaultValueWidget() override
   {
      return SNew(STextBlock);
   }

   virtual TSharedRef<FDragDropOperation> SpawnPinDragEvent(const TSharedRef<class SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphPin> >& InStartingPins) override
   {
      FOSEGenericGraphDragConnection::FDraggedPinTable PinHandles;
      PinHandles.Reserve(InStartingPins.Num());
      // since the graph can be refreshed and pins can be reconstructed/replaced
      // behind the scenes, the DragDropOperation holds onto FGraphPinHandles
      // instead of direct widgets/graph-pins
      for (const TSharedRef<SGraphPin>& PinWidget : InStartingPins)
      {
         PinHandles.Add(PinWidget->GetPinObj());
      }

      return FOSEGenericGraphDragConnection::New(InGraphPanel, PinHandles);
   }

};


//////////////////////////////////////////////////////////////////////////
void SOSEGenericGraphEdNode::Construct(const FArguments& InArgs, UOSEGenericGraphEdNode* InNode)
{
   GraphNode = InNode;
   check(GraphNode != nullptr);
   UpdateGraphNode();
   InNode->SEdNode = this;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SOSEGenericGraphEdNode::UpdateGraphNode()
{
   const FMargin NodePadding = FMargin(5);
   const FMargin NamePadding = FMargin(2);

   InputPins.Empty();
   OutputPins.Empty();

   // Reset variables that are going to be exposed, in case we are refreshing an already setup node.
   RightNodeBox.Reset();
   LeftNodeBox.Reset();

   // If for some reason we don't have valid node data, use an ensure and display an "INVALID NODE" widget rather than crash.
   auto constructFallbackInvalidNodeWidget = [&]()
   {
      this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
      this->GetOrAddSlot(ENodeZone::Center)
         .HAlign(HAlign_Fill)
         .VAlign(VAlign_Center)
         [
            SNew(SBorder)
            .Padding(NodePadding)
            [
               SNew(STextBlock)
               .Text(INVTEXT("INVALID NODE"))
            ]
         ];
   };

   if (!ensure(GraphNode != nullptr))
   {
      constructFallbackInvalidNodeWidget();
      return;
   }
   if (!ensure(GraphNode->IsA<UOSEGenericGraphEdNode>()))
   {
      constructFallbackInvalidNodeWidget();
      return;
   }
   if (!ensure(CastChecked<UOSEGenericGraphEdNode>(GraphNode)->GenericGraphNode != nullptr))
   {
      constructFallbackInvalidNodeWidget();
      return;
   }

   UOSEGenericGraphEdNode* Node = GetGenericGraphNodeChecked();
   const FSlateBrush* NodeTypeIcon = Node->GetNodeIcon();

   FLinearColor TitleShadowColor(0.6f, 0.6f, 0.6f);
   TSharedPtr<SErrorText> ErrorText;
   TSharedPtr<SVerticalBox> NodeBody;

   this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
   this->GetOrAddSlot(ENodeZone::Center)
      .HAlign(HAlign_Fill)
      .VAlign(VAlign_Center)
      [
         SNew(SBorder)
         .ToolTipText(this, &SOSEGenericGraphEdNode::GetToolTipText)
         .BorderImage_Lambda([this]()
         {
            return StyleToNodeBodyBrush(GetGenericGraphNodeChecked()->GenericGraphNode->GetNodeStyle());
         })
         .Padding(0.0f)
         .BorderBackgroundColor(this, &SOSEGenericGraphEdNode::GetBorderBackgroundColor)
         [
            SNew(SOverlay)

            +SOverlay::Slot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            [
               SNew(SVerticalBox)

               // Input Pin Area
               +SVerticalBox::Slot()
               .FillHeight(1)
               [
                  SAssignNew(LeftNodeBox, SVerticalBox)
               ]

               // Output Pin Area
               +SVerticalBox::Slot()
               .FillHeight(1)
               [
                  SAssignNew(RightNodeBox, SVerticalBox)
               ]
            ]

            +SOverlay::Slot()
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(8.0f)
            [
               SNew(SBorder)
               .BorderImage_Lambda([this]()
               {
                  return StyleToNodeColorSpillBrush(GetGenericGraphNodeChecked()->GenericGraphNode->GetNodeStyle());
               })
               .BorderBackgroundColor(this, &SOSEGenericGraphEdNode::GetBackgroundColor)
               .HAlign(HAlign_Center)
               .VAlign(VAlign_Center)
               .Visibility(EVisibility::SelfHitTestInvisible)
               .Padding(6.0f)
               [
                  SAssignNew(NodeBody, SVerticalBox)
                  .Visibility(EVisibility::Visible)

                  // Title
                  +SVerticalBox::Slot()
                  .AutoHeight()
                  [
                     SNew(SHorizontalBox)

                     // Error message
                     +SHorizontalBox::Slot()
                     .AutoWidth()
                     [
                        SAssignNew(ErrorText, SErrorText)
                        .BackgroundColor(this, &SOSEGenericGraphEdNode::GetErrorColor)
                        .ToolTipText(this, &SOSEGenericGraphEdNode::GetErrorMsgToolTip)
                     ]

                     // Icon
                     +SHorizontalBox::Slot()
                     .AutoWidth()
                     .VAlign(VAlign_Center)
                     [
                        SNew(SImage)
                        .Image(NodeTypeIcon)
                     ]

                     // Node Title
                     +SHorizontalBox::Slot()
                     .Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
                     [
                        SNew(SVerticalBox)
                        +SVerticalBox::Slot()
                        .AutoHeight()
                        [
                           SAssignNew(InlineEditableText, SInlineEditableTextBlock)
                           .Style(FAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
                           .Text(this, &SOSEGenericGraphEdNode::GetNodeEditableTitleText)
                           .OnVerifyTextChanged(this, &SOSEGenericGraphEdNode::OnVerifyNameTextChanged)
                           .OnTextCommitted(this, &SOSEGenericGraphEdNode::OnNodeEditableTitleTextCommited)
                           .IsReadOnly(this, &SOSEGenericGraphEdNode::IsNameReadOnly)
                           .IsSelected(this, &SOSEGenericGraphEdNode::IsSelectedExclusively)
                        ]
                        +SVerticalBox::Slot()
                        .AutoHeight()
                        [
                           SNew(SNodeTitle, GraphNode)
                        ]
                     ]
                  ]
                  +SVerticalBox::Slot()
                  .AutoHeight()
                  [
                     SNew(SBox)
                     .HAlign(HAlign_Fill)
                     .MaxDesiredWidth(1000.0f)
                     .Visibility_Lambda([this]() -> EVisibility
                     {
                        return NodeCustomBodyWidget ? EVisibility::Visible : EVisibility::Collapsed;
                     })
                     .Padding(FMargin(0.0, 4.0, 0.0, 0.0))
                     [
                        SAssignNew(NodeCustomBodyParent, SBox)
                        .HAlign(HAlign_Fill)
                     ]
                  ]
               ]
            ]
            +SOverlay::Slot()
            [
               SNew(SImage)
               .Image_Lambda([this]()
               {
                  return StyleToNodeGlossBrush(GetGenericGraphNodeChecked()->GenericGraphNode->GetNodeStyle());
               })
               .ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.66f))
               .Visibility(EVisibility::HitTestInvisible)
            ]
         ]
      ];

   static const FVector2D DefaultNodeIndicatorSize = FVector2D(40, 40);

   this->GetOrAddSlot(ENodeZone::TopLeft)
      .HAlign(HAlign_Fill)
      .VAlign(VAlign_Center)
      .SlotSize_Lambda([this]() -> FVector2D
      {
         UOSEGenericGraphNode* node = GetGenericGraphNodeChecked()->GenericGraphNode;
         return (node != nullptr) ? node->GetNodeIndicatorSize() : DefaultNodeIndicatorSize;
      })
      .SlotOffset_Lambda([this]() -> FVector2D
      {
         UOSEGenericGraphNode* node = GetGenericGraphNodeChecked()->GenericGraphNode;
         const FVector2D indicatorSize = (node != nullptr) ? node->GetNodeIndicatorSize() : DefaultNodeIndicatorSize;
         return indicatorSize * FVector2D(-1, 0);
      })
      [
         SNew(SBorder)
         .Padding(0.0f)
         .Visibility_Lambda([this]() -> EVisibility
         {
            UOSEGenericGraphNode* node = GetGenericGraphNodeChecked()->GenericGraphNode;
            const bool visible = (node != nullptr) ? node->GetNodeIndicatorVisible() : false;
            return visible ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
         })
         .BorderImage_Lambda([this]()
         {
            return StyleToNodeBodyBrush(GetGenericGraphNodeChecked()->GenericGraphNode->GetNodeStyle());
         })
         .BorderBackgroundColor_Lambda([this]()
         {
            UOSEGenericGraphNode* node = GetGenericGraphNodeChecked()->GenericGraphNode;
            return (node != nullptr) ? FSlateColor(node->GetNodeIndicatorBackgroundColor()) : FSlateColor(FLinearColor::White);
         })
         [
            SNew(SBox)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Padding(FMargin(4.0f))
            [
               SNew(STextBlock)
               .Font(FAppStyle::GetFontStyle("BoldFont"))
               .ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f))
               .ShadowOffset(FVector2D(1, 1))
               .Text_Lambda([this]() -> FText
               {
                  UOSEGenericGraphNode* node = GetGenericGraphNodeChecked()->GenericGraphNode;
                  return (node != nullptr) ? node->GetNodeIndicatorText() : FText::GetEmpty();
               })
            ]
         ]
      ]
   ;

   NodeCustomBodyWidget.Reset();
   UOSEGenericGraphEdNode* EdNode_Node = GetGenericGraphNodeChecked();
   NodeCustomBodyWidget = EdNode_Node->GenericGraphNode->ConstructNodeBodyWidget();
   if (NodeCustomBodyWidget.IsValid())
   {
      NodeCustomBodyParent->SetContent(NodeCustomBodyWidget.ToSharedRef());
   }

   // Create comment bubble
   TSharedPtr<SCommentBubble> CommentBubble;
   const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

   SAssignNew(CommentBubble, SCommentBubble)
      .GraphNode(GraphNode)
      .Text(this, &SGraphNode::GetNodeComment)
      .OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
      .ColorAndOpacity(CommentColor)
      .AllowPinning(true)
      .EnableTitleBarBubble(true)
      .EnableBubbleCtrls(true)
      .GraphLOD(this, &SGraphNode::GetCurrentLOD)
      .IsGraphNodeHovered(this, &SGraphNode::IsHovered);

   GetOrAddSlot(ENodeZone::TopCenter)
      .SlotOffset(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetOffset))
      .SlotSize(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetSize))
      .AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
      .VAlign(VAlign_Top)
      [
         CommentBubble.ToSharedRef()
      ];

   ErrorReporting = ErrorText;
   ErrorReporting->SetError(ErrorMsg);
   CreatePinWidgets();
}

void SOSEGenericGraphEdNode::CreatePinWidgets()
{
   UOSEGenericGraphEdNode* StateNode = GetGenericGraphNodeChecked();

   for (int32 PinIdx = 0; PinIdx < StateNode->Pins.Num(); PinIdx++)
   {
      UEdGraphPin* MyPin = StateNode->Pins[PinIdx];
      if (!MyPin->bHidden)
      {
         TSharedPtr<SGraphPin> NewPin = SNew(SGenericGraphPin, MyPin);

         AddPin(NewPin.ToSharedRef());
      }
   }
}

void SOSEGenericGraphEdNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
   PinToAdd->SetOwner(SharedThis(this));

   const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
   const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
   if (bAdvancedParameter)
   {
      PinToAdd->SetVisibility( TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced) );
   }

   TSharedPtr<SVerticalBox> PinBox;
   if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
   {
      PinBox = LeftNodeBox;
      InputPins.Add(PinToAdd);
   }
   else // Direction == EEdGraphPinDirection::EGPD_Output
   {
      PinBox = RightNodeBox;
      OutputPins.Add(PinToAdd);
   }

   if (PinBox)
   {
      PinBox->AddSlot()
         .HAlign(HAlign_Fill)
         .VAlign(VAlign_Fill)
         .FillHeight(1.0f)
         //.Padding(6.0f, 0.0f)
         [
            PinToAdd
         ];
   }
}

bool SOSEGenericGraphEdNode::IsNameReadOnly() const
{
   UOSEGenericGraphEdNode* EdNode_Node = GetGenericGraphNodeChecked();
   UOSEGenericGraph* GenericGraph = EdNode_Node->GenericGraphNode->Graph;
   check(GenericGraph != nullptr);
   return (!GenericGraph->bCanRenameNode || !EdNode_Node->GenericGraphNode->IsTitleEditable()) || SGraphNode::IsNameReadOnly();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FText SOSEGenericGraphEdNode::GetNodeEditableTitleText() const
{
   UOSEGenericGraphNode* Node = GetGenericGraphNodeChecked()->GenericGraphNode;
   check(Node != nullptr);
   return Node->IsTitleEditable() ? Node->GetEditableTitle() : Node->GetNodeDisplayTitle();
}

void SOSEGenericGraphEdNode::OnNodeEditableTitleTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
   SGraphNode::OnNameTextCommited(InText, CommitInfo);

   if (UOSEGenericGraphEdNode* MyNode = GetGenericGraphNodeChecked())
   {
      const FScopedTransaction Transaction(LOCTEXT("GenericGraphEditorRenameNode", "Generic Graph Editor: Rename Node"));
      MyNode->Modify();
      MyNode->GenericGraphNode->Modify();
      MyNode->GenericGraphNode->SetEditableTitle(InText);
      UpdateGraphNode();
   }
}

FSlateColor SOSEGenericGraphEdNode::GetBorderBackgroundColor() const
{
   UOSEGenericGraphEdNode* MyNode = CastChecked<UOSEGenericGraphEdNode>(GraphNode);
   return MyNode ? MyNode->GetBackgroundColor() : OSEGenericGraphColors::NodeBorder::HighlightAbortRange0;
}

FSlateColor SOSEGenericGraphEdNode::GetBackgroundColor() const
{
   return OSEGenericGraphColors::NodeBody::Default;
}

EVisibility SOSEGenericGraphEdNode::GetDragOverMarkerVisibility() const
{
   return EVisibility::Visible;
}

FText SOSEGenericGraphEdNode::GetToolTipText() const
{
   UOSEGenericGraphEdNode* MyNode = Cast<UOSEGenericGraphEdNode>(GraphNode);
   return (MyNode != nullptr && MyNode->GenericGraphNode != nullptr) ? MyNode->GenericGraphNode->GetNodeTooltipText() : FText::GetEmpty();
}

UOSEGenericGraphEdNode* SOSEGenericGraphEdNode::GetGenericGraphNodeChecked() const
{
   UOSEGenericGraphEdNode* Result = CastChecked<UOSEGenericGraphEdNode>(GraphNode);
   check(Result->GenericGraphNode != nullptr);
   return Result;
}

// static
const FSlateBrush* SOSEGenericGraphEdNode::StyleToNodeBodyBrush(EOSEGenericGraphNodeStyle NodeStyle)
{
   switch (NodeStyle)
   {
   case EOSEGenericGraphNodeStyle::Glossy:
      return FAppStyle::GetBrush("Graph.VarNode.Body");
   case EOSEGenericGraphNodeStyle::BorderLight:
      return FAppStyle::GetBrush("Graph.StateNode.Body");
   case EOSEGenericGraphNodeStyle::BorderDark:
      return FAppStyle::GetBrush("Graph.VarNode.Body");
   default:
      break;
   }
   return FAppStyle::GetBrush("Graph.Node.Body");
}

// static
const FSlateBrush* SOSEGenericGraphEdNode::StyleToNodeColorSpillBrush(EOSEGenericGraphNodeStyle NodeStyle)
{
   switch (NodeStyle)
   {
   case EOSEGenericGraphNodeStyle::Glossy:
      return FAppStyle::GetBrush("Graph.VarNode.ColorSpill");
   case EOSEGenericGraphNodeStyle::BorderLight:
      return FAppStyle::GetBrush("Graph.StateNode.ColorSpill");
   case EOSEGenericGraphNodeStyle::BorderDark:
      return FAppStyle::GetBrush("Graph.StateNode.Body");
   case EOSEGenericGraphNodeStyle::Flat:
      return nullptr;
   default:
      break;
   }
   return nullptr;
}

// static
const FSlateBrush* SOSEGenericGraphEdNode::StyleToNodeGlossBrush(EOSEGenericGraphNodeStyle NodeStyle)
{
   if (NodeStyle == EOSEGenericGraphNodeStyle::Glossy)
   {
      return FAppStyle::GetBrush("Graph.VarNode.Gloss");
   }
   return FAppStyle::GetBrush("Graph.Node.TitleGloss");
}

#undef LOCTEXT_NAMESPACE
