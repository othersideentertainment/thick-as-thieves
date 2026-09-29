// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "OSEGenericGraphEdNodeEdge.generated.h"

class UOSEGenericGraphNode;
class UOSEGenericGraphEdge;
class UOSEGenericGraphEdNode;

UCLASS(MinimalAPI)
class UOSEGenericGraphEdNodeEdge : public UEdGraphNode
{
   GENERATED_BODY()

public:
   UOSEGenericGraphEdNodeEdge();

   UPROPERTY()
   class UEdGraph* Graph;

   UPROPERTY(VisibleAnywhere, Instanced, Category = "GenericGraph")
   UOSEGenericGraphEdge* GenericGraphEdge;

   void SetEdge(UOSEGenericGraphEdge* Edge);

   virtual void AllocateDefaultPins() override;

   virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

   virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;

   virtual void PrepareForCopying() override;

   virtual UEdGraphPin* GetInputPin() const { return Pins[0]; }
   virtual UEdGraphPin* GetOutputPin() const { return Pins[1]; }

   void CreateConnections(UOSEGenericGraphEdNode* Start, UOSEGenericGraphEdNode* End);

   UOSEGenericGraphEdNode* GetStartNode();
   UOSEGenericGraphEdNode* GetEndNode();
};
