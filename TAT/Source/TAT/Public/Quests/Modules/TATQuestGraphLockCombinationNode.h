// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATQuestGraphNode.h"
#include "Lockpicking/TATLockCombinationName.h"

#include "TATQuestGraphLockCombinationNode.generated.h"


struct FTATLockCombinationNameRef;

UCLASS()
class TAT_API UTATQuestGraphLockCombinationNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

public:

   UTATQuestGraphLockCombinationNode();
   
   // Map of text replacement key to lock combination "name"
   //
   // Example:
   // If you have a combination lock configured with CombinationName = "Foo",
   // and set "MyCombination" = "Foo", then {MyCombination} will be replaced
   // with the combination used by that lock (or locks).
   UPROPERTY(EditAnywhere, Category = TextReplacement)
   TMap<FString, FTATLockCombinationNameRef> LockCombinations;
   
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
   virtual FText GetNodeDisplayTitle() const override;
   virtual FText GetNodeDisplaySubtitle() const override;

   // from UTATQuestGraphNode
   virtual void ValidateAgainstWorld(FTATQuestGraphMapCheckContext& context) const override;
   virtual bool RequireMapForValidation() const override { return true; }
#endif

protected:
   FString _GetDetailsString() const;
};
