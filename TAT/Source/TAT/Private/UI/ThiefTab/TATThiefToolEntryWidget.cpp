// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/ThiefTab/TATThiefToolEntryWidget.h"

// tat
#include "Tools/TATToolComponent.h"

// ose
#include "Items/ToolComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefToolEntryWidget)

void UTATThiefToolEntryWidget::SetTool(const UToolComponent* toolComponent)
{
   check(IsValid(toolComponent));
   _toolComponent = toolComponent;
}

bool UTATThiefToolEntryWidget::GetAmmo(int& currentAmmo, int& maxAmmo) const
{
   ensure(IsValid(_toolComponent));
   if (const UTATToolComponent* tatToolComponent = Cast<UTATToolComponent>(_toolComponent))
   {
      if (tatToolComponent->AmmoType == ETATToolAmmoType::Finite)
      {
         currentAmmo = tatToolComponent->GetCurrentAmmo();
         maxAmmo = tatToolComponent->GetMaxAmmo();
         return true;
      }
   }
   return false;
}
