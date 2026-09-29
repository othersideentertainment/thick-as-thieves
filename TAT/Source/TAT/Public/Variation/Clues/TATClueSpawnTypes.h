// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/WeakInterfacePtr.h"

class ITATClueLocationInterface;
class UTATClueSetBase;
struct FTATClueRequest;

// extra format params that can be provided from context to be injected into clue text
// (e.g. a lock combination)
//
// Notes:
// 1. Passed as SharedPtr to const, since it will be copied around a lot
// 2. NotThreadSafe for no particular reason
// 3. Might not stay just a TSortedMap<FString, FText> forever, but should be easy-ish to change later
using FTATClueFormatParams = TSortedMap<FString, FText>;
using FTATSharedClueFormatParams = TSharedPtr<const FTATClueFormatParams, ESPMode::NotThreadSafe>;
inline FTATSharedClueFormatParams MakeSharedClueFormatParams(FTATClueFormatParams&& params)
{
   return MakeShared<FTATClueFormatParams, ESPMode::NotThreadSafe>(MoveTemp(params));
}

// A struct representing a clue source that may be async loading its clues
struct FTATPendingClueSource
{
   TSoftObjectPtr<UTATClueSetBase> ClueSet;
   TWeakObjectPtr<UTATClueSetBase> LoadedClueSet;
   TWeakInterfacePtr<ITATClueLocationInterface> Location;
   
   // Tag representing the source of the clue (e.g. the clue tag)
   // Not required
   FGameplayTag SourceTag;
   int32 SourceIndex = 0;

   FTATSharedClueFormatParams ExtraFormatParams;
   FGameplayTagContainer ContextTags;

   FTATClueRequest IntoRequest() const;
};

