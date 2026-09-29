// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Developer/TATPlayerInventorySettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerInventorySettings)

FTATInventorySize UTATPlayerInventorySettings::GetSizesForCharacter(ETATCharacter character, const FUpgradeState& upgradeState) const
{
   // have upgrade state as future room for computing based on upgrades

   const FTATInventorySize* found = SizeByCharacter.Find(character);
   if (found)
   {
      return *found;
   }
   else
   {
      return FTATInventorySize();
   }
}

