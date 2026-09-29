// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "OSEAutoLayoutStrategy.h"
#include "OSETreeLayoutStrategy.generated.h"

class UOSEGenericGraphNode;
class UOSEGenericGraphEdNode;

UCLASS()
class OSEGENERICGRAPHEDITOR_API UOSETreeLayoutStrategy : public UOSEAutoLayoutStrategy
{
   GENERATED_BODY()
public:
   UOSETreeLayoutStrategy();
   virtual ~UOSETreeLayoutStrategy();

   virtual void Layout(UEdGraph* EdGraph) override;

protected:
   void InitPass(UOSEGenericGraphNode* RootNode, const FVector2D& Anchor);
   bool ResolveConflictPass(UOSEGenericGraphNode* Node);

   bool ResolveConflict(UOSEGenericGraphNode* LRoot, UOSEGenericGraphNode* RRoot);

   void GetLeftContour(UOSEGenericGraphNode* RootNode, int32 Level, TArray<UOSEGenericGraphEdNode*>& Contour);
   void GetRightContour(UOSEGenericGraphNode* RootNode, int32 Level, TArray<UOSEGenericGraphEdNode*>& Contour);

   void ShiftSubTree(UOSEGenericGraphNode* RootNode, const FVector2D& Offset);

   void UpdateParentNodePosition(UOSEGenericGraphNode* RootNode);
};
