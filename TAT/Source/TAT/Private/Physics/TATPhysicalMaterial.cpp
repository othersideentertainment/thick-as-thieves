// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Physics/TATPhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPhysicalMaterial)

UAkSwitchValue* UTATPhysicalMaterial::GetImpactSwitchFromPhysicalMaterial(const UPhysicalMaterial* physMat)
{
   if(const UTATPhysicalMaterial* tatPhysMat = Cast<UTATPhysicalMaterial>(physMat))
   {
      return tatPhysMat->ImpactMaterialSwitch;
   }

   return nullptr;
}
