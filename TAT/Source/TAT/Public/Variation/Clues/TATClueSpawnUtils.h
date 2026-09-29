// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATClueSpawnTypes.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/StructView.h"
#include "UObject/WeakInterfacePtr.h"

struct FTATSceneVariantCollection;
class ITATClueLocationInterface;
class UTATClueSpawnerComponent;
using FTATClueInfoView = FConstStructView;

// Notes on the use of structs with non-owning pointers here:
//
// Constraints:
// 0. The clue types are pretty diverse, but routed mostly uniformly to spawners of a matching type.
// 1. This routing should still be fast, since it could be run 1000s of time in a simulation
// 2. Likely don't want to use instanced uobjects given the possible volume of data
// 3. It seems wasteful to box the clue data during this process, since they aren't going anywhere
//
// Given this, the clue spawn plan is uses structs with non-owning pointers.
// This does require a little care to do so safely (don't hold onto any of this),
// but given that there are probably 2-3 places where this is would happen,
// I am not especially concerned at the moment. (but did feel it justified
// writing a paragraph on)

// CLUE-WIP: Should this be moved elsewhere, since used by other things?
struct FTATClueContext
{
   // expected to be non-null (even if dummy)
   TWeakInterfacePtr<ITATClueLocationInterface> Location;
   // Tag representing the source of the clue (e.g. the clue tag)
   // Not required
   FGameplayTag SourceTag;
   // An extra number disambiguating the source
   int32 SourceIndex = 0;
   // Additional format params from context
   FTATSharedClueFormatParams ExtraFormatParams;
};

// NOTE: transient struct not expected to live beyond a stack frame
struct FTATClueRequest
{
   FTATClueContext Context;
   TArray<FTATClueInfoView> Clues;
};

// NOTE: transient struct not expected to live beyond a stack frame
struct TAT_API FTATClueSpawnParams
{
   TConstArrayView<FTATClueRequest> SpawnRequests;
   TConstArrayView<TObjectPtr<UTATClueSpawnerComponent>> Spawners;
   const FTATSceneVariantCollection* Variants = nullptr;
   int32 Seed = 0;

   FTATClueSpawnParams() = default;
   UE_NONCOPYABLE(FTATClueSpawnParams)
};

// NOTE: transient struct not expected to live beyond a stack frame
struct FTATClueSpawnPlan
{
   struct FSpawnerEntry
   {
      UTATClueSpawnerComponent* Spawner = nullptr;
      int32 ClueCount = 0;
   };

   struct FClueEntry
   {
      FTATClueInfoView Clue;
      FTATClueContext Context;
   };

   void AddClue(const FTATClueContext& context, const FTATClueInfoView& clue);
   void AddSpawnerWithClue(UTATClueSpawnerComponent* spawner, const FTATClueInfoView& clue, const FTATClueContext& context);
   void Execute() const;
   void WriteFailuresToLog(FMessageLog& messageLog) const;

   TArray<FSpawnerEntry> Spawners;
   TArray<FClueEntry> Clues;

   TArray<FClueEntry> FailedClues;
};

namespace TATClueSpawnUtils
{
   FTATClueSpawnPlan GeneratePlan(const FTATClueSpawnParams& params);
}
