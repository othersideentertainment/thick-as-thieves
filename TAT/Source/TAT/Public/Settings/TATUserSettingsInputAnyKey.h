// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "TATUserSettingsInputAnyKey.generated.h"

/**
 * 
 */
UCLASS()
class TAT_API UTATUserSettingsInputAnyKey : public UCommonActivatableWidget
{
   GENERATED_BODY()
public:
   UTATUserSettingsInputAnyKey(const FObjectInitializer& initializer);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKeySelected, FKey, key);
   UPROPERTY(BlueprintAssignable)
   FOnKeySelected OnKeySelected;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKeySelectionCanceled);
   UPROPERTY(BlueprintAssignable)
   FOnKeySelectionCanceled OnKeySelectionCanceled;

protected:
   virtual void NativeOnActivated() override;
   virtual void NativeOnDeactivated() override;

   void HandleKeySelected(FKey InKey);
   void HandleKeySelectionCanceled();

   void Dismiss(TFunction<void()> PostDismissCallback);

private:
   bool bKeySelected = false;
   TSharedPtr<class FSettingsPressAnyKeyInputPreProcessor> InputProcessor;
};
