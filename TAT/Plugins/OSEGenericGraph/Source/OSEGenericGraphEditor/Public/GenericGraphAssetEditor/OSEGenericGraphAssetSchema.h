// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "OSEGenericGraph.h"
#include "OSEGenericGraphNode.h"
#include "OSEGenericGraphEdge.h"
#include "OSEGenericGraphAssetSchema.generated.h"

class UOSEGenericGraphEdNode;
class UOSEGenericGraphEdNodeEdge;
class UOSEGenericGraphAutoLayoutStrategy;

/** Action to add a node to the graph */
USTRUCT()
struct OSEGENERICGRAPHEDITOR_API FOSEGenericGraphAssetSchemaAction_NewNode : public FEdGraphSchemaAction
{
   GENERATED_USTRUCT_BODY();

public:
   FOSEGenericGraphAssetSchemaAction_NewNode(): NodeTemplate(nullptr) {}

   FOSEGenericGraphAssetSchemaAction_NewNode(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
      : FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping), NodeTemplate(nullptr) {}

   virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
   virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

   TObjectPtr<UOSEGenericGraphEdNode> NodeTemplate;
};

USTRUCT()
struct OSEGENERICGRAPHEDITOR_API FOSEGenericGraphAssetSchemaAction_NewEdge : public FEdGraphSchemaAction
{
   GENERATED_USTRUCT_BODY();

public:
   FOSEGenericGraphAssetSchemaAction_NewEdge(): NodeTemplate(nullptr){}

   FOSEGenericGraphAssetSchemaAction_NewEdge(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
      : FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping), NodeTemplate(nullptr) {}

   virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
   virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

   TObjectPtr<UOSEGenericGraphEdNodeEdge> NodeTemplate;
};

USTRUCT()
struct OSEGENERICGRAPHEDITOR_API FOSEGenericGraphAssetSchemaAction_NewComment : public FEdGraphSchemaAction
{
   GENERATED_USTRUCT_BODY();

public:
   using FEdGraphSchemaAction::FEdGraphSchemaAction;
   virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

UCLASS(MinimalAPI)
class UOSEGenericGraphAssetSchema : public UEdGraphSchema
{
   GENERATED_BODY()

public:
   void GetBreakLinkToSubMenuActions(class UToolMenu* Menu, class UEdGraphPin* InGraphPin);

   virtual EGraphType GetGraphType(const UEdGraph* TestEdGraph) const override;

   virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;

   virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

   virtual TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const override;

   virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;

   virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
   virtual bool CreateAutomaticConversionNodeAndConnections(UEdGraphPin* A, UEdGraphPin* B) const override;

   virtual class FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;

   virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;

   virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;

   virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const override;

   virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;

   virtual UEdGraphPin* DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const override;

   virtual bool SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const override;

   virtual bool IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const override;

   virtual int32 GetCurrentVisualizationCacheID() const override;

   virtual void ForceVisualizationCacheClear() const override;

private:
   static int32 CurrentCacheRefreshID;
};

