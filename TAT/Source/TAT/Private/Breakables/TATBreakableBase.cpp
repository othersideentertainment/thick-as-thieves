// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Breakables/TATBreakableBase.h"

// tat
#include "Breakables/TATBreakableActorImpl.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATBreakableBase)

BREAKABLE_ACTOR_IMPLS(ATATBreakableBase, _abilitySystemComponent, _breakableComponent)

ATATBreakableBase::ATATBreakableBase()
{
   _abilitySystemComponent = CreateAbilitySystemForBreakables(this);
   _breakableComponent = CreateDefaultSubobject<UTATBreakableComponent>(TEXT("BreakableComponent"));
   bReplicateUsingRegisteredSubObjectList = true;

   // Fine to override this, but for actors that only break, this is a decent default
   NetDormancy = DORM_Initial;
}
