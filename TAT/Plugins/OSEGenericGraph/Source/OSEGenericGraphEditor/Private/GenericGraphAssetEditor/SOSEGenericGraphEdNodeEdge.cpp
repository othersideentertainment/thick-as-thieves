// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/SOSEGenericGraphEdNodeEdge.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/SToolTip.h"
#include "SGraphPanel.h"
#include "EdGraphSchema_K2.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNodeEdge.h"
#include "GenericGraphAssetEditor/OSEGenericGraphConnectionDrawingPolicy.h"
#include "OSEGenericGraphEdge.h"
#include "GenericGraphAssetEditor/SOSEGenericGraphEdNode.h"

#define LOCTEXT_NAMESPACE "SGenericGraphEdge"

namespace GenericGraphHelpers
{
   TOptional<FGeometry> GetNodeGeometry(UOSEGenericGraphEdNode* Node)
   {
      if (Node != nullptr && Node->SEdNode != nullptr)
      {
         return FGeometry(FVector2D(Node->NodePosX, Node->NodePosY), FVector2D::ZeroVector, Node->SEdNode->GetDesiredSize(), 1.0f);
      }
      return NullOpt;
   }

   TOptional<FGeometry> GetNodeGeometry(UOSEGenericGraphEdNode* Node, const TSharedPtr<SWidget>& Widget)
   {
      if (Node != nullptr && Widget != nullptr)
      {
         return FGeometry(FVector2D(Node->NodePosX, Node->NodePosY), FVector2D::ZeroVector, Widget->GetDesiredSize(), 1.0f);
      }
      return NullOpt;
   }

   void GetEdgeEndpoints(const FGeometry& StartGeom, const FGeometry& EndGeom, const FVector2D& EdgeNodeDesiredSize, FVector2D& OutStart, FVector2D& OutEnd)
   {
      // Get a reasonable seed point (halfway between the boxes)
      const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
      const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
      const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

      // Find the (approximate) closest points between the two boxes
      FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
      FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

      // Adjust the anchor points to take the node height into account
      const float NodeSize = FMath::Min(EdgeNodeDesiredSize.X, EdgeNodeDesiredSize.Y);
      const FVector2D Dir = (StartAnchorPoint - EndAnchorPoint).GetSafeNormal();
      StartAnchorPoint -= Dir * NodeSize * 0.5f;
      EndAnchorPoint += Dir * NodeSize * 0.5f;

      FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
      if (DeltaPos.IsNearlyZero())
      {
         DeltaPos = FVector2D(10.0f, 0.0f);
      }

      OutStart = StartAnchorPoint;
      OutEnd = StartAnchorPoint + DeltaPos;
   }

   FVector2D PerpendicularOffsetToTranslation(float PerpendicularOffset, const FVector2D& StartAnchorPoint, const FVector2D EndAnchorPoint)
   {
      const FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
      const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();
      return (PerpendicularOffset + 6.0f) * Normal;
   }

   FVector2D HorizontalAlignmentToTranslation(float HorizontalAlignment, const FVector2D& EdgeNodeDesiredSize)
   {
      const float TranslateX = FMath::GetMappedRangeValueUnclamped<float>({ -1.0f, 1.0f }, FVector2f(EdgeNodeDesiredSize.X * -0.5f, EdgeNodeDesiredSize.X * 0.5f), HorizontalAlignment);
      return FVector2D(TranslateX, 0.0);
   }

   FVector2D GetEdgeNodePositionCenter(const FVector2D& StartAnchorPoint, const FVector2D EndAnchorPoint, const FOSEGenericGraphEdgePosition& EdgePos, const FVector2D& EdgeNodeDesiredSize)
   {
      const FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
      const FVector2D NewCenter = StartAnchorPoint + (EdgePos.NormalizedPosition * DeltaPos) + PerpendicularOffsetToTranslation(EdgePos.PerpendicularOffset, StartAnchorPoint, EndAnchorPoint);
      return NewCenter + HorizontalAlignmentToTranslation(EdgePos.HorizontalAlignment, EdgeNodeDesiredSize);
   }

   FVector2D GetEdgeNodePositionTopLeft(const FVector2D& StartAnchorPoint, const FVector2D EndAnchorPoint, const FOSEGenericGraphEdgePosition& EdgePos, const FVector2D& EdgeNodeDesiredSize)
   {
      return GetEdgeNodePositionCenter(StartAnchorPoint, EndAnchorPoint, EdgePos, EdgeNodeDesiredSize) - (0.5f * EdgeNodeDesiredSize);
   }
}

