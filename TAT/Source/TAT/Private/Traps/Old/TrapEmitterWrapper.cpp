// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/Old/TrapEmitterWrapper.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TrapEmitterWrapper)

// Sets default values
ATrapEmitterWrapper_Old::ATrapEmitterWrapper_Old()
{
   bReplicates = true;
   NetDormancy = DORM_DormantAll;
}

void ATrapEmitterWrapper_Old::OnTriggered_Implementation()
{
   TInlineComponentArray<UActorComponent*> emitterComponents;
   for (UActorComponent* component : GetComponents())
   {
      if (component && component->Implements<UTrapEmitterInterface_Old>())
      {
         emitterComponents.Add(component);
      }
   }
   for (UActorComponent* component : emitterComponents)
   {
      ITrapEmitterInterface_Old::Execute_OnTriggered(component);
   }
}

