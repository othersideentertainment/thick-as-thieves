// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATQuestDataSubsystem.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Quests/TATQuestHandle.h"
#include "Quests/TATQuestInfo.h"
#include "Quests/TATQuestTags.h"

// ue
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestDataSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATQuestDataSubsystem, Log, All);

namespace QuestDataHelpers
{
   template<typename T>
   static const T* LookupQuest(const UDataTable* questTable, const FGameplayTag& questTag)
   {
      if (questTable == nullptr)
      {
         UE_LOG(LogTATQuestDataSubsystem, Warning, TEXT("No quest data table"));
         return nullptr;
      }

      return questTable->FindRow<T>(questTag.GetTagName(), TEXT("FindQuestInfo"));
   }
}

void UTATQuestDataSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
   
   // try to load async if possible
   TArray<FSoftObjectPath> pathsToLoad = {
      UTATProjectSettings::Get().GetDataTableForMissions().ToSoftObjectPath(),
      UTATProjectSettings::Get().ContractDataTable.ToSoftObjectPath()
   };
   UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(pathsToLoad), [weakThis = MakeWeakObjectPtr(this)]()
      {
         if (UTATQuestDataSubsystem* self = weakThis.Get())
         {
            self->_missionData = UTATProjectSettings::Get().GetDataTableForMissions().Get();
            self->_contractData = UTATProjectSettings::Get().ContractDataTable.Get();
         }
      });
}

const UTATQuestDataSubsystem& UTATQuestDataSubsystem::Get(const UObject* worldContext)
{
   check(worldContext);
   UWorld* world = worldContext->GetWorld();
   check(world);
   UGameInstance* gameInstance = world->GetGameInstance();
   check(gameInstance);
   const UTATQuestDataSubsystem* subsystem = gameInstance->GetSubsystem<UTATQuestDataSubsystem>();
   check(subsystem);
   return *subsystem;
}

const UTATQuestDataSubsystem* UTATQuestDataSubsystem::TryGet(const UObject* worldContext)
{
   check(worldContext);
   UWorld* world = worldContext->GetWorld();
   check(world);
   UGameInstance* gameInstance = world->GetGameInstance();
   check(gameInstance);
   return gameInstance->GetSubsystem<UTATQuestDataSubsystem>();
}

const FTATQuestInfo* UTATQuestDataSubsystem::FindQuestInfo(const FGameplayTag& questTag) const
{
   if (!questTag.IsValid())
   {
      return nullptr;
   }

   // NB: Not using _GetTableForMinimalTag, as redirects are N/A
   const UDataTable* questTable = questTag.MatchesTag(TAG_Mission) ? _GetMissionData() : _GetContractData();
   return QuestDataHelpers::LookupQuest<FTATQuestInfo>(questTable, questTag);
}

const FTATMinimalQuestInfo* UTATQuestDataSubsystem::FindMinimalQuestInfo(const FGameplayTag& questTag) const
{
   if (!questTag.IsValid())
   {
      return nullptr;
   }

   const UDataTable* questTable = _GetTableForMinimalTag(questTag);
   return QuestDataHelpers::LookupQuest<FTATMinimalQuestInfo>(questTable, questTag);
}

const FTATContractInfo* UTATQuestDataSubsystem::FindContractInfo(const FGameplayTag& questTag) const
{
   if (!questTag.IsValid())
   {
      return nullptr;
   }

   return QuestDataHelpers::LookupQuest<FTATContractInfo>(_GetContractData(), questTag);
}

const FTATMissionInfo* UTATQuestDataSubsystem::FindMissionInfo(const FGameplayTag& questTag) const
{
   if (!questTag.IsValid())
   {
      return nullptr;
   }

   return QuestDataHelpers::LookupQuest<FTATMissionInfo>(_GetMissionData(), questTag);
}

const FTATQuestObjectiveInfo* UTATQuestDataSubsystem::FindContractObjective(const FGameplayTag& questTag) const
{
   const FTATContractInfo* contract = FindContractInfo(questTag);
   return contract ? contract->GetObjective() : nullptr;
}

bool UTATQuestDataSubsystem::FindQuestHandle(const FGameplayTag& questTag, struct FTATQuestHandle& outHandle) const
{
   if (!questTag.IsValid())
   {
      return false;
   }

   const UDataTable* questTable = _GetTableForMinimalTag(questTag);
   if (questTable == nullptr)
   {
      UE_LOG(LogTATQuestDataSubsystem, Warning, TEXT("No quest data table"));
      return false;
   }

   if (!questTable->GetRowMap().Contains(questTag.GetTagName()))
   {
      return false;
   }

   FDataTableRowHandle rowHandle;
   rowHandle.DataTable = questTable;
   rowHandle.RowName = questTag.GetTagName();
   outHandle = FTATQuestHandle(rowHandle);
   return true;
}

const UDataTable* UTATQuestDataSubsystem::_GetTableForMinimalTag(const FGameplayTag& questTag) const
{
   if (questTag.MatchesTag(TAG_Mission))
   {
      return _GetMissionData();
   }
   else
   {
      return _GetContractData();
   }
}

const UDataTable* UTATQuestDataSubsystem::_GetContractData() const
{  
   // If we lose the race, just fall back to sync load for now
   if (_contractData == nullptr)
   {
      _contractData = UTATProjectSettings::Get().ContractDataTable.LoadSynchronous();
   }

   return _contractData;
}

const UDataTable* UTATQuestDataSubsystem::_GetMissionData() const
{
   // If we lose the race, just fall back to sync load for now
   if (_missionData == nullptr)
   {
      _missionData = UTATProjectSettings::Get().GetDataTableForMissions().LoadSynchronous();
   }

   return _missionData;
}
