// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueInfo.h"

#include "TATReadableClue.generated.h"

class UTATReadableClueVisuals;

// A clue type for readable things that are read with an explicit interaction
USTRUCT(meta=(DisplayName="Readable Clue"))
struct TAT_API FTATReadableClue final : public FTATClueInfo
{
   GENERATED_BODY()
public:
   virtual FTATClueBucketKey GetClueBucket() const override;
   virtual void ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const override;
   virtual FString GetDebugDescription() const override;
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif

   // An asset that is the visual representation of the readable. (e.g. Note, RedFancyNote, …) These should be reused between clues.
   UPROPERTY(EditAnywhere)
   TObjectPtr<UTATReadableClueVisuals> Visuals;

   // A tag that is matched with a placement tag on the clue spawner (That is, the spawner for the readable).
   //
   // For example, if categorizing by rarity, a readable clue marked CluePlacement.Common would only go to a
   // readable spawner configured with CluePlacement.Common.
   UPROPERTY(EditAnywhere, meta=(Categories="CluePlacement"))
   FGameplayTag PlacementTag;
   
   UPROPERTY(EditAnywhere, meta=(MultiLine))
   FText ClueText;

   // (optional) variations of the clue text that can be randomly chosen
   UPROPERTY(EditAnywhere, meta=(MultiLine))
   TArray<FText> AlternateClueText;

   // Whether to allow text replacement of {Stuff} from the context
   // NOTE: If this becomes onerous or error-prone, this can be removed and always treated as allow
   UPROPERTY(EditAnywhere)
   bool AllowTextReplacement = false;

   // facts that the clue contains
   // Feeds into the clue journal and highlights
   UPROPERTY(EditAnywhere, meta=(RowType = "/Script/TAT.TATClueFactSpec", TitleProperty="{RowName}"))
   TArray<FDataTableRowHandle> Facts;
};
