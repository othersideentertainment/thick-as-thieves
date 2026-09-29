// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATStealObjective.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATMatchPersistentTypes.h"
#include "Loot/TATLootActor.h"
#include "Loot/TATLootInventory.h"
#include "Quests/Spawn/TATQuestSpawnTypes.h"
#include "Quests/TATPlayerObjective.h"

// ue
#include "Algo/Count.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStealObjective)

struct FTATStealTrackerParams : public FTATObjectiveTrackerParams
{
   TAT_DEFINE_TRACKER_PARAMS(FTATStealTrackerParams)
   
   FTATLootIdentifier Item;
};

struct FTATStealMultipleTrackerParams : public FTATObjectiveTrackerParams
{
   TAT_DEFINE_TRACKER_PARAMS(FTATStealMultipleTrackerParams)
   
   FTATLootIdentifier Item;
   int32 Count = 0;
};

struct FTATCompleteChildrenTrackerParams : public FTATObjectiveTrackerParams
{
   TAT_DEFINE_TRACKER_PARAMS(FTATCompleteChildrenTrackerParams)
   
   int32 Count = 0;
};

void FTATStealObjectiveInfo::AddToActorSpawns(UWorld* world, TArray<FTATQuestActorSpawnRequest>& requests) const
{
   check(world);
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(world, Item);
   if (lootInfo == nullptr)
   {
      return;
   }

   FGameplayTag locationTag = ShouldOverrideQuestSpawnLocation ? QuestSpawnLocation : lootInfo->QuestLocationTag;

   requests.AddUnique({lootInfo->ActorClass, Item.LootTag, locationTag});
}

FText FTATStealObjectiveInfo::GetObjectiveText(const UObject* worldContext) const
{
   const FTATLootInfo* lootInfo = (worldContext != nullptr) ? UTATLootSettings::Get().FindLootInfo(worldContext, Item) : nullptr;

   const FText& templateText = OverrideObjectiveText
      ? ObjectiveText
      : UTATProjectSettings::Get().DefaultStealItemObjectiveText;

   return FText::FormatNamed(templateText,
      TEXT("Item"), lootInfo ? lootInfo->DisplayName : FText());
}

FGameplayTag FTATStealObjectiveInfo::GetRelatedLootTag() const
{
   return Item.LootTag;
}

FTATObjectiveTrackerPayload FTATStealObjectiveInfo::CreateTracker() const
{
   FTATStealTrackerParams params;
   params.Item = Item;
   return FTATObjectiveTrackerPayload{ UTATStealObjectiveTracker::StaticClass(),  MakeShared<FTATStealTrackerParams>(params) };
}

#if WITH_EDITOR
void FTATStealObjectiveInfo::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if (!Item.IsValid())
   {
      reportError(INVTEXT("Item objective has no item tag"));
   }

   if (OverrideObjectiveText && ObjectiveText.IsEmpty())
   {
      reportError(INVTEXT("Objective text is overridden but empty"));
   }

   if (ShouldOverrideQuestSpawnLocation && !QuestSpawnLocation.IsValid())
   {
      reportError(INVTEXT("Overriding QuestSpawnLocation, but not set to anything"));
   }
}
#endif

void UTATStealObjectiveTracker::Initialize(const FTATObjectiveTrackerContext& context)
{
   const FTATStealTrackerParams* itemParams = context.GetParams<FTATStealTrackerParams>();
   _lootIdentifier = itemParams->Item;

   _inventory = UTATLootInventoryComponent::GetForActor(context.PlayerState);
   if (ensure(_inventory))
   {
      _inventory->OnAuthorityInventoryOrStashChanged.AddUObject(this, &UTATStealObjectiveTracker::_RefreshObjectiveState);
      _RefreshObjectiveState();
   }
}

