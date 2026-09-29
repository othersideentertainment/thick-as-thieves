// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "UpgradeQueryInterface.generated.h"


// An interface with convenience accessors for upgrade values
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UUpgradeQueryInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IUpgradeQueryInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE|Upgrade", meta = (Categories = "Upgrade"))
   virtual int32 GetUpgradeValue(FGameplayTag tag, int32 fallback = 0) const = 0;

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE|Upgrade", meta = (Categories = "Upgrade"))
   virtual bool HasUpgrade(FGameplayTag tag, int32 level = 1) const
   {
      return GetUpgradeValue(tag, 0) >= level;
   }
};
