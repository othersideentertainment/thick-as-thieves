// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATQuestGraphNode.h"


#include "TATQuestGraphTextReplacementNode.generated.h"

struct FInstancedStruct;

/**
 * 
 */
UCLASS()
class TAT_API UTATQuestGraphTextReplacementNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

   
   UTATQuestGraphTextReplacementNode();
   
   // Map of text replacement key to Value
   //
   // Example:
   // If you set "MyCoolThing" = "Foo", then {MyCoolThing} will be replaced
   // with the "Foo" in clue text
   UPROPERTY(EditAnywhere, Category = TextReplacement)
   TMap<FString, FText> FormatParams;
   
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
#endif

protected:
   FString _GetDetailsString() const;
};


UCLASS()
class TAT_API UTATQuestGraphTextReplacementSourceNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

   
   UTATQuestGraphTextReplacementSourceNode();

   // Additional things that can produce text replacement
   UPROPERTY(EditDefaultsOnly, Category="TextReplacement", meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATQuestFormatParamSource"))
   TArray<FInstancedStruct> FormatParamSources;
   
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
#endif

protected:
   FString _GetDetailsString() const;
};