void SOSEGenericGraphEdNodeEdge::Construct(const FArguments& InArgs, UOSEGenericGraphEdNodeEdge* InNode)
{
   this->GraphNode = InNode;
   this->UpdateGraphNode();
}

void SOSEGenericGraphEdNodeEdge::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool MarkDirty)
{
   UOSEGenericGraphEdNodeEdge* EdgeNode = Cast<UOSEGenericGraphEdNodeEdge>(GraphNode);
   const bool AllowDrag = EdgeNode != nullptr && EdgeNode->GenericGraphEdge != nullptr && EdgeNode->GenericGraphEdge->CanDragEdge();

   const bool WasDragging = IsDragging;
   IsDragging = AllowDrag && !MarkDirty;

   SGraphNode::MoveTo(NewPosition, NodeFilter, MarkDirty);

   if (AllowDrag && WasDragging && !IsDragging)
   {
      check(EdgeNode != nullptr && EdgeNode->GenericGraphEdge != nullptr);

      UOSEGenericGraphEdNode* Start = EdgeNode->GetStartNode();
      UOSEGenericGraphEdNode* End = EdgeNode->GetEndNode();
      if (Start != nullptr && Start->SEdNode != nullptr && End != nullptr && End->SEdNode != nullptr)
      {
         const FVector2D CurDesiredSize = GetDesiredSize();

         FVector2D EdgeStart = FVector2D::ZeroVector;
         FVector2D EdgeEnd = FVector2D::ZeroVector;
         GenericGraphHelpers::GetEdgeEndpoints(
            *GenericGraphHelpers::GetNodeGeometry(Start),
            *GenericGraphHelpers::GetNodeGeometry(End),
            CurDesiredSize,
            EdgeStart,
            EdgeEnd);

         // Find the actual target position _before_ horizontal and perpendicular adjustments are applied
         const FOSEGenericGraphEdgePosition EdgePos = EdgeNode->GenericGraphEdge->GetEdgePosition();
         const FVector2D NewPositionAdjusted = NewPosition
            - GenericGraphHelpers::HorizontalAlignmentToTranslation(EdgePos.HorizontalAlignment, CurDesiredSize)
            - GenericGraphHelpers::PerpendicularOffsetToTranslation(EdgePos.PerpendicularOffset, EdgeStart, EdgeEnd)
            + (CurDesiredSize * 0.5f)
         ;

         const FVector2D ClosestPointOnEdge = FMath::ClosestPointOnSegment2D(NewPositionAdjusted, EdgeStart, EdgeEnd);
         const float DistToClosestPoint = FVector2D::Distance(EdgeStart, ClosestPointOnEdge);
         const float EdgeLength = FVector2D::Distance(EdgeStart, EdgeEnd);
         const float NewRelativePosition = DistToClosestPoint / FMath::Max(EdgeLength, 1.0f);
         EdgeNode->GenericGraphEdge->SetEdgePositionNormalized(FMath::Clamp(NewRelativePosition, 0.0f, 1.0f));
      }
   }
}

bool SOSEGenericGraphEdNodeEdge::RequiresSecondPassLayout() const
{
   return !IsDragging;
}

void SOSEGenericGraphEdNodeEdge::PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const
{
   UOSEGenericGraphEdNodeEdge* EdgeNode = CastChecked<UOSEGenericGraphEdNodeEdge>(GraphNode);

   FGeometry StartGeom;
   FGeometry EndGeom;

   UOSEGenericGraphEdNode* Start = EdgeNode->GetStartNode();
   UOSEGenericGraphEdNode* End = EdgeNode->GetEndNode();
   if (Start != nullptr && End != nullptr)
   {
      const TSharedRef<SNode>* pFromWidget = NodeToWidgetLookup.Find(Start);
      const TSharedRef<SNode>* pToWidget = NodeToWidgetLookup.Find(End);
      if (pFromWidget != nullptr && pToWidget != nullptr)
      {
         if (TOptional<FGeometry> Geo = GenericGraphHelpers::GetNodeGeometry(Start, *pFromWidget))
         {
            StartGeom = *Geo;
         }
         if (TOptional<FGeometry> Geo = GenericGraphHelpers::GetNodeGeometry(End, *pToWidget))
         {
            EndGeom = *Geo;
         }
      }
   }

   PositionBetweenTwoNodesWithOffset(StartGeom, EndGeom);
}

