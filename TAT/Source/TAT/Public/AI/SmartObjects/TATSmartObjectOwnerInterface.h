// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATSmartObjectOwnerInterface.generated.h"

class UTATSmartObjectComponent;
// This class does not need to be modified.
UINTERFACE(NotBlueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UTATSmartObjectOwnerInterface : public UInterface
{
   GENERATED_BODY()
};

/**
 * 
 */
class TAT_API ITATSmartObjectOwnerInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable)
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const = 0;
};
