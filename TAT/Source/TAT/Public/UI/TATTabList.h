// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <CommonTabListWidgetBase.h>

#include "TATTabList.generated.h"

UCLASS(MinimalAPI, Abstract, meta=(DisableNativeTick))
class UTATTabList : public UCommonTabListWidgetBase
{
   GENERATED_BODY()

public:
   virtual void NativeConstruct() override;

protected:
   virtual void HandlePreLinkedSwitcherChanged() override;
   virtual void HandlePostLinkedSwitcherChanged() override;
   virtual void HandleTabCreation_Implementation(FName tabId, UCommonButtonBase* tabButton) override;

   void RegisterTabSwitcher();
   void UnregisterTabSwitcher();

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = TabList, meta=(MustImplement = "/Script/TAT.TATTabButtonInterface"))
   TSubclassOf<UCommonButtonBase> ButtonWidgetType;
};
