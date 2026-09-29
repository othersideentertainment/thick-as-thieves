// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueInfo.h"

#include "TATElectrotypeClue.generated.h"

struct FTATClueFactSpec;

UENUM()
enum class ETATElectrotypeClueDistribution
{
   // Get distributed to individual electrotypes
   Local,
   // Gets given to all electrotypes (for now duplicated)
   Global
};

// A clue type for electrotype booths
USTRUCT(meta=(DisplayName="Electrotype Clue"))
struct TAT_API FTATElectrotypeClue final : public FTATClueInfo
{
   GENERATED_BODY()
public:
   virtual FTATClueBucketKey GetClueBucket() const override;
   virtual void ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const override;
   virtual FString GetDebugDescription() const override { return TATClueInfoHelpers::FormatDebugDescription(TEXT("Electrotype"), TATClueInfoHelpers::MakePreviewText(ClueText)); }
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif

   // Valid replacements are {Item} and {Location}
   UPROPERTY(EditAnywhere)
   FText ClueText;

   UPROPERTY(EditAnywhere)
   ETATElectrotypeClueDistribution Distribution = ETATElectrotypeClueDistribution::Local;

   // facts that the clue contains
   // Feeds into the clue journal and highlights
   UPROPERTY(EditAnywhere, meta=(RowType = "/Script/TAT.TATClueFactSpec", TitleProperty="{RowName}"))
   TArray<FDataTableRowHandle> Facts;
};
