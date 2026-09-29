// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueInfo.h"

#include "TATNPCDialogueClue.generated.h"

struct FInstancedStruct;

/// A clue to be shared by an NPC in a dialogue interaction supported by UTATNPCClueSpawnerComponent.
/// Right now it supports a single text snippet on the first interaction, and a separate one for subsequent interactions.
USTRUCT(meta=(DisplayName="NPC Clue"))
struct TAT_API FTATNPCDialogueClue : public FTATClueInfo
{
   GENERATED_BODY()

public:
   // from FTATClueInfo
   virtual FTATClueBucketKey GetClueBucket() const override;
   virtual void ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const override;
   virtual FString GetDebugDescription() const override { return TATClueInfoHelpers::FormatDebugDescription(TEXT("NPC"), TATClueInfoHelpers::MakePreviewText(ClueText)); }
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif

   // A tag that is matched with a placement tag on the dialogue clue spawner.
   //
   // For example, if categorizing by rarity, a dialogue clue marked CluePlacement.Common would only go to a
   // dialogue clue spawner configured with CluePlacement.Common.
   UPROPERTY(EditAnywhere, meta=(Categories="CluePlacement"))
   FGameplayTag PlacementTag;

   UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
   bool UseOverrideInteractionPrompt = false;

   // Prompt displayed when interacting with an NPC to receive this clue.
   UPROPERTY(EditAnywhere, meta = (EditCondition = "UseOverrideInteractionPrompt"))
   FText OverrideInteractionPrompt;

   // The dialogue to display for the clue (supports formatting)
   UPROPERTY(EditAnywhere)
   FText ClueText;

   UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
   bool AllowFollowUpDialogue = true;

   // The dialogue to display on interactions after the clue was shared (does not support formatting)
   UPROPERTY(EditAnywhere, meta = (EditCondition = "AllowFollowUpDialogue"))
   FText FollowUpDialogue;

   // facts that the clue contains
   // Feeds into the clue journal and highlights
   UPROPERTY(EditAnywhere, meta=(RowType = "/Script/TAT.TATClueFactSpec", TitleProperty="{RowName}"))
   TArray<FDataTableRowHandle> Facts;

   // Things that are applied or granted to the player when they get the clue
   UPROPERTY(EditAnywhere, Meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATClueEffect"))
   TArray<FInstancedStruct> Effects;
};
