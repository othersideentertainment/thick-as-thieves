// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/TATContractPrioritizer.h"

// tat
#include "TATGameInstance.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestInfo.h"


FTATPartyContracts TATContractPrioritizer::BuildPartyContractsFromIndividuals(const UObject* worldContext, const TSoftObjectPtr<UWorld>& map,
   TConstArrayView<FTATContractWithNetId> selections)
{
   FTATContractPrioritizer prioritizer(worldContext, map);
   for (const FTATContractWithNetId& contractWithNetId : selections)
   {
      prioritizer.AddContract(contractWithNetId.ContractTag);
   }

   const FGameplayTag contractTag = prioritizer.GetChosenContract();

   TArray<FUniqueNetIdRepl> playersOnContract;
   for (const FTATContractWithNetId& contractWithNetId : selections)
   {
      if (contractWithNetId.ContractTag == contractTag)
      {
         playersOnContract.Add(contractWithNetId.PlayerUniqueId);
      }
   }

   return FTATPartyContracts {
      .ContractTag = contractTag,
      .PlayersOnContract = MoveTemp(playersOnContract),
   };
}

FTATContractPrioritizer::FTATContractPrioritizer(const UObject* worldContext, const TSoftObjectPtr<UWorld>& map)
   : _map(map)
   , _questData(UTATQuestDataSubsystem::TryGet(worldContext))
{
}

void FTATContractPrioritizer::AddContract(const FGameplayTag& contractTag)
{
   if (!contractTag.IsValid() || contractTag == _chosenContract)
   {
      return;
   }

   const UTATQuestDataSubsystem* questData = _questData.Get();
   if (questData == nullptr)
   {
      return;
   }

   const FTATContractInfo* contract = questData->FindContractInfo(contractTag);
   if (contract == nullptr)
   {
      return;
   }

   // skip if for another map
   if (!contract->Map.IsNull() && contract->Map != _map)
   {
      return;
   }

   // lower order contracts take precedence
   if (_chosenContract.IsValid() && _currentPriority <= contract->ContractOrder)
   {
      return;
   }

   _chosenContract = contractTag;
   _currentPriority = contract->ContractOrder;
}
