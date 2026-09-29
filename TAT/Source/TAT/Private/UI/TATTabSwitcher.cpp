// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATTabSwitcher.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTabSwitcher)

UClass* UTATTabSwitcher::GetSlotClass() const
{
   return UTATTabSwitcherSlot::StaticClass();
}

void UTATTabSwitcher::OnSlotAdded(UPanelSlot* slot)
{
   Super::OnSlotAdded(slot);

   FTATTabDescriptor& tabDescriptor = CastChecked<UTATTabSwitcherSlot>(slot)->TabDescriptor;

   tabDescriptor.TabId = slot->Content.GetFName();
   tabDescriptor.DisplayName = FText::FromName(tabDescriptor.TabId);
}
