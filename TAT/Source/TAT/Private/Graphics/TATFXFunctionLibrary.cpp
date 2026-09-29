// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Graphics/TATFXFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATFXFunctionLibrary)


UNiagaraComponent* UTATFXFunctionLibrary::SpawnSystemAtLocationInRealm(const UObject* worldContextObject, class UNiagaraSystem* systemTemplate, FVector location, FRotator rotation /*= FRotator::ZeroRotator*/, FVector scale /*= FVector(1.f)*/, bool autoDestroy /*= true*/, bool autoActivate /*= true*/, ENCPoolMethod poolingMethod /*= ENCPoolMethod::None*/, bool preCullCheck /*= true*/)
{
   FFXSystemSpawnParameters spawnParams;
   spawnParams.WorldContextObject = worldContextObject;
   spawnParams.SystemTemplate = systemTemplate;
   spawnParams.Location = location;
   spawnParams.Rotation = rotation;
   spawnParams.Scale = scale;
   spawnParams.bAutoDestroy = autoDestroy;
   spawnParams.bAutoActivate = autoActivate;
   spawnParams.PoolingMethod = ToPSCPoolMethod(poolingMethod);
   spawnParams.bPreCullCheck = preCullCheck;
   return UNiagaraFunctionLibrary::SpawnSystemAtLocationWithParams(spawnParams);
}