void SOSEGenericGraphEdNodeEdge::UpdateGraphNode()
{
   InputPins.Empty();
   OutputPins.Empty();

   RightNodeBox.Reset();
   LeftNodeBox.Reset();

   this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
   this->GetOrAddSlot( ENodeZone::Center )
      .HAlign(HAlign_Center)
      .VAlign(VAlign_Center)
      [
         SNew(SOverlay)
         .ToolTipText(this, &SOSEGenericGraphEdNodeEdge::GetToolTipText)

         +SOverlay::Slot()
         [
            SNew(SImage)
            .Image(FAppStyle::GetBrush("Graph.TransitionNode.ColorSpill"))
            .ColorAndOpacity(this, &SOSEGenericGraphEdNodeEdge::GetEdgeColor)
         ]

         +SOverlay::Slot()
         .Padding(FMargin(4.0f))
         [
            SNew(SBox)
            .MinDesiredWidth(24.0f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
               SNew(SHorizontalBox)
               +SHorizontalBox::Slot()
               .AutoWidth()
               [
                  SNew(SBox)
                  .VAlign(VAlign_Center)
                  .Padding(FMargin(2.0f))
                  .Visibility(this, &SOSEGenericGraphEdNodeEdge::GetEdgeImageVisibility)
                  [
                     SNew(SImage)
                     .Image(this, &SOSEGenericGraphEdNodeEdge::GetEdgeImage)
                  ]
               ]
               +SHorizontalBox::Slot()
               .AutoWidth()
               [
                  SNew(SBox)
                  .VAlign(VAlign_Center)
                  .HAlign(HAlign_Left)
                  .Padding(FMargin(2.0f))
                  .Visibility(this, &SOSEGenericGraphEdNodeEdge::GetEdgeTitleVisbility)
                  [
                     SNew(SVerticalBox)
                     +SVerticalBox::Slot()
                     .VAlign(VAlign_Center)
                     .AutoHeight()
                     [
                        SAssignNew(InlineEditableText, SInlineEditableTextBlock)
                        .ColorAndOpacity(FLinearColor::Black)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
                        .Text(this, &SOSEGenericGraphEdNodeEdge::GetEdgeEditableTitleText)
                        .OnTextCommitted(this, &SOSEGenericGraphEdNodeEdge::OnEdgeTitleTextCommited)
                        .IsReadOnly(this, &SOSEGenericGraphEdNodeEdge::IsEdgeTitleReadOnly)
                     ]
                     +SVerticalBox::Slot()
                     .VAlign(VAlign_Center)
                     .AutoHeight()
                     [
                        SNew(SNodeTitle, GraphNode)
                        .Visibility_Lambda([this]() -> EVisibility
                        {
                           return IsEdgeTitleReadOnly() ? EVisibility::Collapsed : EVisibility::Visible;
                        })
                     ]
                  ]
               ]
            ]
         ]
      ];
}

void SOSEGenericGraphEdNodeEdge::PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom) const
{
   const FVector2D CurDesiredSize = GetDesiredSize();

   FVector2D StartAnchorPoint = FVector2D::ZeroVector;
   FVector2D EndAnchorPoint = FVector2D::ZeroVector;
   GenericGraphHelpers::GetEdgeEndpoints(StartGeom, EndGeom, CurDesiredSize, StartAnchorPoint, EndAnchorPoint);

   FOSEGenericGraphEdgePosition EdgePos{};
   UOSEGenericGraphEdNodeEdge* EdgeNode = Cast<UOSEGenericGraphEdNodeEdge>(GraphNode);
   if (EdgeNode != nullptr && EdgeNode->GenericGraphEdge != nullptr)
   {
      EdgePos = EdgeNode->GenericGraphEdge->GetEdgePosition();
   }

   const FVector2D NewPos = GenericGraphHelpers::GetEdgeNodePositionTopLeft(StartAnchorPoint, EndAnchorPoint, EdgePos, CurDesiredSize);
   GraphNode->NodePosX = NewPos.X;
   GraphNode->NodePosY = NewPos.Y;
}

