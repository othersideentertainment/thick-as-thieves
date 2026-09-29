// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ose
#include "OSEGenericGraph.h"

// ue
#include "GameplayTagContainer.h"

#include "TATQuestGraphEdge.generated.h"


UCLASS()
class TAT_API UTATQuestGraphEdge : public UOSEGenericGraphEdge
{
   GENERATED_BODY()

public:
   UTATQuestGraphEdge();

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests", Meta = (UIMin = 0.001, ClampMin = 0.001))
   bool AlwaysTakePathIfConditionsMatch = false;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests", Meta = (UIMin = 0.001, ClampMin = 0.001, EditCondition = "!AlwaysTakePathIfConditionsMatch"))
   float ChanceToTakePath = 1.0f;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests", meta = (Categories="QuestGraphQuestTags"))
   FGameplayTagQuery QuestTagQuery;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quests", meta = (Categories="QuestGraphWorldTags"))
   FGameplayTagQuery WorldTagQuery;

#if WITH_EDITORONLY_DATA
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Editor")
   bool ShowQuery = false;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Editor")
   FOSEGenericGraphEdgePosition Position;
#endif

#if WITH_EDITOR
   virtual FLinearColor GetEdgeColor() const override;
   virtual const FSlateBrush* GetEdgeIcon() const override;
   virtual bool CanDragEdge() const override { return true; }
   virtual FOSEGenericGraphEdgePosition GetEdgePosition() const override { return Position; }
   virtual void SetEdgePositionNormalized(float newNormalizedPosition) override { Position.NormalizedPosition = newNormalizedPosition; }
   virtual FText GetEdgeTooltipText() const override;
   virtual FText GetEdgeDisplayTitle() const override;
   virtual bool IsTitleVisible() const override { return true; }
#endif

protected:
   FText _GetEdgeDescription(bool detailed) const;
};
