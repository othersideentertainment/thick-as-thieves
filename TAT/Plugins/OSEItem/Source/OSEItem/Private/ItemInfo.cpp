// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ItemInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemInfo)

DEFINE_LOG_CATEGORY(LogOSEItems);

const FPrimaryAssetType UItemInfo::PrimaryAssetType("Item");

FPrimaryAssetId UItemInfo::GetPrimaryAssetId() const
{
   if (HasAnyFlags(RF_ClassDefaultObject) && !GetClass()->HasAnyClassFlags(CLASS_Native | CLASS_Intrinsic))
   {
      // TODO: should we strip the `BP_` prefix and `_ItemInfo` suffix?
      return FPrimaryAssetId(PrimaryAssetType, FPackageName::GetShortFName(GetPackage()->GetName()));
   }

   return FPrimaryAssetId();
}

/* static */
TSoftClassPtr<AItemActor> UItemInfo::GetItemActorFromItemInfoClass(TSubclassOf<UItemInfo> itemInfoClass)
{
   if (itemInfoClass)
   {
      if (const UItemInfo* itemInfoCDO = itemInfoClass.GetDefaultObject())
      {
         return itemInfoCDO->ItemActor;
      }
   }
   return nullptr;
}

