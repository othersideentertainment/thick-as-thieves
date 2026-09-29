// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphNode.h"

#include "TATQuestGraphPropertyNode.generated.h"


/// Sets a quest property.
/// Note that this node assigns a value - multiple assignments to the same property will overwrite previous values.
UCLASS()
class TAT_API UTATQuestGraphPropertyNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

public:
   UTATQuestGraphPropertyNode();

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests")
   ETATQuestGraphPropertyType PropertyType = ETATQuestGraphPropertyType::None;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests", Meta = (Units = "seconds", EditCondition = "PropertyType == ETATQuestGraphPropertyType::EndgameDuration", EditConditionHides))
   float EndgameDuration = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests", Meta = (Units = "seconds", EditCondition = "PropertyType == ETATQuestGraphPropertyType::MatchMainPhaseDuration", EditConditionHides))
   float Duration = 0.0f;

   TOptional<FVariant> GetValueAsVariant() const;

   // From UTATQuestGraphNode
   virtual bool ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const override;

   // From UOSEGenericGraphNode
   virtual FString GetNodeDebugName() const override;

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;

   // From UOSEGenericGraphNode
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const override { return TATQuestGraphUtil::kSetterStyle; }
   virtual FLinearColor GetBackgroundColor() const override { return TATQuestGraphUtil::kSetterColor; }
   virtual void GetCustomGraphContextMenuActions(TArray<FOSEGenericGraphNodeCustomAction>& outActions) const override;
   virtual FText GetNodeDisplayTitle() const override;
   virtual FText GetNodeDisplaySubtitle() const override;
#endif

};
