// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "UObject/Interface.h"

#include "ToolSetSystemInterface.generated.h"

class IToolSetInterface;

// Interface to provide access to the ToolSetInterface
UINTERFACE(BlueprintType, MinimalAPI, Category = "Tools", meta = (CannotImplementInterfaceInBlueprint))
class UToolSetSystemInterface : public UInterface
{
   GENERATED_BODY()
};

//---------------------------------------------------------------------------------------------------
/// ToolSet System Interface
/// This is modeled after the ability system interface. The sole purpose is to ensure that the
/// ToolSetInterface is accessible somehow. The actual ToolSet can live on a different object
/// than the one that implements this interface.
//---------------------------------------------------------------------------------------------------
class OSECORE_API IToolSetSystemInterface
{
   GENERATED_BODY()

public:

   /// Returns the ToolSet interface to use. It may live on another actor or component.
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual TScriptInterface<IToolSetInterface> GetToolSetInterface() const = 0;
};
