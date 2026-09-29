// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSESerializedTagMap.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESerializedTagMap)

void FOSESerializedTagMap::MergeNumerically(const FOSESerializedTagMap& other)
{
   for (const TPair<FGameplayTag, int32>& pair : other.Values)
   {
      Values.FindOrAdd(pair.Key) += pair.Value;
   }
}

bool FOSESerializedTagMap::NetSerialize(FArchive& ar, UPackageMap* map, bool& bOutSuccess)
{
   int32 size;
   if (ar.IsSaving())
   {
      size = Values.Num();
   }
   ar << size;

   if (ar.IsLoading())
   {
      Values.Empty(size);
      for (int i = 0; i < size; ++i)
      {
         FGameplayTag tag;
         int32 value;
         ar << tag << value;
         Values.Add(tag, value);
      }
   }
   else
   {
      for (auto& pair : Values)
      {
         ar << pair.Key << pair.Value;
      }
   }

   bOutSuccess = true;
   return true;
}

