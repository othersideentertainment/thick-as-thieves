// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ItemInventoryComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemInventoryComponent)

// ose

// ue4

UItemInventoryComponent::UItemInventoryComponent()
   : Super()
{
   SetIsReplicatedByDefault(true);
   SetAutoActivate(true);
}


