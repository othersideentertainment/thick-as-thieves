// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueInfo.h"

#include "TATTalkingDoorClue.generated.h"

class UAkAudioEvent;
class UTATReadableClueVisuals;

// A clue type for talking doors (from say informants)
USTRUCT(meta=(DisplayName="Talking Door Clue"))
struct TAT_API FTATTalkingDoorClue final : public FTATClueInfo
{
   GENERATED_BODY()
public:
   virtual FTATClueBucketKey GetClueBucket() const override;
   virtual void ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const override;
   virtual FString GetDebugDescription() const override { return TATClueInfoHelpers::FormatDebugDescription(TEXT("Talking Door"), AudioEvent.GetAssetName()); }
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif

   // The audio to play
   // (TBD may adjust format)
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<UAkAudioEvent> AudioEvent;
   
   // facts that the clue contains
   // Feeds into the clue journal and highlights
   UPROPERTY(EditAnywhere, meta=(RowType = "/Script/TAT.TATClueFactSpec", TitleProperty="{RowName}"))
   TArray<FDataTableRowHandle> Facts;
};
