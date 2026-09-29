// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphSelectorNodeBase.h"
#include "Quests/Modules/TATQuestGraphTypes.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"

#include "TATQuestGraphSceneVariantNode.generated.h"


/// Selects scene variants to add to map variation
UCLASS()
class TAT_API UTATQuestGraphSceneVariantNode : public UTATQuestGraphSelectorNodeBase
{
   GENERATED_BODY()

public:
   UTATQuestGraphSceneVariantNode();

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests")
   TArray<FTATQuestRandomWeight_SceneVariant> SceneVariants;

   // From UTATQuestGraphNode
   virtual bool ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const override;
   virtual FTATQuestGraphCompatibility GetSelfCompatibility(const FTATQuestGraphEvalParams& params, const FTATQuestGraphEvalContext& ctx, FTATQuestGraphCompatibilityCache& cache) const override;
   virtual FTATQuestGraphSceneCache::MaskType GetSceneMask(FTATQuestGraphSceneCache& cache) const override;

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;

   // From UTATQuestGraphSelectorNodeBase
   virtual FText _GetSelectorTypeDisplayName() const override { return FText::FromString(TEXT("Scene Variant")); }
   virtual FName _GetSelectorListPropertyName() const override { return GET_MEMBER_NAME_CHECKED(UTATQuestGraphSceneVariantNode, SceneVariants); }
   virtual int32 _GetSelectorNumChoices() const override { return SceneVariants.Num(); }
   virtual FText _GetSelectorKeyColumnText(int32 idx) const override { return SceneVariants.IsValidIndex(idx) ? FText::AsNumber(SceneVariants[idx].Weight) : FText::GetEmpty(); }
   virtual FText _GetSelectorValueColumnText(int32 idx) const override { return SceneVariants.IsValidIndex(idx) ? FText::FromString(GetNameSafe(SceneVariants[idx].SceneVariant)) : FText::GetEmpty(); }
#endif
};