bool UTATStealObjectiveTracker::IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const
{
   // TODO: Either assume that any player-specific objectives would be excluded from the team loot, or parameterize it so it sometimes does not check
   return matchData.CarriedLoot.Contains(_lootIdentifier) || matchData.AllyCarriedLoot.Contains(_lootIdentifier);
}

void UTATStealObjectiveTracker::CheatComplete()
{
   if (IsComplete())
   {
      return;
   }

   // Just actually give the desired item
   // That way any later parts of the flow still work,
   // and there isn't a different book-keeping
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, _lootIdentifier);
   if (!ensure(lootInfo) || _inventory == nullptr)
   {
      return;
   }

   // Just hopes there is enough inventory space, but the
   // player should hopefully be able to resolve that themselves
   FTATLootItemVariant lootItem(_lootIdentifier);
   if (lootInfo->RequiresInstanceStorage())
   {
      lootItem.Set(lootInfo->CreateDefaultInstance(this));
   }
   _inventory->AuthorityTryPickupLootItem(lootItem);
}

void UTATStealObjectiveTracker::_RefreshObjectiveState()
{
   check(_inventory);
   _SetIsComplete(_inventory->AuthorityHasLoot(_lootIdentifier));
}

void FTATStealMultipleObjectiveInfo::AddToActorSpawns(UWorld* world, TArray<FTATQuestActorSpawnRequest>& requests) const
{
   check(world);
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(world, Item);
   if (lootInfo == nullptr)
   {
      return;
   }

   FGameplayTag locationTag = ShouldOverrideQuestSpawnLocation ? QuestSpawnLocation : lootInfo->QuestLocationTag;

   FTATQuestActorSpawnRequest newRequest = { lootInfo->ActorClass, Item.LootTag, locationTag };

   const int32 currentCount = Algo::Count(requests, newRequest);
   const int32 neededCount = NumberToSpawn - currentCount;
   for (int i = 0; i < neededCount; ++i)
   {
      requests.Add(newRequest);
   }
}

FText FTATStealMultipleObjectiveInfo::GetObjectiveText(const UObject* worldContext) const
{
   const FTATLootInfo* lootInfo = (worldContext != nullptr) ? UTATLootSettings::Get().FindLootInfo(worldContext, Item) : nullptr;

   const FText& templateText = ObjectiveText;

   return FText::FormatNamed(templateText,
      TEXT("Item"), lootInfo ? lootInfo->DisplayName : FText(),
      TEXT("Quantity"), NumberToSteal);
}

FGameplayTag FTATStealMultipleObjectiveInfo::GetRelatedLootTag() const
{
   return Item.LootTag;
}

FTATObjectiveTrackerPayload FTATStealMultipleObjectiveInfo::CreateTracker() const
{
   FTATStealMultipleTrackerParams params;
   params.Item = Item;
   params.Count = NumberToSteal;
   return FTATObjectiveTrackerPayload{ UTATStealMultipleObjectiveTracker::StaticClass(),  MakeShared<FTATStealMultipleTrackerParams>(params) };
}

#if WITH_EDITOR
void FTATStealMultipleObjectiveInfo::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if (!Item.IsValid())
   {
      reportError(INVTEXT("Item objective has no item tag"));
   }

   if (NumberToSteal > NumberToSpawn)
   {
      reportError(INVTEXT("NumberToSteal is greater than NumberToSpawn, so objective cannot be completed"));
   }

   if (ObjectiveText.IsEmpty())
   {
      reportError(INVTEXT("Objective text is overridden but empty"));
   }

   if (ShouldOverrideQuestSpawnLocation && !QuestSpawnLocation.IsValid())
   {
      reportError(INVTEXT("Overriding QuestSpawnLocation, but not set to anything"));
   }
}
#endif

