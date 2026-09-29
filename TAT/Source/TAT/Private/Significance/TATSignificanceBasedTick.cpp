// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Significance/TATSignificanceBasedTick.h"

// ue
#include "SignificanceManager.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSignificanceBasedTick)


namespace TickSignificance
{
   static const FName kSignificanceTag = "SignificanceTick";

   static float GetSignificanceForDistance(TWeakObjectPtr<const UTATSignificanceBasedTickConfig> weakConfig, const float distanceSquared)
   {
      const UTATSignificanceBasedTickConfig* config = weakConfig.Get();
      if (config == nullptr)
      {
         return 0.0f;
      }

      float interval = 0;
      for (const FTATSigificanceTickLodThreshold& threshold : config->SignificanceThresholds)
      {
         interval = threshold.TickInterval;
         if (distanceSquared < FMath::Square(threshold.MaxDistanceToViewer))
         {
            break;
         }
      }

      // significance is sorted descending by default (will use the highest)
      // so just encoding the significance as the negative interval is an easy way
      // to achieve that. Other ways to do if if we need it to be positive, but not
      // sure that we care.
      return -interval;
   }

   static float ActorSignificanceFunction(TWeakObjectPtr<const UTATSignificanceBasedTickConfig> weakConfig, const USignificanceManager::FManagedObjectInfo* objectInfo, const FTransform& viewport)
   {
      const AActor* actor = CastChecked<AActor>(objectInfo->GetObject());
      const float distanceSquared = FVector::DistSquared(actor->GetActorLocation(), viewport.GetLocation());
      return GetSignificanceForDistance(weakConfig, distanceSquared);
   }

   static float ComponentSignificanceFunction(TWeakObjectPtr<const UTATSignificanceBasedTickConfig> weakConfig, const USignificanceManager::FManagedObjectInfo* objectInfo, const FTransform& viewport)
   {
      const UActorComponent* component = CastChecked<UActorComponent>(objectInfo->GetObject());
      const float distanceSquared = FVector::DistSquared(component->GetOwner()->GetActorLocation(), viewport.GetLocation());
      return GetSignificanceForDistance(weakConfig, distanceSquared);
   }

   static void ActorPostSignificanceFunction(const USignificanceManager::FManagedObjectInfo* objectInfo,
      float oldSignificance,
      float significanceValue,
      bool bFinal)
   {
      if (oldSignificance != significanceValue && !bFinal)
      {
         // significance
         const float interval = -significanceValue;
         AActor* actor = CastChecked<AActor>(objectInfo->GetObject());
         if (interval > 0)
         {
            // Set the initial cooldown to a random value in the interval to reduce clustering
            // Setting the tick interval afterward will then set the tick interval when scheduled the next time
            actor->PrimaryActorTick.UpdateTickIntervalAndCoolDown(FMath::RandRange(0.f, interval));
         }
         actor->PrimaryActorTick.TickInterval = interval;
      }
   }

   static void ComponentPostSignificanceFunction(const USignificanceManager::FManagedObjectInfo* objectInfo,
   float oldSignificance,
   float significanceValue,
   bool bFinal)
   {
      if (oldSignificance != significanceValue && !bFinal)
      {
         // significance
         const float interval = -significanceValue;
         UActorComponent* component = CastChecked<UActorComponent>(objectInfo->GetObject());
         if (interval > 0)
         {
            // Set the initial cooldown to a random value in the interval to reduce clustering
            // Setting the tick interval afterward will then set the tick interval when scheduled the next time
            component->PrimaryComponentTick.UpdateTickIntervalAndCoolDown(FMath::RandRange(0.f, interval));
         }
         component->PrimaryComponentTick.TickInterval = interval;
      }
   }
}

void UTATSignificanceBasedTickConfig::RegisterComponent(UActorComponent* component, const UTATSignificanceBasedTickConfig* config)
{
   check(component != nullptr);
   if (config)
   {
      if (USignificanceManager* significanceManager = USignificanceManager::Get(component->GetWorld()))
      {
         significanceManager->RegisterObject(component, TickSignificance::kSignificanceTag,
            [config = MakeWeakObjectPtr(config)] (const USignificanceManager::FManagedObjectInfo* objectInfo, const FTransform& viewport){ return TickSignificance::ComponentSignificanceFunction(config, objectInfo, viewport); },
            USignificanceManager::EPostSignificanceType::Sequential,
            TickSignificance::ComponentPostSignificanceFunction);
      }
   }
}

void UTATSignificanceBasedTickConfig::RegisterActor(AActor* actor, const UTATSignificanceBasedTickConfig* config)
{
   check(actor != nullptr);
   if (config)
   {
      if (USignificanceManager* significanceManager = USignificanceManager::Get(actor->GetWorld()))
      {
         significanceManager->RegisterObject(actor, TickSignificance::kSignificanceTag,
            [config = MakeWeakObjectPtr(config)] (const USignificanceManager::FManagedObjectInfo* objectInfo, const FTransform& viewport){ return TickSignificance::ActorSignificanceFunction(config, objectInfo, viewport); },
            USignificanceManager::EPostSignificanceType::Sequential,
            TickSignificance::ActorPostSignificanceFunction);
      }
   }
}

void UTATSignificanceBasedTickConfig::Unregister(UObject* object)
{
   check(object != nullptr);
   if (USignificanceManager* significanceManager = USignificanceManager::Get(object->GetWorld()))
   {
      significanceManager->UnregisterObject(object);
   }
}