FText SOSEGenericGraphEdNodeEdge::GetToolTipText() const
{
   UOSEGenericGraphEdNodeEdge* EdgeNode = CastChecked<UOSEGenericGraphEdNodeEdge>(GraphNode);
   if (EdgeNode != nullptr && EdgeNode->GenericGraphEdge != nullptr)
   {
      return EdgeNode->GenericGraphEdge->GetEdgeTooltipText();
   }
   return FText::GetEmpty();
}

FSlateColor SOSEGenericGraphEdNodeEdge::GetEdgeColor() const
{
   UOSEGenericGraphEdNodeEdge* EdgeNode = CastChecked<UOSEGenericGraphEdNodeEdge>(GraphNode);
   if (EdgeNode != nullptr && EdgeNode->GenericGraphEdge != nullptr)
   {
      return EdgeNode->GenericGraphEdge->GetEdgeColor();
   }
   return FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
}

const FSlateBrush* SOSEGenericGraphEdNodeEdge::GetEdgeImage() const
{
   UOSEGenericGraphEdNodeEdge* EdgeNode = CastChecked<UOSEGenericGraphEdNodeEdge>(GraphNode);
   if (EdgeNode != nullptr && EdgeNode->GenericGraphEdge != nullptr)
   {
      return EdgeNode->GenericGraphEdge->GetEdgeIcon();
   }
   return FAppStyle::GetBrush("Graph.TransitionNode.Icon");
}

EVisibility SOSEGenericGraphEdNodeEdge::GetEdgeImageVisibility() const
{
   UOSEGenericGraphEdNodeEdge* EdgeNode = CastChecked<UOSEGenericGraphEdNodeEdge>(GraphNode);
   if (EdgeNode != nullptr && EdgeNode->GenericGraphEdge != nullptr)
   {
      return (EdgeNode->GenericGraphEdge->GetEdgeIcon() != nullptr) ? EVisibility::Visible : EVisibility::Collapsed;
   }
   return EVisibility::Visible;
}

bool SOSEGenericGraphEdNodeEdge::IsEdgeTitleReadOnly() const
{
   UOSEGenericGraphEdNodeEdge* EdgeNode = CastChecked<UOSEGenericGraphEdNodeEdge>(GraphNode);
   if (EdgeNode != nullptr && EdgeNode->GenericGraphEdge != nullptr)
   {
      return !EdgeNode->GenericGraphEdge->IsTitleEditable();
   }
   return true;
}

FText SOSEGenericGraphEdNodeEdge::GetEdgeEditableTitleText() const
{
   UOSEGenericGraphEdNodeEdge* EdgeNode = CastChecked<UOSEGenericGraphEdNodeEdge>(GraphNode);
   if (EdgeNode != nullptr && EdgeNode->GenericGraphEdge != nullptr)
   {
      return EdgeNode->GenericGraphEdge->IsTitleEditable() ? EdgeNode->GenericGraphEdge->GetEditableTitle() : EdgeNode->GenericGraphEdge->GetEdgeDisplayTitle();
   }
   return FText::GetEmpty();
}

EVisibility SOSEGenericGraphEdNodeEdge::GetEdgeTitleVisbility() const
{
   UOSEGenericGraphEdNodeEdge* EdgeNode = CastChecked<UOSEGenericGraphEdNodeEdge>(GraphNode);
   if (EdgeNode && EdgeNode->GenericGraphEdge && EdgeNode->GenericGraphEdge->IsTitleVisible())
   {
      return EVisibility::Visible;
   }

   return EVisibility::Collapsed;
}

void SOSEGenericGraphEdNodeEdge::OnEdgeTitleTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
   SGraphNode::OnNameTextCommited(InText, CommitInfo);

   UOSEGenericGraphEdNodeEdge* MyNode = CastChecked<UOSEGenericGraphEdNodeEdge>(GraphNode);
   if (MyNode == nullptr || MyNode->GenericGraphEdge == nullptr)
   {
      return;
   }

   if (MyNode->GenericGraphEdge->IsTitleEditable())
   {
      const FScopedTransaction Transaction(LOCTEXT("GenericGraphEditorRenameEdge", "Generic Graph Editor: Rename Edge"));
      MyNode->Modify();
      MyNode->GenericGraphEdge->SetEditableTitle(InText);
      UpdateGraphNode();
   }
}

#undef LOCTEXT_NAMESPACE
