// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATClueSpawnTypes.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"

#include "TATClueFact.generated.h"

struct FTATClueFactThunk;
struct FTATClueContext;
struct FTATClueFactSpec;

USTRUCT()
struct FTATClueFactNamespace
{
   GENERATED_BODY()

   UPROPERTY()
   FGameplayTag SourceTag;

   UPROPERTY()
   int32 SourceIndex = 0;

   bool operator==(const FTATClueFactNamespace& other) const = default;

   friend uint32 GetTypeHash(const FTATClueFactNamespace& n)
   {
      return HashCombineFast(GetTypeHash(n.SourceTag), GetTypeHash(n.SourceIndex));
   }
};

// Clue Facts are atoms of logical information that a clue conveys
// (separate from their primary form)
//
// 1. Multiple clues can have the same fact, and these are deduplicated
// 2. A clue can have multiple facts
//
// When a player interacts with a clue, the facts are added
// the player's known clue facts. These are then visible in
// the clue journal. It also may be used to highlight the subject
// of the clues.
//
// NOTE: For TOD, the clue facts are forced to assume that fact tags uniquely-enough
//       identify a fact. This breaks when there are sources of clues outside the mono-quest.
//       But because the TOD quest colors outside the lines, we must treat all
//       facts as being in the same namespace. Here lies tech debt.


// A runtime bundle of facts from a clue, so that they can be registered when seen by the player
//
// Notes:
// 1. Passed as SharedPtr to const, since it will be copied through things that cannot move
// 2. NotThreadSafe for no particular reason
using FTATSharedClueFactThunk = TSharedPtr<const FTATClueFactThunk, ESPMode::NotThreadSafe>;
struct FTATClueFactThunk
{
   explicit FTATClueFactThunk(const TArray<FTATClueFactSpec>& factSpecs, const FTATClueContext& context);
   explicit FTATClueFactThunk(const TArray<FDataTableRowHandle>& factSpecs, const FTATClueContext& context);
   
   static FTATSharedClueFactThunk Make(const TArray<FTATClueFactSpec>& factSpecs, const FTATClueContext& context)
   {
      return MakeShared<FTATClueFactThunk, ESPMode::NotThreadSafe>(factSpecs, context);
   }

   static FTATSharedClueFactThunk Make(const TArray<FDataTableRowHandle>& factSpecs, const FTATClueContext& context)
   {
      return MakeShared<FTATClueFactThunk, ESPMode::NotThreadSafe>(factSpecs, context);
   }
   
   int32 GetFactCount() const { return _specs.Num(); }
   FGameplayTag GetFactTagAt(int32 index) const;
   const FGameplayTag& GetSourceTag() const { return _sourceTag; }
   int32 GetSourceIndex() const { return _sourceIndex; }
   FText GetFactTextAt(int32 index, UObject* worldContext) const;
   // could maybe use a little more insulation, but there is only
   // one usage, so not too hard to change if needed
   const FTATClueFactSpec& GetFactSpecAt(int32 index) const;
private:
   TArray<FTATClueFactSpec> _specs;
   FText _locationName;
   FGameplayTag _sourceTag;
   int32 _sourceIndex = 0;
   FTATSharedClueFormatParams _formatParams;
};

namespace ClueFactUtils
{
#if WITH_EDITOR
   void ValidateFactHandles(TConstArrayView<FDataTableRowHandle> factHandles, TFunctionRef<void(const FText&)> reportError);
#endif

   FGameplayTagContainer TagsFromThunk(const FTATClueFactThunk* thunk);
}

// A fact that a given clue grants
USTRUCT()
struct FTATClueFactSpec : public FTableRowBase
{
   GENERATED_BODY()

   // The tag that identifies this fact. Should represent the same thing
   // for all actually-chosen clues that use this fact. It is okay if
   // clues that are not chosen use the tag to mean different things.
   UPROPERTY(EditAnywhere, meta=(Categories="ClueFact"))
   FGameplayTag FactTag;

   // The journal text equivalent of this fact
   // may include replacement, if applicable
   UPROPERTY(EditAnywhere, meta=(MultiLine))
   FText JournalText;

   // The display priority of this clue in ascending order
   // (0 = highest priority, although I don't disallow negative)
   UPROPERTY(EditAnywhere)
   int32 DisplayPriority = 2;

   // The category of this fact
   // Could be used to group and organize the display of facts
   UPROPERTY(EditAnywhere, meta=(Categories="ClueFactCategory"))
   FGameplayTag Category;
   
   // This fact is suppressed (e.g. hidden) if a fact matching any of these tags is known
   // (includes subtag matches)
   UPROPERTY(EditAnywhere, meta=(Categories="ClueFact"))
   FGameplayTagContainer SuppressedByFacts;
};
