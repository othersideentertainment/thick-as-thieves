// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/TATUpgradeCurrencyAwareWidget.h"

#include "Player/TATPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUpgradeCurrencyAwareWidget)

void UTATUpgradeCurrencyAwareWidget::NativeConstruct()
{
   Super::NativeConstruct();

   _AddCurrencyListener();
}

void UTATUpgradeCurrencyAwareWidget::NativeDestruct()
{
   Super::NativeDestruct();

   _RemoveCurrencyListener();
}

void UTATUpgradeCurrencyAwareWidget::SetUpgradeCurrencySource(TScriptInterface<ITATUpgradeCurrencySource> currencySource)
{
   if (currencySource == _currencySource) return;

   ClearUpgradeCurrencySource();

   _currencySource = currencySource;
   _AddCurrencyListener();
}

void UTATUpgradeCurrencyAwareWidget::ClearUpgradeCurrencySource()
{
   _RemoveCurrencyListener();
   _currencySource = nullptr;
}

FTATCharacterSaveId UTATUpgradeCurrencyAwareWidget::GetOwningCharacterSaveId() const
{
   if (ATATPlayerState* playerState = GetOwningPlayerState<ATATPlayerState>())
   {
      return playerState->GetCharacterSaveId();
   }
   return FTATCharacterSaveId{};
}

int32 UTATUpgradeCurrencyAwareWidget::GetUpgradeCurrency(FGameplayTag currencyTag) const
{
   if (_currencySource)
   {
      return _currencySource->GetUpgradeCurrency(GetOwningCharacterSaveId(), currencyTag);
   }

   return 0;
}

void UTATUpgradeCurrencyAwareWidget::_AddCurrencyListener()
{
   if (_currencySource)
   {
      _currencySource->GetUpgradeCurrencyChanged(GetOwningCharacterSaveId()).AddUniqueDynamic(this, &UTATUpgradeCurrencyAwareWidget::BP_OnUpgradeCurrencyChanged);
   }
}

void UTATUpgradeCurrencyAwareWidget::_RemoveCurrencyListener()
{
   if (_currencySource)
   {
      _currencySource->GetUpgradeCurrencyChanged(GetOwningCharacterSaveId()).RemoveAll(this);
   }
}
