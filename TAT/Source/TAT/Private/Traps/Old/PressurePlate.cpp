// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/Old/PressurePlate.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PressurePlate)

// Sets default values
APressurePlate_Old::APressurePlate_Old()
{

}

void APressurePlate_Old::NotifyActorBeginOverlap(AActor* otherActor)
{
   if (!HasAuthority() || !CanBeTriggeredByActor(otherActor)) return;

   if (_triggerOnEnter)
   {
      if (IsInState(ETrapTriggerState::Ready))
      {
         AuthorityTrigger();
      }
      return;
   }

   _actorsOnPlate.AddUnique(otherActor);
   if (IsInState(ETrapTriggerState::Ready))
   {
      _SetState(ETrapTriggerState::Depressed);
   }
}

void APressurePlate_Old::NotifyActorEndOverlap(AActor* otherActor)
{
   if (!HasAuthority()) return;

   _actorsOnPlate.RemoveSwap(otherActor);
   _actorsOnPlate.RemoveAllSwap([](AActor* a) {return a == nullptr;});

   if (_actorsOnPlate.Num() == 0 && IsInState(ETrapTriggerState::Depressed))
   {
      AuthorityTrigger();
   }
}

ETrapTriggerState APressurePlate_Old::GetStateAfterReset() const
{
   bool isDown = _actorsOnPlate.Num() > 0;
   return isDown ? ETrapTriggerState::Depressed : ETrapTriggerState::Ready;
}

