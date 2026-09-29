// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Lockpicking/TATLockCombinationName.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLockCombinationName)

void FTATLockCombinationName::PostSerialize(const FArchive& ar)
{
   if (ar.IsSaving() && Name.IsValid())
   {
      // This marks the saved name for later searching
      ar.MarkSearchableName(FTATLockCombinationName::StaticStruct(), Name);
   }
}

bool FTATLockCombinationName::SerializeFromMismatchedTag(const FPropertyTag& tag, FStructuredArchive::FSlot slot)
{
   if (tag.Type == NAME_NameProperty)
   {
      slot << Name;
      return true;
   }
   return false;
}
