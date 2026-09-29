// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphNode.h"

// ue
#include "StructUtils/InstancedStruct.h"

#include "TATQuestGraphObjectiveNode.generated.h"

struct FTATQuestObjectiveInfo;


/// Sets the quest objective.
/// Note that subsequent calls will replace the existing objective.
UCLASS()
class TAT_API UTATQuestGraphObjectiveNode : public UTATQuestGraphNode
{
   GENERATED_BODY()

public:
   UTATQuestGraphObjectiveNode();

   UPROPERTY(EditAnywhere, Category = "Quests", meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATQuestObjectiveInfo", HideFromDataTableEditorColumn))
   FInstancedStruct Objective;

   /// Displayed in pre/post-match UI to visualize the objective
   UPROPERTY(EditAnywhere, Category = "Quests")
   TSoftObjectPtr<UTexture2D> DisplayImage;

   // From UTATQuestGraphNode
   virtual bool ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const override;

   // From UOSEGenericGraphNode
   virtual FString GetNodeDebugName() const override { return FString::Printf(TEXT("ObjectiveNode(%s)"), *_GetObjectiveDebugDescription()); }

   const FTATQuestObjectiveInfo& GetObjectiveChecked() const;

#if WITH_EDITOR
   // From UOSEGenericGraphNode
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const override { return TATQuestGraphUtil::kSetterStyle; }
   virtual FLinearColor GetBackgroundColor() const override { return TATQuestGraphUtil::kSetterColor; }
   virtual FText GetNodeDisplayTitle() const override { return FText::FromString(TEXT("Set Objective")); }
   virtual FText GetNodeDisplaySubtitle() const override { return FText::FromString(_GetObjectiveDebugDescription()); }
#endif

protected:
   FString _GetObjectiveDebugDescription() const;
};
