// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// TAT
#include "UI/TATTabButtonInterface.h"

// UE
#include <CommonActivatableWidgetSwitcher.h>
#include <Components/WidgetSwitcherSlot.h>

#include "TATTabSwitcher.generated.h"

UCLASS(MinimalAPI)
class UTATTabSwitcherSlot : public UWidgetSwitcherSlot
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widget Switcher Slot", meta=(ShowOnlyInnerProperties))
   FTATTabDescriptor TabDescriptor;
};

UCLASS(MinimalAPI)
class UTATTabSwitcher : public UCommonActivatableWidgetSwitcher
{
   GENERATED_BODY()

public:
   virtual UClass* GetSlotClass() const override;
   virtual void OnSlotAdded(UPanelSlot* slot) override;
};
