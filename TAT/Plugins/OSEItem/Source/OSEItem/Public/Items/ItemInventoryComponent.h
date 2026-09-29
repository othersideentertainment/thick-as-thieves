// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose

// ue4
#include "CoreMinimal.h"

#include "ItemInventoryComponent.generated.h"

UCLASS(BlueprintType, ClassGroup = (Items), meta = (BlueprintSpawnableComponent))
class OSEITEM_API UItemInventoryComponent : public UActorComponent
{
   GENERATED_BODY()

public:

   UItemInventoryComponent();

};

