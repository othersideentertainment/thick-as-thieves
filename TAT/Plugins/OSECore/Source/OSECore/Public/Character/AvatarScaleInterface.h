// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "AvatarScaleInterface.generated.h"

UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UAvatarScaleInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IAvatarScaleInterface
{
   GENERATED_BODY()

public:

   // Avatar scale is a normalized value representing the size of the avatar relative to a "normal" six foot human
   // This value is used in adjusting animation selection
   UFUNCTION(BlueprintCallable, Category = "Character|OSE")
   virtual float GetAvatarScale() const = 0;
};

