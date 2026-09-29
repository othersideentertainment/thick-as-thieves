// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueSetBase.h"

// ue
#include "StructUtils/StructView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueSetBase)

TArray<FConstStructView> UTATClueSetBase::FindRelevantClueViews(const FTATClueSetContext& context) const
{
   TArray<FConstStructView> result;
   AddRelevantClueViews(context, result);
   // assuming NRVO (definitely at least a move)
   return result;
}
