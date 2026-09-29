// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ItemActorRuntimeStateOwnerInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemActorRuntimeStateOwnerInterface)

bool UItemActorRuntimeStateOwnerFunctionLibrary::AuthorityTransferItemActorRuntimeState(TSubclassOf<AActor> itemActorClass, TScriptInterface<IItemActorRuntimeStateOwnerInterface> from, TScriptInterface<IItemActorRuntimeStateOwnerInterface> to)
{
   if (from && to)
   {
      if (AItemActorRuntimeState* runtimeState = from->AuthorityGetItemActorRuntimeState(itemActorClass))
      {
         to->AuthorityAddItemActorRuntimeState(itemActorClass, runtimeState);
         from->AuthorityClearItemActorRuntimeState(itemActorClass);
         return true;
      }
   }
   return false;
}

