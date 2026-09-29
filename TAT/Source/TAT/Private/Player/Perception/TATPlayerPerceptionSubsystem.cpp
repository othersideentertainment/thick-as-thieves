// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/Perception/TATPlayerPerceptionSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerPerceptionSubsystem)


void UTATPlayerPerceptionSubsystem::RegisterPerceivable(UTATPlayerPerceivableComponent* perceivable)
{
   check(perceivable != nullptr);
   checkf(!_perceivables.Contains(perceivable), TEXT("Trying to double-register perceivable '%s' on actor '%s'"),
      *GetNameSafe(perceivable),
      *GetNameSafe(perceivable ? perceivable->GetOwner() : nullptr));
   _perceivables.Add(perceivable);

   OnPerceivableRegistered.Broadcast(perceivable);
}

void UTATPlayerPerceptionSubsystem::UnregisterPerceivable(UTATPlayerPerceivableComponent* perceivable)
{
   _perceivables.RemoveSingleSwap(perceivable);

   // Remove any weak pointers that have gone stale
   _perceivables.RemoveAll([](const TWeakObjectPtr<UTATPlayerPerceivableComponent>& perceivablePtr)
   {
      return !perceivablePtr.IsValid();
   });
}
