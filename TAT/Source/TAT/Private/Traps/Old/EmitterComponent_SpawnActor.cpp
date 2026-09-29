// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/Old/EmitterComponent_SpawnActor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EmitterComponent_SpawnActor)

// Sets default values for this component's properties
UEmitterComponent_SpawnActor_Old::UEmitterComponent_SpawnActor_Old()
{
}

FTransform UEmitterComponent_SpawnActor_Old::GetSpawnTransform() const
{
   FTransform transform = GetComponentTransform();
   FTransform localTransform(SpawnRotation.Quaternion(), SpawnLocation);
   FTransform::Multiply(&transform, &localTransform, &transform);
   transform.SetScale3D(FVector::OneVector);
   return transform;
}


