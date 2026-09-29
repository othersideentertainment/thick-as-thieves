// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/TATInventoryTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInventoryTypes)

const FInventoryStackId FInventoryStackId::Invalid;

int32 FTATInventorySize::GetSizeForBucket(EInventoryType bucket) const
{
   // Consider an explicit default value if different users want to treat unlimited buckets differently
   switch (bucket)
   {
   case EInventoryType::Backpack:
      return BackpackSize;
   case EInventoryType::Toolbelt:
      return ToolbeltSize;
   case EInventoryType::QuestItems:
   default:
      return MAX_int32;
   }
}

