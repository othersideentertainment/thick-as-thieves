// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATUtilityAITargetingGroupInterface.generated.h"

UINTERFACE(BlueprintType, MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTATUtilityAITargetingGroupInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATUtilityAITargetingGroupInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable, Category = "AI|Utility|Targeting Group")
   virtual FGameplayTag GetUtilityAITargetingGroup() const = 0;
};
