// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "SaveGame/TATCharacterProgressionViewModel.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Quests/TATContractState.h"
#include "SaveGame/TATSaveGame.h"
#include "SaveGame/TATSavedLoot.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterProgressionViewModel)

void UTATCharacterProgressionViewModel::Initialize(UTATSaveGame* saveGame, FTATCharacterSaveId character)
{
   check(saveGame);
   _saveGame = saveGame;
   _saveId = character;

   FTATCharacterProgression::FOnDataChangedEvent& lootEvent = _saveGame->GetCharacterProgressionLootChangedEvent(_saveId);
   lootEvent.RemoveAll(this);
   lootEvent.AddUObject(this, &UTATCharacterProgressionViewModel::_OnCharacterProgressionLootChanged);

   FTATCharacterProgression::FOnDataChangedEvent& moneyEvent = _saveGame->GetPlayerProgressionMoneyChangedEvent();
   moneyEvent.RemoveAll(this);
   moneyEvent.AddUObject(this, &UTATCharacterProgressionViewModel::_OnCharacterProgressionMoneyChanged);

   FTATUpgradeCurrencyChanged& currencyEvent = _saveGame->GetUpgradeCurrencyChanged(_saveId);
   currencyEvent.RemoveAll(this);
   currencyEvent.AddUniqueDynamic(this, &UTATCharacterProgressionViewModel::_OnCharacterProgressionUpgradeCurrencyChanged);
}

void UTATCharacterProgressionViewModel::Deinitialize()
{
   if (_saveGame != nullptr && _saveId.IsValid())
   {
      FTATCharacterProgression::FOnDataChangedEvent& lootEvent = _saveGame->GetCharacterProgressionLootChangedEvent(_saveId);
      lootEvent.RemoveAll(this);

      FTATCharacterProgression::FOnDataChangedEvent& moneyEvent = _saveGame->GetPlayerProgressionMoneyChangedEvent();
      moneyEvent.RemoveAll(this);

      FTATUpgradeCurrencyChanged& currencyEvent = _saveGame->GetUpgradeCurrencyChanged(_saveId);
      currencyEvent.RemoveAll(this);
   }
}

ETATCharacter UTATCharacterProgressionViewModel::GetCharacterType() const
{
   return _GetCharacterProgression().Character;
}

FTATCharacterSaveId UTATCharacterProgressionViewModel::GetSaveId() const
{
   return _saveId;
}

int32 UTATCharacterProgressionViewModel::GetMoney() const
{
   return _GetPlayerProgression().Money;
}

int32 UTATCharacterProgressionViewModel::GetUpgradeCurrency(FGameplayTag currencyTag) const
{
   return _GetCharacterProgression().UpgradeCurrencies.GetValue(currencyTag);
}

int32 UTATCharacterProgressionViewModel::GetMatchCount() const
{
   return _GetCharacterProgression().MatchCount;
}

FTATPlayerExperience UTATCharacterProgressionViewModel::GetXP() const
{
   return _GetPlayerProgression().XP;
}

int32 UTATCharacterProgressionViewModel::GetTotalLootValue(const UObject* contextObject) const
{
   const UTATLootSettings& settings = UTATLootSettings::Get();
   const FTATSavedLootInventory& loot = GetLoot();
   int32 total = 0;
   for (const FTATLootCountPair& stack : loot.GetStacks())
   {
      total += settings.GetLootValue(contextObject, stack.Key) * stack.Value;
   }

   return total;
}

FGameplayTag UTATCharacterProgressionViewModel::GetLastActiveContract() const
{
   return _saveGame->GetLastActiveContract();
}

void UTATCharacterProgressionViewModel::GetPossibleContracts(TArray<FGameplayTag>& outQuests) const
{
   _saveGame->FindContractsInState(ETATContractState::Objective, outQuests);
}

const FTATSavedLootInventory& UTATCharacterProgressionViewModel::GetLoot() const
{
   return _GetCharacterProgression().Loot;
}

const FTATCharacterProgression& UTATCharacterProgressionViewModel::_GetCharacterProgression() const
{
   check(_saveGame);
   return _saveGame->GetCharacterProgression(_saveId);
}

const FTATPlayerProgression& UTATCharacterProgressionViewModel::_GetPlayerProgression() const
{
   check(_saveGame);
   return _saveGame->GetPlayerProgression();
}

void UTATCharacterProgressionViewModel::_OnCharacterProgressionLootChanged()
{
   OnLootChanged.Broadcast();
}

void UTATCharacterProgressionViewModel::_OnCharacterProgressionMoneyChanged()
{
   OnMoneyChanged.Broadcast(GetMoney());
}

void UTATCharacterProgressionViewModel::_OnCharacterProgressionUpgradeCurrencyChanged(FGameplayTag currencyTag, int32 newValue)
{
   OnUpgradeCurrencyChanged.Broadcast(currencyTag, newValue);
}
