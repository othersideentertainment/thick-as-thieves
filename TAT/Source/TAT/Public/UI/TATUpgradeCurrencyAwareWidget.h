// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "UI/TATUserWidget.h"
#include "Upgrades/UpgradeCurrencies/TATUpgradeCurrencySource.h"

#include "TATUpgradeCurrencyAwareWidget.generated.h"

// A widget that can listen to changes to an upgrade currency source
// TODO [JC 6/27/2024]: Delete this whole class, it's deprecated and should not be used for new things
UCLASS(meta = (DisableNativeTick))
class TAT_API UTATUpgradeCurrencyAwareWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;

   UFUNCTION(BlueprintCallable, Meta = (BlueprintProtected))
   void SetUpgradeCurrencySource(TScriptInterface<ITATUpgradeCurrencySource> currencySource);

   UFUNCTION(BlueprintCallable)
   void ClearUpgradeCurrencySource();

   UFUNCTION(BlueprintPure, Category = "Upgrade Currency Aware Widget")
   FTATCharacterSaveId GetOwningCharacterSaveId() const;

protected:

   UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="TAT UserWidget|Upgrade", Meta = (BlueprintProtected, DisplayName = "OnUpgradeCurrencyChanged"))
   void BP_OnUpgradeCurrencyChanged(FGameplayTag currencyTag, int32 newValue);

   UFUNCTION(BlueprintPure, Meta = (BlueprintProtected))
   bool HasCurrrencySource() const { return _currencySource != nullptr; }

   UFUNCTION(BlueprintPure, Meta = (BlueprintProtected))
   int32 GetUpgradeCurrency(FGameplayTag currencyTag) const;

private:
   void _AddCurrencyListener();
   void _RemoveCurrencyListener();

   UPROPERTY(Transient)
   TScriptInterface<ITATUpgradeCurrencySource> _currencySource;
};
