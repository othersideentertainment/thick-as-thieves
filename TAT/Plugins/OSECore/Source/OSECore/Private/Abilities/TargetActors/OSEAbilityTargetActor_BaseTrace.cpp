// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TargetActors/OSEAbilityTargetActor_BaseTrace.h"

// ue4
#include "Abilities/GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityTargetActor_BaseTrace)

AOSEAbilityTargetActor_BaseTrace::AOSEAbilityTargetActor_BaseTrace()
   : Super()
{
}

bool AOSEAbilityTargetActor_BaseTrace::ShouldProduceTargetData() const
{
   // Workaround for ShouldProduceTargetData being false on the server for AI, because it is erroneously only checking
   // MasterPC, which is explicitly a PlayerController. This is preferable to setting ShouldProduceTargetDataOnServer,
   // as that will have other, potentially undesirable side-effects on the server.
   // TODO: Consider engine mod?
   // NOTE: If you change this, also update the other ShouldProduceTargetData methods with the same fix.
   return Super::ShouldProduceTargetData() || (OwningAbility && OwningAbility->GetActorInfo().IsLocallyControlled());
}

