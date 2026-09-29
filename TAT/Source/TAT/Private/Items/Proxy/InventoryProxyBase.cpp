// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Proxy/InventoryProxyBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryProxyBase)

const TArray< UTATItemUIProxy*>& UInventoryProxyBase::GetBucket(EInventoryType bucket) const
{
   unimplemented();

   // Oh well
   static const TArray< UTATItemUIProxy*> kDummy;
   return kDummy;
}

