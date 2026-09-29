// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATElectrotypeClue.h"

// tat
#include "Variation/Clues/TATClueSpawnUtils.h"
#include "Variation/Clues/TATClueTextUtils.h"
#include "Variation/Clues/TATElectrotypeBooth.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATElectrotypeClue)

FTATClueBucketKey FTATElectrotypeClue::GetClueBucket() const
{
   const ETATClueType clueType = Distribution == ETATElectrotypeClueDistribution::Local ? ETATClueType::Electrotype : ETATClueType::ElectrotypeGlobal;
   return {clueType};
}

void FTATElectrotypeClue::ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const
{
   FText resolvedClueText = TATClueTextUtils::FormatUsingContext(ClueText, context, spawner);
   FTATSharedClueFactThunk factThunk = FTATClueFactThunk::Make(Facts, context);
   CastChecked<UTATElectrotypeClueComponent>(spawner)->AddClue({resolvedClueText, factThunk, context.SourceTag});
}

#if WITH_EDITOR
void FTATElectrotypeClue::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(ClueText.IsEmpty())
   {
      reportError(INVTEXT("ClueText is empty"));
   }
   
   ClueFactUtils::ValidateFactHandles(Facts, reportError);
}
#endif
