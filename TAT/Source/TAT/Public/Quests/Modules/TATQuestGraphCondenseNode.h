// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphNode.h"

#include "TATQuestGraphCondenseNode.generated.h"


/// Condenses the decision tree into one node.
/// Essentially a reroute node, it allows simplifying the flow of a quest graph.
UCLASS()
class TAT_API UTATQuestGraphCondenseNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

public:
   UTATQuestGraphCondenseNode();

#if WITH_EDITORONLY_DATA
   /// Editor-only label to show on this node. Effectively just a comment.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Editor")
   FString Label = TEXT("Condense");
#endif

   // From UOSEGenericGraphNode
   virtual FString GetNodeDebugName() const override;

#if WITH_EDITOR
   // From UOSEGenericGraphNode
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const override { return EOSEGenericGraphNodeStyle::BorderDark; }
   virtual FLinearColor GetBackgroundColor() const override;
   virtual FText GetNodeDisplayTitle() const override { return FText::FromString(Label); }
   virtual bool IsTitleEditable() const override { return true; }
   virtual FText GetEditableTitle() const override { return FText::FromString(Label); }
   virtual void SetEditableTitle(const FText& newTitle) override { Label = newTitle.ToString(); }
#endif
};
