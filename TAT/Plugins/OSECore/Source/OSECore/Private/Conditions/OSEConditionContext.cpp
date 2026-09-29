// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


//ose
#include "Conditions/OSEConditionContext.h"
#include "Character/OSECharacterBase.h"

FOSEConditionContext::FOSEConditionContext(const AActor* actor)
   : _actor(actor)
   , _character(Cast<AOSECharacterBase>(actor))
{

}
