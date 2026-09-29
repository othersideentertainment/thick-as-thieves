// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "SNodePanel.h"
#include "SGraphNode.h"

class SToolTip;
class UOSEGenericGraphEdNodeEdge;

class OSEGENERICGRAPHEDITOR_API SOSEGenericGraphEdNodeEdge : public SGraphNode
{
public:
   SLATE_BEGIN_ARGS(SOSEGenericGraphEdNodeEdge){}
   SLATE_END_ARGS()

   void Construct(const FArguments& InArgs, UOSEGenericGraphEdNodeEdge* InNode);

   // SNodePanel::SNode interface
   virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool MarkDirty = true) override;

   virtual bool RequiresSecondPassLayout() const override;
   virtual void PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const override;

   virtual void UpdateGraphNode() override;

   // Calculate position for multiple nodes to be placed between a start and end point
   void PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom) const;

protected:
   FText GetToolTipText() const;
   FSlateColor GetEdgeColor() const;

   const FSlateBrush* GetEdgeImage() const;
   EVisibility GetEdgeImageVisibility() const;

   bool IsEdgeTitleReadOnly() const;
   FText GetEdgeEditableTitleText() const;
   EVisibility GetEdgeTitleVisbility() const;
   void OnEdgeTitleTextCommited(const FText& InText, ETextCommit::Type CommitInfo);

private:
   TSharedPtr<STextEntryPopup> TextEntryWidget;
   bool IsDragging = false;
};
