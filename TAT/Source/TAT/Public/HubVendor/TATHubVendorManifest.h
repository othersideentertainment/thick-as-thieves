// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Engine/DataTable.h"

#include "TATHubVendorManifest.generated.h"


class UTATItemInfo;

USTRUCT(BlueprintType)
struct FTATItemVendorListing : public FTableRowBase
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TSubclassOf<UTATItemInfo> Item;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   int32 ItemCount = 1;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   int32 GoldCost = 0;
};
