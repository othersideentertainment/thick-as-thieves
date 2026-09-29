// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueSpawnTypes.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FTATClueContext;

struct FTATClueTextFormatContext
{
   // NOTE: Just noncopyable to make the ExtraFormatParams not easily misused
   //       but could have just kept it as a shared pointer.
   FTATClueTextFormatContext() = default;
   UE_NONCOPYABLE(FTATClueTextFormatContext);

   FText LocationName;
   FGameplayTag SourceTag;
   const FTATClueFormatParams* ExtraFormatParams = nullptr;
};

namespace TATClueTextUtils
{
   FText FindItemName(FGameplayTag sourceTag, UObject* worldContext);
   void AddItemNameToParams(FTATClueFormatParams& params, FGameplayTag sourceTag, UObject* worldContext);

   FText FormatUsingContext(const FText& format, const FTATClueContext& context, UObject* worldContext);
   FText FormatUsingContext(const FText& format, const FTATClueTextFormatContext& context, UObject* worldContext);
}