void UTATStealMultipleObjectiveTracker::Initialize(const FTATObjectiveTrackerContext& context)
{
   const FTATStealMultipleTrackerParams* itemParams = context.GetParams<FTATStealMultipleTrackerParams>();
   _lootIdentifier = itemParams->Item;
   _requiredCount = itemParams->Count;

   _inventory = UTATLootInventoryComponent::GetForActor(context.PlayerState);
   if (ensure(_inventory))
   {
      _inventory->OnAuthorityInventoryOrStashChanged.AddUObject(this, &UTATStealMultipleObjectiveTracker::_RefreshObjectiveState);
      _RefreshObjectiveState();
   }
}

bool UTATStealMultipleObjectiveTracker::IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const
{
   // TODO: Either assume that any player-specific objectives would be excluded from the team loot, or parameterize it so it sometimes does not check
   const int32 total = Algo::Count(matchData.CarriedLoot, _lootIdentifier)
      + Algo::Count(matchData.AllyCarriedLoot, _lootIdentifier);
   return total >= _requiredCount;
}

void UTATStealMultipleObjectiveTracker::CheatComplete()
{
   if (IsComplete())
   {
      return;
   }

   // Just actually give the desired item
   // That way any later parts of the flow still work,
   // and there isn't a different book-keeping
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, _lootIdentifier);
   if (!ensure(lootInfo) || _inventory == nullptr)
   {
      return;
   }

   // Just hopes there is enough inventory space, but the
   // player should hopefully be able to resolve that themselves
   FTATLootItemVariant lootItem(_lootIdentifier);
   if (lootInfo->RequiresInstanceStorage())
   {
      lootItem.Set(lootInfo->CreateDefaultInstance(this));
   }

   const int32 needed = _requiredCount - _inventory->AuthorityCountLoot(_lootIdentifier);
   for (int i = 0; i < needed; ++i)
   {
      _inventory->AuthorityTryPickupLootItem(lootItem);
   }
}

void UTATStealMultipleObjectiveTracker::_RefreshObjectiveState()
{
   check(_inventory);
   const int32 haveCount = _inventory->AuthorityCountLoot(_lootIdentifier);
   // TODO: should this clamp at required?
   _SetProgress(FTATQuestObjectiveState { .IsComplete = haveCount >= _requiredCount, .Progress = haveCount });
}

void FTATStealSetObjectiveInfo::AddToActorSpawns(UWorld* world, TArray<FTATQuestActorSpawnRequest>& requests) const
{
   check(world);
   for (const FTATStealSetEntry& entry : ItemsToSteal)
   {
      const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(world, entry.Item);
      if (lootInfo == nullptr)
      {
         return;
      }

      FGameplayTag locationTag = entry.QuestSpawnLocation.IsValid() ? entry.QuestSpawnLocation : lootInfo->QuestLocationTag;

      FTATQuestActorSpawnRequest newRequest = { lootInfo->ActorClass, entry.Item.LootTag, locationTag };

      const int32 currentCount = Algo::Count(requests, newRequest);
      const int32 neededCount = entry.NumberToSpawn - currentCount;
      for (int i = 0; i < neededCount; ++i)
      {
         requests.Add(newRequest);
      }
   }
}

FText FTATStealSetObjectiveInfo::GetObjectiveText(const UObject* worldContext) const
{
   return ObjectiveText;
}

void FTATStealSetObjectiveInfo::ForEachRelatedLootTag(TFunctionRef<void(const FGameplayTag&)> visitor) const
{
   for (const FTATStealSetEntry& entry : ItemsToSteal)
   {
      visitor(entry.Item.LootTag);
   }
}

FTATObjectiveTrackerPayload FTATStealSetObjectiveInfo::CreateTracker() const
{
   FTATCompleteChildrenTrackerParams params;
   params.Count = GetTargetProgress();
   return FTATObjectiveTrackerPayload {
      .TrackerClass = UTATAllChildrenObjectiveTracker::StaticClass(),
      .Params = MakeShared<FTATCompleteChildrenTrackerParams>(params)
   };
}

int32 FTATStealSetObjectiveInfo::GetTargetProgress() const
{
   return OverrideMinNumberToSteal ? MinNumberToSteal : ItemsToSteal.Num();
}

