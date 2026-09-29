// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATBriefingClue.h"

// tat
#include "TATClueTextUtils.h"
#include "Variation/Clues/TATClueFact.h"
#include "Variation/Clues/TATClueSpawnUtils.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATBriefingClue)

FTATClueBucketKey FTATBriefingClueInfo::GetClueBucket() const
{
   // Briefing clues are distributed separately for now
   return FTATClueBucketKey {};
}

void FTATBriefingClueInfo::ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const
{
   // Briefing clues are distributed separately for now
   checkNoEntry();
}

FTATBriefingClueData FTATBriefingClueInfo::MakeBriefingData(const FTATClueContext& context, UWorld* world) const
{
   FText resolvedClueText = TATClueTextUtils::FormatUsingContext(ClueText, context, world);
   FTATSharedClueFactThunk factThunk = FTATClueFactThunk::Make(Facts, context);
   return FTATBriefingClueData {resolvedClueText, factThunk };
}

void FTATBriefingClueInfo::TakeFromRequest(FTATClueRequest& request, TArray<FTATBriefingClueData>& outBriefings, UWorld* world)
{
   request.Clues.RemoveAll([&outBriefings, &request, world](const FConstStructView& clue)
   {
      if (const FTATBriefingClueInfo* briefingInfo = clue.GetPtr<const FTATBriefingClueInfo>())
      {
         outBriefings.Add(briefingInfo->MakeBriefingData(request.Context, world));
         return true;
      }

      return false;
   });
}

#if WITH_EDITOR
void FTATBriefingClueInfo::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(ClueText.IsEmpty())
   {
      reportError(INVTEXT("ClueText is empty"));
   }
   
   ClueFactUtils::ValidateFactHandles(Facts, reportError);
}
#endif
