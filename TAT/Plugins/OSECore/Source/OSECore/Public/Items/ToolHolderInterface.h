// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "ToolVisuals.h"

#include "ToolHolderInterface.generated.h"

// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(BlueprintType, MinimalAPI, Category = "Tools", meta = (CannotImplementInterfaceInBlueprint))
class UToolHolderInterface : public UInterface
{
   GENERATED_BODY()
};

//---------------------------------------------------------------------------------------------------
/// Tool holder interface
//---------------------------------------------------------------------------------------------------

class OSECORE_API IToolHolderInterface
{
   GENERATED_BODY()

public:

   /// Returns the scene component to attach to based on perspective
   UFUNCTION(BlueprintCallable, Category = "Tools")
   virtual USceneComponent* GetToolRoot(EMeshPerspective MeshPerspective) const = 0;
};
