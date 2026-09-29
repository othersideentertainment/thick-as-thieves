// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueSpawnTypes.h"

// tat
#include "Variation/Clues/TATClueSetBase.h"
#include "Variation/Clues/TATClueSpawnUtils.h"

FTATClueRequest FTATPendingClueSource::IntoRequest() const
{
   FTATClueRequest request{
      .Context = {
         .Location = Location,
         .SourceTag = SourceTag,
         .SourceIndex = SourceIndex,
         .ExtraFormatParams = ExtraFormatParams },
      .Clues = {},
   };
   // NOTE: if clues end up coming from outside a single asset (as seen here, use an out param instead)
   // Assumption: ClueSet has been async loaded elsewhere (or somebody has chosen to sync load)
   FTATClueSetContext clueSetContext = {
      .ContextTags = ContextTags
   };
   // Not sure if there will ever be a case where ClueSet and LoadedClueSet are both valid,
   // but it's simpler to just handle it than it is to assert or silently leave it broken.
   if (UTATClueSetBase* clueSet = ClueSet.LoadSynchronous())
   {
      request.Clues = clueSet->FindRelevantClueViews(clueSetContext);
   }
   if (UTATClueSetBase* loadedClueSet = LoadedClueSet.Get())
   {
      request.Clues.Append(loadedClueSet->FindRelevantClueViews(clueSetContext));
   }
   return request;
}
