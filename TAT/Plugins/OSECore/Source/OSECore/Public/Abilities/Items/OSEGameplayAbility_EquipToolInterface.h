// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "UObject/Interface.h"

#include "OSEGameplayAbility_EquipToolInterface.generated.h"

// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(Blueprintable, MinimalAPI, Category = "Tools")
class UOSEGameplayAbility_EquipToolInterface : public UInterface
{
   GENERATED_BODY()
};

//---------------------------------------------------------------------------------------------------
/// IOSEGameplayAbility_EquipToolInterface
//---------------------------------------------------------------------------------------------------

class OSECORE_API IOSEGameplayAbility_EquipToolInterface
{
   GENERATED_BODY()

public:
   /// Returns the tool class equipped by this ability
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tools")
   TSubclassOf<UToolComponent> GetEquipToolClass() const;
   virtual TSubclassOf<UToolComponent> GetEquipToolClass_Implementation() const { return nullptr; }
};
