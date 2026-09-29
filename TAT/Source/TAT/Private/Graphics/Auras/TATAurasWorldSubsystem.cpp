// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/Auras/TATAurasWorldSubsystem.h"

// tat
#include "Graphics/Auras/TATAuraVisibilityTargetComponent.h"
#include "Graphics/Auras/TATAuraVisibilityTypes.h"
#include "Graphics/Auras/TATAuraVisibilityPerceiverComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAurasWorldSubsystem)

DEFINE_LOG_CATEGORY(LogTATAuraWorldSubsystem);

UTATAurasWorldSubsystem::UTATAurasWorldSubsystem() : Super()
{
}

UTATAurasWorldSubsystem* UTATAurasWorldSubsystem::Get(const UWorld* world)
{
   if (world)
   {
      return world->GetSubsystem<UTATAurasWorldSubsystem>();
   }

   return nullptr;
}

void UTATAurasWorldSubsystem::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   _cleanUpAuraComponents();
   _populateActiveAuraActors();
}

TStatId UTATAurasWorldSubsystem::GetStatId() const
{
   RETURN_QUICK_DECLARE_CYCLE_STAT(UTATAurasWorldSubsystem, STATGROUP_Tickables);
}

void UTATAurasWorldSubsystem::RegisterAuraVisibilityTarget(TWeakObjectPtr<UTATAuraVisibilityTargetComponent> auraComponent)
{
   check(auraComponent.IsValid())
   _auraVisibilityTargets.AddUnique(auraComponent);
}

void UTATAurasWorldSubsystem::UnregisterAuraVisibilityTarget(TWeakObjectPtr<UTATAuraVisibilityTargetComponent> auraComponent)
{
   _auraVisibilityTargets.Remove(auraComponent);
}

void UTATAurasWorldSubsystem::RegisterAuraVisibilityPerceiver(TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver)
{
   check(auraVisibilityPerceiver.IsValid());
   check(auraVisibilityPerceiver->ShouldEvaluateAnyAuraSenses());

   _auraVisibilityPerceivers.AddUnique(auraVisibilityPerceiver);
}

void UTATAurasWorldSubsystem::UnregisterAuraVisibilityPerceiver(TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver)
{
   check(auraVisibilityPerceiver.IsValid());
   check(auraVisibilityPerceiver->ShouldEvaluateAnyAuraSenses());

   if (_auraVisibilityPerceivers.Remove(auraVisibilityPerceiver) > 0)
   {
      OnAuraVisibilityPerceiverUnregistered.Broadcast(auraVisibilityPerceiver);
   }
}

void UTATAurasWorldSubsystem::_cleanUpAuraComponents()
{
   AURA_STAT_SCOPE(_cleanUpAuraComponents);
   int32 invalidActorCount = _auraVisibilityTargets.RemoveAllSwap([](const TWeakObjectPtr<UTATAuraVisibilityTargetComponent>& entry)
      {
         return !entry.IsValid();
      });

   if (invalidActorCount > 0)
   {
      UE_LOG(LogTATAuraWorldSubsystem, Log, TEXT("Found %d invalid aura actor(s)."), invalidActorCount);
   }
}

void UTATAurasWorldSubsystem::_populateActiveAuraActors()
{
   AURA_STAT_SCOPE(_populateActiveAuraActors);
   _activeAuraActors.Reset();

   //assume one component per actor
   _activeAuraActors.Reserve(_auraVisibilityTargets.Num());

   for (TWeakObjectPtr<UTATAuraVisibilityTargetComponent> auraVisibilityTargetWeakPtr : _auraVisibilityTargets)
   {
      const UTATAuraVisibilityTargetComponent* auraTarget = auraVisibilityTargetWeakPtr.Get();
      if (!IsValid(auraTarget) || !IsValid(auraTarget->GetOwner()))
      {
         continue;
      }
      if (auraTarget->GetAuraPerceivedVisibilityForLocalPlayer() > ETATAuraVisibilityType::None)
      {
         _activeAuraActors.AddUnique(auraTarget->GetOwner());
      }
   }
}

void UTATAurasWorldSubsystem::GetActiveAuraActors(TArray<AActor*>& activeAuraActors)
{
   AURA_STAT_SCOPE(GetActiveAuraActors);

   activeAuraActors.Reset();

   //assume every aura actor is valid
   activeAuraActors.Reserve(_activeAuraActors.Num());

   for (TWeakObjectPtr<AActor> auraActor : _activeAuraActors)
   {
      if (!auraActor.IsValid())
      {
         continue;
      }

      activeAuraActors.Add(auraActor.Get());
   }
}