FString FTATStealSetObjectiveInfo::GetDebugDescription() const
{
   return FString::Printf(TEXT("Steal a Set(%d)"), ItemsToSteal.Num());
}

void FTATStealSetObjectiveInfo::PopulateChildObjectives(const UObject* worldContext, TFunctionRef<void(const FTATMinimalObjective&)> callback) const
{
   for (const FTATStealSetEntry& entry : ItemsToSteal)
   {
      const FTATLootInfo* lootInfo = (worldContext != nullptr) ? UTATLootSettings::Get().FindLootInfo(worldContext, entry.Item) : nullptr;

      const FText& templateText = UTATProjectSettings::Get().DefaultStealItemObjectiveText;

      FTATStealTrackerParams params;
      params.Item = entry.Item;
      
      callback(FTATMinimalObjective{
         .Id = reinterpret_cast<UPTRINT>(&entry), // NB: pointer-to-int cast is well-defined, just not the other direction
         .ObjectiveText = FText::FormatNamed(templateText, TEXT("Item"), lootInfo ? lootInfo->DisplayName : FText()),
         .Tracker = FTATObjectiveTrackerPayload{ UTATStealObjectiveTracker::StaticClass(),  MakeShared<FTATStealTrackerParams>(params) }
      });
   }
}

#if WITH_EDITOR
void FTATStealSetObjectiveInfo::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   for (int i = 0; i < ItemsToSteal.Num(); ++i)
   {
      const FTATStealSetEntry& entry = ItemsToSteal[i];
      auto reportEntryError = [&reportError, i ](const FText& message)
      {
         reportError(FText::FormatOrdered(INVTEXT("Entry[{0}]: {1}"), i, message));
      };
      
      if (!entry.Item.IsValid())
      {
         reportEntryError(INVTEXT("Loot not specified"));
      }

      if (!entry.QuestSpawnLocation.IsValid())
      {
         reportEntryError(INVTEXT("QuestSpawnLocation not specified"));
      }
   }

   if(OverrideMinNumberToSteal && MinNumberToSteal > ItemsToSteal.Num())
   {
      reportError(FText::FormatOrdered(INVTEXT("MinNumberToSteal > ItemsToSteal ({0} > {1})"), MinNumberToSteal, ItemsToSteal.Num()));
   }
}
#endif

void UTATAllChildrenObjectiveTracker::Initialize(const FTATObjectiveTrackerContext& context)
{
   const FTATCompleteChildrenTrackerParams* params = context.GetParams<FTATCompleteChildrenTrackerParams>();
   _requiredCount = params->Count;
   _children = context.ChildObjectives;
   for (UTATPlayerObjective* child : _children)
   {
      if (child)
      {
         child->OnCompleteChanged.AddUniqueDynamic(this, &ThisClass::_OnChildCompletionChanged);
      }
   }
   _RefreshObjectiveState();
}

bool UTATAllChildrenObjectiveTracker::IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const
{
   int completeCount = 0;
   for (UTATPlayerObjective* child : _children)
   {
      if (child->IsCompleteForMatchEnd(escaped, matchData))
      {
         completeCount += 1;
      }
   }

   return completeCount >= _requiredCount;
}

void UTATAllChildrenObjectiveTracker::CheatComplete()
{
   for (UTATPlayerObjective* child : _children)
   {
      child->AuthorityCheatComplete();
   }
}

void UTATAllChildrenObjectiveTracker::_OnChildCompletionChanged(bool isComplete, UTATPlayerObjective* objective)
{
   _RefreshObjectiveState();
}

void UTATAllChildrenObjectiveTracker::_RefreshObjectiveState()
{
   const int32 completeCount = Algo::CountIf(_children, [] (const UTATPlayerObjective* child) { return child && child->IsComplete();});
   _SetProgress(FTATQuestObjectiveState {
      .IsComplete = completeCount >= _requiredCount,
      .Progress = completeCount
   });
}
