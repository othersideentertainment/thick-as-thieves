// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <UObject/Interface.h>

#include "TATTabButtonInterface.generated.h"

class UTATTabList;

USTRUCT(BlueprintType)
struct FTATTabDescriptor
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FName TabId;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText DisplayName;
};

UINTERFACE(BlueprintType)
class UTATTabButtonInterface : public UInterface
{
   GENERATED_BODY()
};

class ITATTabButtonInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintNativeEvent, Category = TabButton)
   void SetRepresentedTab(UTATTabList* tabList, const FTATTabDescriptor& tabDescriptor);
};
