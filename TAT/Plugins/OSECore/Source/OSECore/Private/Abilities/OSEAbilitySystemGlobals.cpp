// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEAbilitySystemGlobals.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/OSEAbilityFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilitySystemGlobals)

UOSEAbilitySystemComponent* UOSEAbilitySystemGlobals::GetOSEAbilitySystemComponentFromActor(const AActor* actor, bool lookForComponent /*= false*/)
{
   return Cast<UOSEAbilitySystemComponent>(GetAbilitySystemComponentFromActor(actor, lookForComponent));
}

FGameplayEffectContext* UOSEAbilitySystemGlobals::AllocGameplayEffectContext() const
{
   return new FOSEGameplayEffectContext();
}

void UOSEAbilitySystemGlobals::InitGlobalData()
{
   Super::InitGlobalData();

   // Register for PreloadMap so cleanup can occur on map transitions
   FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UOSEAbilitySystemGlobals::OSEHandlePreLoadMap);
}

AGameplayAbilityTargetActor* UOSEAbilitySystemGlobals::GetTargetActor(UWorld* world, TSubclassOf<AGameplayAbilityTargetActor> actorClass, EGameplayTargetingConfirmation::Type confirmationType)
{
   AGameplayAbilityTargetActor* poolActor = nullptr;

   // Only pool instant actors
   // This reset code is fairly limited, it will not work on actors that have a lot of custom BP logic and construction scripts, 
   // but those should not be used as instant target actors
   // We don't unregister the components, we just make them invisible
   if (confirmationType == EGameplayTargetingConfirmation::Instant)
   {
      FOSEPooledTargetActors& pool = _targetActorClassPool.FindOrAdd(*actorClass);
      for (int32 i = pool.InactiveList.Num() - 1; i >= 0; i--)
      {
         poolActor = pool.InactiveList[i];

         // Filter out actors for a different world (for PIE)
         // CONSIDER: If this becomes fragile or annoying, possible move to something instantiated per-world rather than global
         const bool alive = poolActor && !poolActor->IsPendingKillPending();
         if (alive && poolActor->GetWorld() == world)
         {
            pool.InactiveList.RemoveAt(i);
            poolActor->SetActorHiddenInGame(false);
            poolActor->SetActorTickEnabled(true);

            pool.ActiveList.Add(poolActor);
            return poolActor;
         }
         else if (!alive)
         {
            pool.InactiveList.RemoveAt(i);
         }
      }

      poolActor = world->SpawnActorDeferred<AGameplayAbilityTargetActor>(*actorClass, FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
      pool.ActiveList.Add(poolActor);
   }
   else
   {
      // Not pooled, just make a new one and don't add to active list
      poolActor = world->SpawnActorDeferred<AGameplayAbilityTargetActor>(*actorClass, FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
   }

   return poolActor;
}

void UOSEAbilitySystemGlobals::DoneWithTargetActor(AGameplayAbilityTargetActor* targetActor)
{
   FOSEPooledTargetActors& pool = _targetActorClassPool.FindOrAdd(targetActor->GetClass());

   // See if it's in the pool
   for (int32 i = pool.ActiveList.Num() - 1; i >= 0; i--)
   {
      AGameplayAbilityTargetActor* poolActor = pool.ActiveList[i];
      if (!poolActor || poolActor->IsPendingKillPending())
      {
         // Actor has gone bad, don't re-add to pool
         pool.ActiveList.RemoveAt(i);
      }
      else if (poolActor == targetActor && poolActor->GetWorld() && pool.InactiveList.Num() <= MaxTargetActorPoolSize)
      {
         // Return to inactive pool
         pool.ActiveList.RemoveAt(i);
         pool.InactiveList.Add(poolActor);

         // This is a very limited reset, would not be enough for something like a character
         poolActor->SetActorHiddenInGame(true);
         poolActor->SetActorTickEnabled(false);
         poolActor->GetWorld()->GetLatentActionManager().RemoveActionsForObject(poolActor);

         // Reset the spawn params, the bp nodes will only set overridden params so we need to set others back to defaults
         UOSEAbilityFunctionLibrary::ResetSpawnParameters(poolActor);

         // Disable confirmation destroy as we have pooled it
         poolActor->bDestroyOnConfirmation = false;
         poolActor->OwningAbility = nullptr;
         poolActor->PrimaryPC = nullptr;
         poolActor->SourceActor = nullptr;

         // TODO add more target actor-specific reset logic if needed
         return;
      }
   }

   // Not pooled, just destroy      
   targetActor->Destroy();
}

void UOSEAbilitySystemGlobals::OSEHandlePreLoadMap(const FString& mapName)
{
   _targetActorClassPool.Reset();
}

