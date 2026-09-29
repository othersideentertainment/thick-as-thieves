// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATThievesDenQuestSubsystem.h"

// tat
#include "GameFramework/TATWorldSettings.h"
#include "Loot/TATLootTypes.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestInfo.h"
#include "Quests/TATContractState.h"
#include "SaveGame/TATSaveGame.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThievesDenQuestSubsystem)

bool UTATThievesDenQuestSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   UWorld* world = CastChecked<UWorld>(outer);

   const ATATWorldSettings* worldSettings = CastChecked<ATATWorldSettings>(world->GetWorldSettings());
   return worldSettings->MapType == ETATMapType::ThievesDen;
}

void UTATThievesDenQuestSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);

   // Note: the save game should be available in the thieves den, but that may not be true
   //       if something goes directly into that level (outside of PIE, where that always works)
   //       Not handling that for now, but could bind to ose save game initialization flow if needed.

   _saveGame = UTATSaveGame::GetTATSaveGame(this);
   if (!ensure(_saveGame))
   {
      return;
   }

   _ReconcileTriggeredContracts();
   
   _saveGame->OnContractsChanged.AddUniqueDynamic(this, &UTATThievesDenQuestSubsystem::_RefreshContracts);
   _RefreshContracts();
}

void UTATThievesDenQuestSubsystem::Deinitialize()
{
   if (IsValid(_saveGame))
   {
      _saveGame->OnContractsChanged.RemoveAll(this);
   }
}

FGameplayTag UTATThievesDenQuestSubsystem::GetCompletableContractForFlow(ETATContractOutroFlow flow) const
{
   return _completableContracts.FindRef(flow);
}

bool UTATThievesDenQuestSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

void UTATThievesDenQuestSubsystem::_RefreshContracts()
{
   if (!ensure(_saveGame))
   {
      return;
   }

   const TArray<FTATContractWithStatus>& contracts = _saveGame->GetPlayerProgression().Contracts;

   const UTATQuestDataSubsystem& questDataSubsystem = UTATQuestDataSubsystem::Get(this);

   // May not usually bother checking a delta, but the state should be small enough to trivially compare
   FCompletableContractMap newCompleteableQuests;
   for (const FTATContractWithStatus& contract : contracts)
   {
      if (contract.State != ETATContractState::Outro)
      {
         continue;
      }

      const FTATContractInfo* info = questDataSubsystem.FindContractInfo(contract.ContractTag);
      if (info == nullptr)
      {
         continue;
      }

      ETATContractOutroFlow flow = info->OutroFlow;
      if (flow == ETATContractOutroFlow::Automatic)
      {
         // This maybe happened if the data changed out from under it,
         // allow some fallback (probably easier than completing it right now)
         flow = ETATContractOutroFlow::Interactable;
      }

      if (!newCompleteableQuests.Contains(flow))
      {
         newCompleteableQuests.Add(flow, contract.ContractTag);
      }
   }

   if (!_completableContracts.OrderIndependentCompareEqual(newCompleteableQuests))
   {
      _completableContracts = MoveTemp(newCompleteableQuests);
      OnCompletableContractsChanged.Broadcast();
   }

   const int32 startableContractIndex = contracts.FindLastByPredicate([](const FTATContractWithStatus& contract) { return contract.State == ETATContractState::Intro; });
   const FGameplayTag startableContractTag = startableContractIndex >= 0 ? contracts[startableContractIndex].ContractTag : FGameplayTag();
   if (startableContractTag != _startableContract)
   {
      _startableContract = startableContractTag;
      OnStartableContractChanged.Broadcast(_startableContract);
   }
}

void UTATThievesDenQuestSubsystem::_ReconcileTriggeredContracts()
{
   // Looks for contracts that should have been triggered but are not, and then trigger them

   if (!ensure(_saveGame))
   {
      return;
   }

   const TArray<FTATContractWithStatus>& contracts = _saveGame->GetPlayerProgression().Contracts;
   TMap<FGameplayTag, ETATContractState> contractToState;
   for (const FTATContractWithStatus& contract : contracts)
   {
      contractToState.Add(contract.ContractTag, contract.State);
   }

   const UTATQuestDataSubsystem& questDataSubsystem = UTATQuestDataSubsystem::Get(this);

   // realistically would only be one, but what the heck
   TArray<FGameplayTag, TInlineAllocator<4>> newContractsToTrigger;
   for (const FTATContractWithStatus& contract : contracts)
   {
      if (contract.State != ETATContractState::Complete)
      {
         continue;
      }

      const FTATContractInfo* contractInfo = questDataSubsystem.FindContractInfo(contract.ContractTag);
      if (contractInfo == nullptr)
      {
         continue;
      }

      const FGameplayTag nextContract = contractInfo->NextContractInChain;
      if (!nextContract.IsValid())
      {
         continue;
      }

      if (contractToState.FindRef(nextContract, ETATContractState::Unstarted) == ETATContractState::Unstarted)
      {
         newContractsToTrigger.Add(nextContract);
      }
   }

   for (FGameplayTag contractTag : newContractsToTrigger)
   {
      _saveGame->SetContractState(contractTag, ETATContractState::Intro);
   }
}
