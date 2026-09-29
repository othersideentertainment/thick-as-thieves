// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "OSEGenericGraphNode.h"
#include "OSEGenericGraphEdNode.generated.h"

class UOSEGenericGraphEdNodeEdge;
class UOSEGenericGraphEdGraph;
class SOSEGenericGraphEdNode;

UCLASS(MinimalAPI)
class UOSEGenericGraphEdNode : public UEdGraphNode
{
   GENERATED_BODY()

public:
   UOSEGenericGraphEdNode();
   virtual ~UOSEGenericGraphEdNode();

   UPROPERTY(VisibleAnywhere, Instanced, Category = "Generic Graph")
   UOSEGenericGraphNode* GenericGraphNode;

   void SetGenericGraphNode(UOSEGenericGraphNode* InNode);
   UOSEGenericGraphEdGraph* GetGenericGraphEdGraph();

   SOSEGenericGraphEdNode* SEdNode;

   virtual void AllocateDefaultPins() override;
   virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
   virtual void PrepareForCopying() override;
   virtual void AutowireNewNode(UEdGraphPin* FromPin) override;

   virtual FLinearColor GetBackgroundColor() const;
   const FSlateBrush* GetNodeIcon() const;

   virtual UEdGraphPin* GetInputPin() const;
   virtual UEdGraphPin* GetOutputPin() const;

#if WITH_EDITOR
   virtual void PostEditUndo() override;
#endif

};
