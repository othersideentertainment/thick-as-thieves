// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphSelectorNodeBase.h"

#include "TATQuestGraphTagNode.generated.h"


/// Adds a gameplay tag to the quest
UCLASS()
class TAT_API UTATQuestGraphTagNode : public UTATQuestGraphSelectorNodeBase
{
   GENERATED_BODY()

public:
   UTATQuestGraphTagNode();

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests", Meta = (TitleProperty = "Tag"))
   TArray<FTATQuestRandomWeight> TagWeights;

#if WITH_EDITORONLY_DATA
   /// Custom label that appears in the node title. Editor-only.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests")
   FString SelectorTypeDisplayName;
#endif

   // From UTATQuestGraphNode
   virtual bool ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const override;

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;

   // From UTATQuestGraphSelectorNodeBase
   virtual FText _GetSelectorTypeDisplayName() const override { return FText::FromString(SelectorTypeDisplayName); }
   virtual FName _GetSelectorListPropertyName() const override { return GET_MEMBER_NAME_CHECKED(UTATQuestGraphTagNode, TagWeights); }
   virtual int32 _GetSelectorNumChoices() const override { return TagWeights.Num(); }
   virtual FText _GetSelectorKeyColumnText(int32 idx) const override { return TagWeights.IsValidIndex(idx) ? FText::AsNumber(TagWeights[idx].Weight) : FText::GetEmpty(); }
   virtual FText _GetSelectorValueColumnText(int32 idx) const override { return TagWeights.IsValidIndex(idx) ? FText::FromString(TagWeights[idx].Tag.ToString()) : FText::GetEmpty(); }
#endif
};
