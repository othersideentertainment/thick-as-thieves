// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "OSEGenericGraph.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdGraph.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNode.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEdNodeEdge.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEditorSettings.h"
#include "OSEAutoLayoutStrategy.generated.h"

class UOSEGenericGraphEditorSettings;

UCLASS(abstract)
class OSEGENERICGRAPHEDITOR_API UOSEAutoLayoutStrategy : public UObject
{
   GENERATED_BODY()
public:
   UOSEAutoLayoutStrategy();
   virtual ~UOSEAutoLayoutStrategy();

   virtual void Layout(UEdGraph* G) {};

   const FOSEGenericGraphEditorSettings* Settings;

protected:
   int32 GetNodeWidth(UOSEGenericGraphEdNode* EdNode);

   int32 GetNodeHeight(UOSEGenericGraphEdNode* EdNode);

   FBox2D GetNodeBound(UEdGraphNode* EdNode);

   FBox2D GetActualBounds(UOSEGenericGraphNode* RootNode);

   virtual void RandomLayoutOneTree(UOSEGenericGraphNode* RootNode, const FBox2D& Bound);

protected:
   UOSEGenericGraph* Graph;
   UOSEGenericGraphEdGraph* EdGraph;
   int32 MaxIteration;
   int32 OptimalDistance;
};
