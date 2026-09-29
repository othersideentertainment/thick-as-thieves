// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "OSEGenericGraphEdge.generated.h"

class UOSEGenericGraph;
class UOSEGenericGraphNode;


USTRUCT(BlueprintType)
struct FOSEGenericGraphEdgePosition
{
   GENERATED_BODY()

   /// Normalized position along the edge's line. 0.0 is the start node, 1.0 is the end node.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generic Graph Edge Position", Meta = (UIMin = "0.0", UIMax = "1.0"))
   float NormalizedPosition = 0.5f;

   /// Normalized position along the horizontal axis, relative to the width of the node. -1.0 is left-aligned, 0.0 is centered, and 1.0 is right-aligned.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generic Graph Edge Position", Meta = (UIMin = "-1.0", UIMax = "1.0"))
   float HorizontalAlignment = 0.0f;

   /// Position offset on the axis that's perpendicular to the edge line
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generic Graph Edge Position", Meta = (UIMin = "-50.0", UIMax = "50.0"))
   float PerpendicularOffset = 0.0f;
};


UCLASS(Blueprintable)
class OSEGENERICGRAPHRUNTIME_API UOSEGenericGraphEdge : public UObject
{
   GENERATED_BODY()

public:
   UPROPERTY(VisibleDefaultsOnly, Category = "Generic Graph Edge")
   TObjectPtr<UOSEGenericGraph> Graph;

   UPROPERTY(BlueprintReadOnly, Category = "Generic Graph Edge")
   TObjectPtr<UOSEGenericGraphNode> StartNode;

   UPROPERTY(BlueprintReadOnly, Category = "Generic Graph Edge")
   TObjectPtr<UOSEGenericGraphNode> EndNode;

   UFUNCTION(BlueprintPure, Category = "Generic Graph Edge")
   UOSEGenericGraph* GetGraph() const;

   /// If Node is the start node, return the end node, and vice versa.
   /// If Node is not the start or end node, returns null.
   UFUNCTION(BlueprintPure, Category = "Generic Graph Edge")
   UOSEGenericGraphNode* GetOtherNode(UOSEGenericGraphNode* Node) const;

   /// Gets the debug name of this edge. Only useful in editor builds.
   UFUNCTION(BlueprintPure, Category = "Generic Graph Edge")
   virtual FString GetEdgeDebugName() const;

#if WITH_EDITOR
   // These functions allow customizing the behavior and appearance of the node in the editor.

   /// The color of the edge node (this is the transition node background color, not the line color)
   virtual FLinearColor GetEdgeColor() const { return FLinearColor(0.9f, 0.9f, 0.9f); }

   /// The icon on the edge node, if any. Return nullptr to hide the icon.
   virtual const FSlateBrush* GetEdgeIcon() const;

   /// Allow dragging the edge node.
   /// After the drag is complete, SetEdgePositionNormalized will be called with the closest relative position between the start and end nodes.
   /// Note that you will also need to implement GetEdgePosition and make sure it returns this value in the NormalizedPosition field.
   virtual bool CanDragEdge() const { return false; }

   /// Determines the position of the edge node.
   virtual FOSEGenericGraphEdgePosition GetEdgePosition() const { return FOSEGenericGraphEdgePosition{}; }

   /// Requests that the edge normalized position be set to a new value when the edge is dragged.
   virtual void SetEdgePositionNormalized(float NewNormalizedPosition) {}

   /// Gets the edge's tooltip text
   virtual FText GetEdgeTooltipText() const { return FText::GetEmpty(); }

   /// The text that will show up on the edge node when the title is not being edited
   virtual FText GetEdgeDisplayTitle() const { return FText::GetEmpty(); }

   /// Show a text-based label on this edge?
   virtual bool IsTitleVisible() const { return false; }

   /// Can users rename the edge (eg. by pressing F2)?
   virtual bool IsTitleEditable() const { return false; }

   /// Called after a user edits the edge title
   virtual void SetEditableTitle(const FText& NewTitle) {}

   /// The text that will show up in the text input box when editing the edge title
   virtual FText GetEditableTitle() const { return FText::GetEmpty(); }
#endif
};
