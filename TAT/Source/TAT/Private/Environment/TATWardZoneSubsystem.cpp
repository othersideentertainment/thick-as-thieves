// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWardZoneSubsystem.h"

// tat
#include "Environment/TATWardZone.h"
#include "Player/TATPlayerController.h"

// ue
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWardZoneSubsystem)

UTATWardZoneSubsystem::UTATWardZoneSubsystem()
{
}

void UTATWardZoneSubsystem::RegisterWardZone(ATATWardZone* wardZone)
{
   if (wardZone != nullptr)
   {
      _wardZones.Add(wardZone);
   }
}

void UTATWardZoneSubsystem::UnregisterWardZone(ATATWardZone* wardZone)
{
   if (wardZone != nullptr)
   {
      _wardZones.Remove(wardZone);
   }
}

void UTATWardZoneSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);

   // Only tick on non-dedicated
   if (inWorld.GetNetMode() != NM_DedicatedServer)
   {
      static constexpr float tickInterval = 1.0 / 30.f;
      static constexpr bool looping = true;
      GetWorld()->GetTimerManager().SetTimer(_wardVisualStateTickHandle, FTimerDelegate::CreateUObject(this, &UTATWardZoneSubsystem::_TickWardVisualState), tickInterval, looping);
   }
}

void UTATWardZoneSubsystem::_TickWardVisualState()
{
   // Nothing to do if we have no ward zones
   if (_wardZones.Num() == 0)
   {
      return;
   }

   // Make sure we have a valid local player controller
   if (_localPlayerController == nullptr)
   {
      _localPlayerController = ATATPlayerController::GetLocalTATPlayerController(this);
      if (_localPlayerController == nullptr)
      {
         return;
      }
   }

   // Get the local player character
   ACharacter* character = _localPlayerController->GetCharacter();
   if (character == nullptr)
   {
      return;
   }

   const FVector playerLoc = character->GetActorLocation();

   for (ATATWardZone* wardZone : _wardZones)
   {
      if (ensure(wardZone != nullptr))
      {
         const bool isFirstTick = !_localPlayerWardStates.Contains(wardZone);
         FTATWardZonePlayerVisibilityState& state = _localPlayerWardStates.FindOrAdd(wardZone);
         wardZone->TickLocalPlayerVisualState(character, isFirstTick, state);
      }
   }
}
