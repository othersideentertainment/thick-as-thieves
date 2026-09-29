// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "SaveGame/TATCharacterSaveId.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "TATUpgradeCurrencySource.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTATUpgradeCurrencyChanged, FGameplayTag, currency, int32, value);

// This class does not need to be modified.
UINTERFACE(BlueprintType, MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTATUpgradeCurrencySource : public UInterface
{
   GENERATED_BODY()
};

// An interface for getting upgrade currency balances
// TODO [JC 6/27/2024]: Delete this interface, it's deprecated and should not be used for new things (the TOW-era item inventory still uses it)
class TAT_API ITATUpgradeCurrencySource
{
   GENERATED_BODY()
public:

   UFUNCTION(BlueprintCallable, Category="Upgrade|Currency")
   virtual int32 GetUpgradeCurrency(FTATCharacterSaveId character, FGameplayTag currencyTag) const { return 0; }

   virtual FTATUpgradeCurrencyChanged& GetUpgradeCurrencyChanged(FTATCharacterSaveId character) = 0;
};
