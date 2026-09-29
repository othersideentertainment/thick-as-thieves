// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Loot/TATLootTypes.h"
#include "Quests/TATQuestObjective.h"
#include "Quests/TATQuestObjectiveTracker.h"

#include "TATStealObjective.generated.h"

class UTATLootInventoryComponent;

// Steal (or somehow get) a single piece of loot
USTRUCT(meta = (DisplayName = "Steal Item"))
struct TAT_API FTATStealObjectiveInfo : public FTATQuestObjectiveInfo
{
   GENERATED_BODY()
public:
   UPROPERTY(EditAnywhere, meta = (ShowOnlyInnerProperties, Categories="Loot.Quest"))
   FTATLootIdentifier Item;

   // Objective text used in the metagame
   // Replacement keywords are {Item}
   // Default defined in Project Settings
   UPROPERTY(EditAnywhere, meta = (EditCondition="OverrideObjectiveText"))
   FText ObjectiveText;
   
   UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
   bool OverrideObjectiveText = false;

   // If used, it uses this as the quest location tag to spawn at instead of
   // the QuestLocationTag defined in the loot info
   UPROPERTY(EditAnywhere, meta = (Categories = "QuestLocation", EditCondition="ShouldOverrideQuestSpawnLocation"))
   FGameplayTag QuestSpawnLocation;

   UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
   bool ShouldOverrideQuestSpawnLocation = false;

   virtual void AddToActorSpawns(UWorld* world, TArray<FTATQuestActorSpawnRequest>& requests) const override;
   virtual FText GetObjectiveText(const UObject* worldContext) const override;
   virtual FGameplayTag GetRelatedLootTag() const override;
   virtual FTATObjectiveTrackerPayload CreateTracker() const override;
   virtual FString GetDebugDescription() const override { return FString::Printf(TEXT("Steal %s"), *Item.LootTag.ToString()); }
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif
};

UCLASS()
class TAT_API UTATStealObjectiveTracker : public UTATQuestObjectiveTracker
{
   GENERATED_BODY()

public:
   virtual void Initialize(const FTATObjectiveTrackerContext& context) override;
   virtual bool IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const override;
   virtual void CheatComplete() override;

private:
   void _RefreshObjectiveState();

   FTATLootIdentifier _lootIdentifier;

   UPROPERTY(Transient)
   TObjectPtr<UTATLootInventoryComponent> _inventory;
};

// To steal multiple of a single loot type, rather than multiple distinct ones
USTRUCT(meta = (DisplayName = "Steal N Items"))
struct TAT_API FTATStealMultipleObjectiveInfo : public FTATQuestObjectiveInfo
{
   GENERATED_BODY()
public:
   // The loot to be stolen
   UPROPERTY(EditAnywhere, meta = (Categories="Loot.Quest"))
   FTATLootIdentifier Item;
   
   // The number of copies the item that the player must have to complete the objective
   UPROPERTY(EditAnywhere, meta=(ClampMin=1, UIMin=1))
   int32 NumberToSteal = 1;

   // The number copies of the item that is spawned in the level
   // Should be >= NumberToSteal
   UPROPERTY(EditAnywhere, meta=(ClampMin=1, UIMin=1))
   int32 NumberToSpawn = 1;

   // Objective text used in the metagame
   // Replacement keywords are {Item} and {Quantity}
   UPROPERTY(EditAnywhere)
   FText ObjectiveText;

   // If used, it uses this as the quest location tag to spawn at instead of
   // the QuestLocationTag defined in the loot info
   UPROPERTY(EditAnywhere, meta = (Categories = "QuestLocation", EditCondition="ShouldOverrideQuestSpawnLocation"))
   FGameplayTag QuestSpawnLocation;

   UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
   bool ShouldOverrideQuestSpawnLocation = false;

