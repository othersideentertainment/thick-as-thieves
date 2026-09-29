// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AttributeBaseSystemInterface.generated.h"

class IAttributeBaseInterface;


// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(MinimalAPI, Category = "Ability", meta = (CannotImplementInterfaceInBlueprint))
class UAttributeBaseSystemInterface : public UInterface
{
   GENERATED_BODY()
};


//---------------------------------------------------------------------------------------------------
/// Provides access to the base attribute system
//---------------------------------------------------------------------------------------------------

class OSECORE_API IAttributeBaseSystemInterface
{
   GENERATED_BODY()

public:

   /// Returns the attribute interface to use. It may live on another actor or component.
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual TScriptInterface<IAttributeBaseInterface> GetBaseAttributeInterface() const = 0;
};
