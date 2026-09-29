// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//ue
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "TATPhysicalMaterial.generated.h"

class UAkSwitchValue;
// A TAT-level physical material subclass that we can add properties to
UCLASS()
class TAT_API UTATPhysicalMaterial : public UPhysicalMaterial
{
   GENERATED_BODY()

public:
   // The wwise switch to set when playing sounds for impacts on this physical material
   UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = Audio)
   TObjectPtr<UAkSwitchValue> ImpactMaterialSwitch;


   UFUNCTION(BlueprintPure, Category = "Audio|TAT")
   static UAkSwitchValue* GetImpactSwitchFromPhysicalMaterial(const UPhysicalMaterial* physMat);
   
};