   virtual void AddToActorSpawns(UWorld* world, TArray<FTATQuestActorSpawnRequest>& requests) const override;
   virtual FText GetObjectiveText(const UObject* worldContext) const override;
   virtual FGameplayTag GetRelatedLootTag() const override;
   virtual FTATObjectiveTrackerPayload CreateTracker() const override;
   virtual ETATQuestObjectiveProgressStyle GetProgressStyle() const override { return ETATQuestObjectiveProgressStyle::Integer; }
   virtual int32 GetTargetProgress() const override { return NumberToSteal; }
   virtual FString GetDebugDescription() const override { return FString::Printf(TEXT("Steal x%i %s"), NumberToSteal, *Item.LootTag.ToString()); }
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif
};

// CONSIDER: should replace UTATStealSingleObjectiveTracker with this?
//           It does a super-set of the things, but can't short-circuit
UCLASS()
class TAT_API UTATStealMultipleObjectiveTracker : public UTATQuestObjectiveTracker
{
   GENERATED_BODY()

public:
   virtual void Initialize(const FTATObjectiveTrackerContext& context) override;
   virtual bool IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const override;
   virtual void CheatComplete() override;

private:
   void _RefreshObjectiveState();

   FTATLootIdentifier _lootIdentifier;
   int32 _requiredCount;

   UPROPERTY(Transient)
   TObjectPtr<UTATLootInventoryComponent> _inventory;
};

USTRUCT()
struct TAT_API FTATStealSetEntry
{
   GENERATED_BODY()

   // The loot to be stolen
   UPROPERTY(EditAnywhere, meta = (Categories="Loot.Quest"))
   FTATLootIdentifier Item;

   // The number of copies of the item that is spawned in the level
   UPROPERTY(EditAnywhere, meta=(ClampMin=1, UIMin=1))
   int32 NumberToSpawn = 1;

   // If used, it uses this as the quest location tag to spawn at instead of
   // the QuestLocationTag defined in the loot info
   UPROPERTY(EditAnywhere, meta = (Categories = "QuestLocation"))
   FGameplayTag QuestSpawnLocation;
};

// To steal at least one of each of a set of items
USTRUCT(meta = (DisplayName = "Steal a Set"))
struct TAT_API FTATStealSetObjectiveInfo : public FTATQuestObjectiveInfo
{
   GENERATED_BODY()
public:
   UPROPERTY(EditAnywhere, meta=(TitleProperty="{Item} (N={NumberToSpawn})"))
   TArray<FTATStealSetEntry> ItemsToSteal;

   // The number of distinct items in the set that you have to steal
   // If not set, defaults to stealing the entire set
   UPROPERTY(EditAnywhere, meta=(EditCondition="OverrideMinNumberToSteal", ClampMin=1, UIMin=1))
   int32 MinNumberToSteal = 1;

   UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle))
   bool OverrideMinNumberToSteal = false;

   // Objective text used in the metagame
   UPROPERTY(EditAnywhere)
   FText ObjectiveText;

   virtual void AddToActorSpawns(UWorld* world, TArray<FTATQuestActorSpawnRequest>& requests) const override;
   virtual FText GetObjectiveText(const UObject* worldContext) const override;
   virtual void ForEachRelatedLootTag(TFunctionRef<void(const FGameplayTag&)> visitor) const override;
   virtual FTATObjectiveTrackerPayload CreateTracker() const override;
   virtual ETATQuestObjectiveProgressStyle GetProgressStyle() const override { return ETATQuestObjectiveProgressStyle::Integer; }
   virtual int32 GetTargetProgress() const override;
   virtual FString GetDebugDescription() const override;
   virtual void PopulateChildObjectives(const UObject* worldContext, TFunctionRef<void (const FTATMinimalObjective&)> callback) const override;
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif
};

UCLASS()
class TAT_API UTATAllChildrenObjectiveTracker : public UTATQuestObjectiveTracker
{
   GENERATED_BODY()

public:
   virtual void Initialize(const FTATObjectiveTrackerContext& context) override;
   virtual bool IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const override;
   virtual void CheatComplete() override;

private:
   UFUNCTION()
   void _OnChildCompletionChanged(bool isComplete, UTATPlayerObjective* objective);
   void _RefreshObjectiveState();

   UPROPERTY(Transient)
   TArray<TObjectPtr<UTATPlayerObjective>>  _children;
   int _requiredCount = 0;
};
