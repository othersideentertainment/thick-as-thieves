// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "CharacterCustomization/TATCharacterLoadout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterLoadout)

bool FTATCharacterLoadout::IsEmpty() const
{
   for (const FTATCharacterLoadoutEntry& entry : Entries)
   {
      if (entry.LoadoutTag.IsValid())
      {
         return false;
      }
   }
   return true;
}
