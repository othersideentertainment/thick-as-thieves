// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Unlockables/TATAvailableUnlocksComponent.h"

// tat
#include "SaveGame/TATSaveGame.h"
#include "Unlockables/TATUnlockableContent.h"
#include <Quests/TATThievesDenQuestSubsystem.h>
#include "Quests/TATQuestInfo.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAvailableUnlocksComponent)

struct FTATUnlockableContent;

// Sets default values for this component's properties
UTATAvailableUnlocksComponent::UTATAvailableUnlocksComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UTATAvailableUnlocksComponent::BeginPlay()
{
   Super::BeginPlay();


   if (!ensure(_unlockableContent))
   {
      return;
   }

   // Note: the save game should be available in the thieves den, but that may not be true
   //       if something goes directly into that level (outside of PIE, where that always works)
   //       Not handling that for now, but could bind to ose save game initialization flow if needed.

   _saveGame = UTATSaveGame::GetTATSaveGame(this);
   if (!ensure(_saveGame))
   {
      return;
   }

   // If FTUE is ongoing, skip it (even if it laters completes while in level)
   if (_saveGame->GetFtueState() != ETATSavedFtueState::Complete)
   {
      return;
   }

   _saveGame->OnUnlocksChanged.AddUniqueDynamic(this, &ThisClass::_ScheduleRefreshUnlocks);
   _saveGame->OnSeenUnlocksChanged.AddUObject(this, &ThisClass::_ScheduleRefreshUnlocks);
   _saveGame->OnContractsChanged.AddUniqueDynamic(this, &ThisClass::_ScheduleRefreshUnlocks);
   _saveGame->OnXPProgressionChanged.AddUniqueDynamic(this, &ThisClass::_ScheduleRefreshUnlocks);
   if (_checkMoneyCost)
   {
      _saveGame->GetPlayerProgressionMoneyChangedEvent().AddUObject(this, &ThisClass::_ScheduleRefreshUnlocks);
   }

   if (_skipIfFairyOutro)
   {
      if (UTATThievesDenQuestSubsystem* questSubsystem = GetWorld()->GetSubsystem<UTATThievesDenQuestSubsystem>())
      {
         questSubsystem->OnCompletableContractsChanged.AddUObject(this, &ThisClass::_ScheduleRefreshUnlocks);
      }
   }

   _RefreshUnlocks();
}

void UTATAvailableUnlocksComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (_saveGame)
   {
      _saveGame->OnUnlocksChanged.RemoveAll(this);
      _saveGame->OnSeenUnlocksChanged.RemoveAll(this);
      _saveGame->OnContractsChanged.RemoveAll(this);
      _saveGame->OnXPProgressionChanged.RemoveAll(this);
      _saveGame->GetPlayerProgressionMoneyChangedEvent().RemoveAll(this);
   }

   if (UTATThievesDenQuestSubsystem* questSubsystem = GetWorld()->GetSubsystem<UTATThievesDenQuestSubsystem>())
   {
      questSubsystem->OnCompletableContractsChanged.RemoveAll(this);
   }
   
   Super::EndPlay(endPlayReason);
}

void UTATAvailableUnlocksComponent::MarkAvailableUnlocksSeen(const UObject* worldContext, const UTATUnlockableContentDataAsset* unlockData)
{
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(worldContext);
   if (!ensure(saveGame))
   {
      return;
   }

   if (!ensure(unlockData))
   {
      return;
   }

   auto isUnlockable = [saveGame](const FTATUnlockableContent& content) -> bool
      {
         const FGameplayTag contentTag = content.Tag;
         return contentTag.IsValid()
            && !saveGame->HasUnlockedContent(contentTag)
            && saveGame->GetAvailabilityFromUnlockRequirement(content.UnlockRequirement) == ETATFeatureAvailabilityToPlayer::Available;
      };

   const FTATPlayerProgression& progression = saveGame->GetPlayerProgression();
   TArray<FGameplayTag, TInlineAllocator<16>> newSeenUnlocks;
   for (const FTATUnlockableContent& content : unlockData->EntryTable)
   {
      if (isUnlockable(content) && !progression.SeenUnlockableContent.Contains(content.Tag))
      {
         newSeenUnlocks.Add(content.Tag);
      }
   }
   if (newSeenUnlocks.Num() > 0)
   {
      saveGame->MarkUnlocksSeen(newSeenUnlocks);
   }
}

void UTATAvailableUnlocksComponent::_ScheduleRefreshUnlocks()
{
   if (!_refreshTimerHandle.IsValid())
   {
      _refreshTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UTATAvailableUnlocksComponent::_RefreshUnlocks);
   }
}

void UTATAvailableUnlocksComponent::_RefreshUnlocks()
{
   _refreshTimerHandle.Invalidate();

   const bool hasAvailable = _CalculateHasAvailableUnlocks();
   if (hasAvailable != _hasAvailableUnlocks)
   {
      _hasAvailableUnlocks = hasAvailable;
      OnAvailableUnlocksChanged.Broadcast(_hasAvailableUnlocks);
   }
}

bool UTATAvailableUnlocksComponent::_CalculateHasAvailableUnlocks() const
{
   if (!ensure(_saveGame))
   {
      return false;
   }

   check(_unlockableContent);

   if (_skipIfFairyOutro)
   {
      if (const UTATThievesDenQuestSubsystem* questSubsystem = GetWorld()->GetSubsystem<UTATThievesDenQuestSubsystem>())
      {
         if (questSubsystem->GetCompletableContractForFlow(ETATContractOutroFlow::Fairy).IsValid())
         {
            return false;
         }
      }
   }

   auto isUnlockable = [this](const FTATUnlockableContent& content) -> bool
   {
      const FGameplayTag contentTag = content.Tag;
      if (!contentTag.IsValid())
      {
         return false;
      }

      if (_saveGame->HasUnlockedContent(contentTag) || _saveGame->GetPlayerProgression().SeenUnlockableContent.Contains(contentTag))
      {
         return false;
      }

      if (_saveGame->GetAvailabilityFromUnlockRequirement(content.UnlockRequirement) != ETATFeatureAvailabilityToPlayer::Available)
      {
         return false;
      }

      if (_checkMoneyCost && _saveGame->GetMoney() < content.Price)
      {
         return false;
      }
      
      return true;
   };

   return _unlockableContent->EntryTable.ContainsByPredicate(isUnlockable);
}


