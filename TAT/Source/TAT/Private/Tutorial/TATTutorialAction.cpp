// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tutorial/TATTutorialAction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTutorialAction)

FString UTATTutorialAction::GetDebugName() const
{
   return TEXT("UNKNOWN ACTION");
}

#if WITH_EDITOR
void UTATTutorialAction::PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent)
{
   UObject::PostEditChangeProperty(propertyChangedEvent);
   _debugNameForEditor = GetDebugName();
}

void UTATTutorialAction::PostLoad()
{
   UObject::PostLoad();
   
   // DANGER ZONE: We're allowing VM code to potentially run during post load so fingers crossed it has no side effects
   TGuardValue<bool> guardIsRoutingPostLoad(FUObjectThreadContext::Get().IsRoutingPostLoad, false);
   _debugNameForEditor = GetDebugName();
}
#endif
