// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/PostMatch/TATPostMatchFeedbackTipTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPostMatchFeedbackTipTypes)

bool UTATPostMatchFeedbackTipDataAsset::SelectFeedbackTip(const FMatchPersistentData& matchPersistentData, const ATATPlayerState* playerState, FTATPostMatchFeedbackTipEntry& postMatchFeedbackTip) const
{
   if (Tips.IsEmpty())
   {
      postMatchFeedbackTip = FTATPostMatchFeedbackTipEntry();
      return false;
   }
   const int32 randomIndex = FMath::RandRange(0, Tips.Num() - 1);
   postMatchFeedbackTip = Tips[randomIndex];
   return true;
}
