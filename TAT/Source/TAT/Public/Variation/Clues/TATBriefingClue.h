// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueFact.h"
#include "Variation/Clues/TATClueInfo.h"

#include "TATBriefingClue.generated.h"

// replicated payload
USTRUCT()
struct FTATBriefingClueData
{
   GENERATED_BODY()

   UPROPERTY()
   FText ClueText;

   FTATSharedClueFactThunk Facts;
};


// A clue type for showing a popup at start
// Mostly a clue for convenience of authoring
USTRUCT(meta=(DisplayName="Briefing Clue"))
struct TAT_API FTATBriefingClueInfo final : public FTATClueInfo
{
   GENERATED_BODY()
public:
   virtual FTATClueBucketKey GetClueBucket() const override;
   virtual void ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const override;
   FTATBriefingClueData MakeBriefingData(const FTATClueContext& context, UWorld* world) const;
   virtual FString GetDebugDescription() const override { return TATClueInfoHelpers::FormatDebugDescription(TEXT("Briefing"), TATClueInfoHelpers::MakePreviewText(ClueText)); }

   static void TakeFromRequest(FTATClueRequest& request, TArray<FTATBriefingClueData>& outBriefings, UWorld* world);
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif

   // Valid replacements are {Item} and {Location}
   UPROPERTY(EditAnywhere)
   FText ClueText;

   // facts that the clue contains
   // Feeds into the clue journal and highlights
   UPROPERTY(EditAnywhere, meta=(RowType = "/Script/TAT.TATClueFactSpec", TitleProperty="{RowName}"))
   TArray<FDataTableRowHandle> Facts;
};
