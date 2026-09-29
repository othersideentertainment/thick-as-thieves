// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATTabList.h"

// TAT
#include "UI/TATTabButtonInterface.h"
#include "UI/TATTabSwitcher.h"

// UE
#include <CommonButtonBase.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTabList)

void UTATTabList::NativeConstruct()
{
   Super::NativeConstruct();

   RegisterTabSwitcher();
}

void UTATTabList::HandlePreLinkedSwitcherChanged()
{
   Super::HandlePreLinkedSwitcherChanged();

   if (!IsDesignTime())
   {
      UnregisterTabSwitcher();
   }
}

void UTATTabList::HandlePostLinkedSwitcherChanged()
{
   Super::HandlePostLinkedSwitcherChanged();

   if (!IsDesignTime())
   {
      RegisterTabSwitcher();
   }
}

void UTATTabList::HandleTabCreation_Implementation(FName tabId, UCommonButtonBase* tabButton)
{
   Super::HandleTabCreation_Implementation(tabId, tabButton);

   if (tabButton->GetClass()->ImplementsInterface(UTATTabButtonInterface::StaticClass()))
   {
      const FCommonRegisteredTabInfo& tab = GetRegisteredTabsByID().FindChecked(tabId);

      const UTATTabSwitcherSlot* tabSwitcherSlot = Cast<UTATTabSwitcherSlot>(tab.ContentInstance->Slot);
      if (tabSwitcherSlot)
      {
         ITATTabButtonInterface::Execute_SetRepresentedTab(tabButton, this, tabSwitcherSlot->TabDescriptor);
      }
   }
}

void UTATTabList::RegisterTabSwitcher()
{
   const UTATTabSwitcher* tabSwitcher = Cast<UTATTabSwitcher>(LinkedSwitcher.Get());
   if (tabSwitcher)
   {
      for (const UPanelSlot* slot : tabSwitcher->GetSlots())
      {
         const UTATTabSwitcherSlot* tabSwitcherSlot = CastChecked<UTATTabSwitcherSlot>(slot);
         RegisterTab(tabSwitcherSlot->TabDescriptor.TabId, ButtonWidgetType, tabSwitcherSlot->Content);
      }
   }
}

void UTATTabList::UnregisterTabSwitcher()
{
   const UTATTabSwitcher* tabSwitcher = Cast<UTATTabSwitcher>(LinkedSwitcher.Get());
   if (tabSwitcher)
   {
      for (const UPanelSlot* slot : tabSwitcher->GetSlots())
      {
         const UTATTabSwitcherSlot* tabSwitcherSlot = CastChecked<UTATTabSwitcherSlot>(slot);
         RemoveTab(tabSwitcherSlot->TabDescriptor.TabId);
      }
   }
}
