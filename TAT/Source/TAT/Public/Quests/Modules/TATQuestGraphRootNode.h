// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphNode.h"

#include "TATQuestGraphRootNode.generated.h"


UCLASS()
class TAT_API UTATQuestGraphRootNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

public:
   UTATQuestGraphRootNode();

   /// Is this root node enabled?
   /// You can disable root nodes to allow more than one in a graph.
   /// This is mostly useful for temporarily disabling things for iteration and experimentation.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests")
   bool Enabled = true;

   // From UTATQuestGraphNode
   virtual bool ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const override;
   virtual bool IsRootNode() const override { return true; }

   // From UOSEGenericGraphNode
   virtual FString GetNodeDebugName() const override;

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;

   // From UOSEGenericGraphNode
   virtual TSharedPtr<SWidget> ConstructNodeBodyWidget() override;
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const override { return EOSEGenericGraphNodeStyle::Glossy; }
   virtual FLinearColor GetBackgroundColor() const override;
   virtual FText GetNodeDisplayTitle() const override { return FText::FromString(TEXT("Root")); }
   virtual FText GetNodeDisplaySubtitle() const override { return Enabled ? FText::GetEmpty() : FText::FromString(TEXT("Disabled")); }
   virtual bool CanCreateConnectionFrom(UOSEGenericGraphNode* other, int32 numberOfParentNodes, FText& errorMessage) override;
#endif
};
