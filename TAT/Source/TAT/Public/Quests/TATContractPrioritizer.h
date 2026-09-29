// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UTATQuestDataSubsystem;
struct FTATPartyContracts;

struct FTATContractWithNetId
{
   FGameplayTag ContractTag;
   FUniqueNetIdRepl PlayerUniqueId;
};

namespace TATContractPrioritizer
{
   FTATPartyContracts BuildPartyContractsFromIndividuals(const UObject* worldContext, const TSoftObjectPtr<UWorld>& map, TConstArrayView<FTATContractWithNetId> selections);
}

struct TAT_API FTATContractPrioritizer
{
public:
   FTATContractPrioritizer(const UObject* worldContext, const TSoftObjectPtr<UWorld>& map);

   void AddContract(const FGameplayTag& contract);

   const FGameplayTag& GetChosenContract() const { return _chosenContract; }

   
private:
   FGameplayTag _chosenContract;
   int _currentPriority = TNumericLimits<int>::Max();
   TSoftObjectPtr<UWorld> _map;
   TWeakObjectPtr<const UTATQuestDataSubsystem> _questData;
};
