// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Progression/TATPlayerExperience.h"
#include "SaveGame/TATCharacterSaveId.h"
#include "Upgrades/UpgradeCurrencies/TATUpgradeCurrencySource.h"

// ue
#include "CoreMinimal.h"

#include "TATCharacterProgressionViewModel.generated.h"

class UTATSaveGame;
struct FTATCharacterProgression;
struct FTATPlayerProgression;
struct FTATSavedLootInventory;
enum class ETATCharacter : uint8;

// A proxy class for the persisted character progression,
// since it is too easy to copy the struct
UCLASS(BlueprintType)
class TAT_API UTATCharacterProgressionViewModel : public UObject
{
   GENERATED_BODY()

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDataChangedProxyEvent);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntDataChangedProxyEvent, int32, newValue);

public:
   void Initialize(UTATSaveGame* saveGame, FTATCharacterSaveId character);
   void Deinitialize();

   UFUNCTION(BlueprintPure)
   ETATCharacter GetCharacterType() const;

   UFUNCTION(BlueprintPure)
   FTATCharacterSaveId GetSaveId() const;

   UFUNCTION(BlueprintPure, meta = (CompactNodeTitle = "Money"))
   int32 GetMoney() const;

   UFUNCTION(BlueprintPure, meta = (CompactNodeTitle = "UpgradeCurrency"))
   int32 GetUpgradeCurrency(UPARAM(Meta = (Categories = "UpgradeCurrency")) FGameplayTag currencyTag) const;

   UFUNCTION(BlueprintPure, meta = (CompactNodeTitle = "MatchCount"))
   int32 GetMatchCount() const;

   UFUNCTION(BlueprintPure, meta = (CompactNodeTitle = "XP"))
   FTATPlayerExperience GetXP() const;

   UFUNCTION(BlueprintCallable, BlueprintPure=False)
   const FTATSavedLootInventory& GetLoot() const;

   UFUNCTION(BlueprintCallable, meta = (WorldContext="contextObject"))
   int32 GetTotalLootValue(const UObject* contextObject) const;

   UFUNCTION(BlueprintPure, meta = (CompactNodeTitle = "LastActiveContract"))
   FGameplayTag GetLastActiveContract() const;

   UFUNCTION(BlueprintCallable)
   void GetPossibleContracts(TArray<FGameplayTag>& outQuests) const;

   UPROPERTY(BlueprintAssignable)
   FOnDataChangedProxyEvent OnLootChanged;

   UPROPERTY(BlueprintAssignable)
   FOnIntDataChangedProxyEvent OnMoneyChanged;

   UPROPERTY(BlueprintAssignable)
   FTATUpgradeCurrencyChanged OnUpgradeCurrencyChanged;

   // TODO: expose some events here?

private:
   const FTATCharacterProgression& _GetCharacterProgression() const;
   const FTATPlayerProgression& _GetPlayerProgression() const;

   void _OnCharacterProgressionLootChanged();
   void _OnCharacterProgressionMoneyChanged();
   UFUNCTION()
   void _OnCharacterProgressionUpgradeCurrencyChanged(FGameplayTag currencyTag, int32 newValue);

private:
   UPROPERTY(Transient)
   TObjectPtr<UTATSaveGame> _saveGame = nullptr;

   FTATCharacterSaveId _saveId;
};
