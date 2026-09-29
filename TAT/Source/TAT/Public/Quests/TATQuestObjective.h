// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTagContainer.h"

#include "TATQuestObjective.generated.h"

class UTATQuestObjectiveTracker;
struct FTATQuestActorSpawnRequest;
struct FTATObjectiveTrackerParams;
struct FTATMinimalObjective;


struct FTATObjectiveTrackerPayload
{
   TSoftClassPtr<UTATQuestObjectiveTracker> TrackerClass = nullptr;
   TSharedPtr<FTATObjectiveTrackerParams> Params;
};

// Just how progress should be displayed
UENUM(BlueprintType)
enum class ETATQuestObjectiveProgressStyle : uint8
{
   Binary,
   Integer
};

USTRUCT()
struct TAT_API FTATQuestObjectiveInfo
{
   GENERATED_BODY()

public:
   virtual ~FTATQuestObjectiveInfo() {}

   virtual void AddToActorSpawns(UWorld* world, TArray<FTATQuestActorSpawnRequest>& requests) const {}

   // Tag of related loot/item to the objective (if any)
   // NOTE: I could see a more generic tag, but there is probably still
   //       a desire to know the loot specifically, since parts of the
   //       meta involve interactions with the item that was stolen.
   // NOTE: Using FGameplayTag over loot identifier just to avoid the
   //       extra include, which may or may not be a good reason.
   // TODO: Wean ourselves off uses of the singular form of GetRelatedLootTag
   virtual FGameplayTag GetRelatedLootTag() const;
   virtual void ForEachRelatedLootTag(TFunctionRef<void(const FGameplayTag&)> visitor) const;

   // Get the brief imperative version of the objective
   // TODO: maybe remove if not needed
   virtual FText GetObjectiveText(const UObject* worldContext) const;

   // q: is this indirection overkill? I could just return the thing, this just allows external async loading
   virtual FTATObjectiveTrackerPayload CreateTracker() const;

   virtual ETATQuestObjectiveProgressStyle GetProgressStyle() const { return ETATQuestObjectiveProgressStyle::Binary; }
   // optional maximum progress if integer style.
   // If this can ever vary, move to the replicated payload
   virtual int32 GetTargetProgress() const { return 0; }

   // Gets a short description of this objective for editor/debug purposes
   virtual FString GetDebugDescription() const { return FString(); }

   // NOTE: This does work/allocations
   FTATMinimalObjective ToMinimalObjective(const UObject* worldContext) const;

   // NOTE: This does work/allocations
   virtual void PopulateChildObjectives(const UObject* worldContext, TFunctionRef<void (const FTATMinimalObjective&)> callback) const {}

#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const {}
#endif

};

struct FTATMinimalObjective
{
   // An opaque id representing the source. Possibly just the address of the data
   uint64 Id;
   FText ObjectiveText;
   FTATObjectiveTrackerPayload Tracker;
   int32 TargetProgress = 0;
   ETATQuestObjectiveProgressStyle ProgressStyle = ETATQuestObjectiveProgressStyle::Binary;
};
