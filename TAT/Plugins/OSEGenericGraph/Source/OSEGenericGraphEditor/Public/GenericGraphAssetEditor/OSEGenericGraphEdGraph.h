// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "OSEGenericGraphEdGraph.generated.h"

class UOSEGenericGraph;
class UOSEGenericGraphNode;
class UOSEGenericGraphEdge;
class UOSEGenericGraphEdNode;
class UOSEGenericGraphEdNodeEdge;

UCLASS()
class OSEGENERICGRAPHEDITOR_API UOSEGenericGraphEdGraph : public UEdGraph
{
   GENERATED_BODY()

public:
   UOSEGenericGraphEdGraph();
   virtual ~UOSEGenericGraphEdGraph();

   virtual void RebuildGenericGraph();

   UOSEGenericGraph* GetGenericGraph() const;

   virtual bool Modify(bool bAlwaysMarkDirty = true) override;
   virtual void PostEditUndo() override;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   UPROPERTY(Transient)
   TMap<UOSEGenericGraphNode*, UOSEGenericGraphEdNode*> NodeMap;

   UPROPERTY(Transient)
   TMap<UOSEGenericGraphEdge*, UOSEGenericGraphEdNodeEdge*> EdgeMap;

protected:
   void Clear();

   void SortNodes(UOSEGenericGraphNode* RootNode);
};
