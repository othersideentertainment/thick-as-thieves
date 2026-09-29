// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/Electrical/TATPowerNetworkSubsystem.h"

// tat
#include "Interactables/Electrical/TATPowerNetworkComponent.h"
#include "Interactables/Electrical/TATPowerNetworkUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPowerNetworkSubsystem)

DEFINE_LOG_CATEGORY(LogTATPowerNetwork)

class FTATPowerNetworkVisualizer : public TATPowerNetworkUtils::FPowerNetworkDebugDraw
{
   UWorld* _world = nullptr;

public:
   explicit FTATPowerNetworkVisualizer(UWorld* forWorld)
      : _world(forWorld)
   {
   }

   virtual void DrawLine(const FVector& start, const FVector& end, const FLinearColor& color, float thickness) override
   {
      check(_world != nullptr);
      static constexpr bool persistentLines = false;
      static constexpr float lifeTime = 0.0f;
      static constexpr uint8 depthPriority = 0;
      static constexpr bool sRGB = true;
      DrawDebugLine(_world, start, end, color.ToFColor(sRGB), persistentLines, lifeTime, depthPriority, thickness);
   }
};


void FTATPowerNetworkTraversalState::AddStartingActor(AActor* actor)
{
   if (actor == nullptr || UTATPowerNetworkComponent::GetPowerNetworkComponent(actor) == nullptr)
   {
      return;
   }
   Queue.Enqueue(actor);
}

void FTATPowerNetworkTraversalState::TraverseLinks(FLinkFunctionRef callback, bool requireEnabled)
{
   if (Queue.IsEmpty())
   {
      return;
   }

   Visited.Reset();

   while (!Queue.IsEmpty())
   {
      AActor* thisActor = nullptr;
      const bool success = Queue.Dequeue(thisActor);
      check(success);
      check(thisActor != nullptr);
      Visited.Add(thisActor);

      UTATPowerNetworkComponent* thisPowerComponent = UTATPowerNetworkComponent::GetPowerNetworkComponent(thisActor);
      if (!ensure(thisPowerComponent != nullptr))
      {
         continue;
      }

      if (requireEnabled && !thisPowerComponent->IsEnabled())
      {
         continue;
      }

      // Fire the callback for all direct connections to this actor, then queue up those actors to visit later
      const int32 numConnectedActors = thisPowerComponent->NumLinkSlots();
      for (int32 i = 0; i < numConnectedActors; i++)
      {
         AActor* connectedActor = nullptr;
         if (thisPowerComponent->GetActorConnectedToLinkSlot(i, connectedActor))
         {
            if (UTATPowerNetworkComponent* connectedActorPowerComp = UTATPowerNetworkComponent::GetPowerNetworkComponent(connectedActor))
            {
               if (callback(thisActor, connectedActor) != ContinueTraversal)
               {
                  return;
               }
               if (!Visited.Contains(connectedActor))
               {
                  Queue.Enqueue(connectedActor);
               }
            }
         }
      }

      // Also queue up actors with reverse connections to this one
      for (TWeakObjectPtr<AActor> reverseConnection : _subsystem.GetReverseConnections(thisActor))
      {
         AActor* reverseActorLink = reverseConnection.Get();
         if (reverseActorLink != nullptr && !Visited.Contains(reverseActorLink))
         {
            Queue.Enqueue(reverseActorLink);
         }
      }
   }
}

void FTATPowerNetworkTraversalState::TraverseComponents(FComponentFunctionRef callback, bool requireEnabled)
{
   if (Queue.IsEmpty())
   {
      return;
   }

   Visited.Reset();

   while (!Queue.IsEmpty())
   {
      AActor* thisActor = nullptr;
      const bool success = Queue.Dequeue(thisActor);
      check(success);
      check(thisActor != nullptr);
      Visited.Add(thisActor);

      UTATPowerNetworkComponent* thisPowerComponent = UTATPowerNetworkComponent::GetPowerNetworkComponent(thisActor);
      if (!ensure(thisPowerComponent != nullptr))
      {
         continue;
      }

      if (requireEnabled && !thisPowerComponent->IsEnabled())
      {
         continue;
      }

      if (callback(thisPowerComponent) != ContinueTraversal)
      {
         return;
      }

      // Queue up all actors connected to this power component
      const int32 numConnectedActors = thisPowerComponent->NumLinkSlots();
      for (int32 i = 0; i < numConnectedActors; i++)
      {
         AActor* connectedActor = nullptr;
         if (thisPowerComponent->GetActorConnectedToLinkSlot(i, connectedActor) && connectedActor != nullptr && !Visited.Contains(connectedActor))
         {
            if (UTATPowerNetworkComponent* connectedActorPowerComp = UTATPowerNetworkComponent::GetPowerNetworkComponent(connectedActor))
            {
               Queue.Enqueue(connectedActor);
            }
         }
      }

      // Queue up all reverse connections
      for (TWeakObjectPtr<AActor> reverseConnection : _subsystem.GetReverseConnections(thisActor))
      {
         AActor* reverseActorLink = reverseConnection.Get();
         if (reverseActorLink != nullptr && !Visited.Contains(reverseActorLink))
         {
            Queue.Enqueue(reverseActorLink);
         }
      }
   }
}

bool UTATPowerNetworkSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   return Super::ShouldCreateSubsystem(outer);
}

bool UTATPowerNetworkSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return Super::DoesSupportWorldType(worldType);
}

void UTATPowerNetworkSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   //TODO: Reenable this once this warning is fixed:
   //      LogInit: Display: LogConsoleManager: Warning: Console object named 'PowerNetwork.DumpLog' already exists but is being registered again,
   //      but we weren't expected it to be! (FConsoleManager::AddConsoleObject)

   //IConsoleManager::Get().RegisterConsoleCommand(TEXT("PowerNetwork.DumpLog"),
   //   TEXT("Dumps to the log all power network components and their connections"),
   //   FConsoleCommandDelegate::CreateUObject(this, &UTATPowerNetworkSubsystem::_DumpAllPowerNetworkActorsToLog),
   //   ECVF_Default);
}

void UTATPowerNetworkSubsystem::Deinitialize()
{
   Super::Deinitialize();
}

void UTATPowerNetworkSubsystem::PostInitialize()
{
   Super::PostInitialize();
}

void UTATPowerNetworkSubsystem::OnWorldBeginPlay(UWorld& world)
{
   Super::OnWorldBeginPlay(world);

   if (world.GetAuthGameMode() != nullptr)
   {
      // Make sure all power network components have the correct initial state
      static constexpr bool findAllRequireEnabled = false;
      TraversePowerNetworkComponents([this](UTATPowerNetworkComponent* powerComp) -> bool
      {
         static constexpr bool requireEnabled = true;
         static constexpr bool requirePowered = true;
         const bool hasPower = FindFirstConnectedPowerSource(powerComp->GetOwner(), requireEnabled, requirePowered) != nullptr;
         powerComp->_AuthorityUpdatePoweredState(hasPower);
         return FTATPowerNetworkTraversalState::ContinueTraversal;
      }, findAllRequireEnabled);
   }
}

void UTATPowerNetworkSubsystem::OnWorldComponentsUpdated(UWorld& world)
{
   Super::OnWorldComponentsUpdated(world);
}

void UTATPowerNetworkSubsystem::TraversePowerNetworkLinks(FTATPowerNetworkTraversalState::FLinkFunctionRef callback, bool requireEnabled, AActor* startingActor) const
{
   FTATPowerNetworkTraversalState state(*this);

   if (startingActor != nullptr)
   {
      // If startingActor is NOT null, require that the actor is actually in the network.
      if (!_actorToPowerComponentMap.Contains(startingActor))
      {
         return;
      }
      state.AddStartingActor(startingActor);
   }
   else
   {
      // If startingActor is null, assume we want to iterate over the entire network.
      for (const auto& pair : _actorToPowerComponentMap)
      {
         if (AActor* actor = pair.Key.Get())
         {
            state.AddStartingActor(actor);
         }
      }
   }

   state.TraverseLinks(callback, requireEnabled);
}

void UTATPowerNetworkSubsystem::TraversePowerNetworkComponents(FTATPowerNetworkTraversalState::FComponentFunctionRef callback, bool requireEnabled, AActor* startingActor) const
{
   FTATPowerNetworkTraversalState state(*this);

   if (startingActor != nullptr)
   {
      // If startingActor is NOT null, require that the actor is actually in the network.
      if (!_actorToPowerComponentMap.Contains(startingActor))
      {
         return;
      }
      state.AddStartingActor(startingActor);
   }
   else
   {
      // If startingActor is null, assume we want to iterate over the entire network.
      for (const auto& pair : _actorToPowerComponentMap)
      {
         if (AActor* actor = pair.Key.Get())
         {
            state.AddStartingActor(actor);
         }
      }
   }

   state.TraverseComponents(callback, requireEnabled);
}

void UTATPowerNetworkSubsystem::AddActor(AActor* actor)
{
   if (actor == nullptr)
   {
      return;
   }

   UTATPowerNetworkComponent* powerComponent = UTATPowerNetworkComponent::GetPowerNetworkComponent(actor);
   if (powerComponent == nullptr)
   {
      UE_LOG(LogTATPowerNetwork, Warning, TEXT("Actor '%s' can't be added to the power network subsystem because it does not have a power network component"),
         *GetNameSafe(actor));
      return;
   }

   if (_network.Contains(powerComponent))
   {
      return;
   }

   _network.Add(powerComponent);
   _actorToPowerComponentMap.Add(actor, powerComponent);
}

void UTATPowerNetworkSubsystem::RemoveActor(AActor* actor)
{
   if (actor == nullptr || !_actorToPowerComponentMap.Contains(actor))
   {
      return;
   }

   TWeakObjectPtr<UTATPowerNetworkComponent> powerCompWeak;
   const bool removedFromMap = _actorToPowerComponentMap.RemoveAndCopyValue(actor, powerCompWeak);
   ensure(removedFromMap);

   UTATPowerNetworkComponent* powerComp = powerCompWeak.Get();
   if (powerComp == nullptr)
   {
      powerComp = UTATPowerNetworkComponent::GetPowerNetworkComponent(actor);
   }

   if (ensure(powerComp != nullptr))
   {
      _network.Remove(powerComp);
   }
}

void UTATPowerNetworkSubsystem::RegisterConnection(AActor* source, AActor* target)
{
   if (source == nullptr)
   {
      return;
   }

   _reverseDependencies.FindOrAdd(target).AddUnique(source);
}

void UTATPowerNetworkSubsystem::UnregisterConnection(AActor* source, AActor* target)
{
   if (source == nullptr)
   {
      return;
   }

   if (TArray<TWeakObjectPtr<AActor>>* dependencies = _reverseDependencies.Find(target))
   {
      dependencies->RemoveSingleSwap(source);
   }
}

bool UTATPowerNetworkSubsystem::HasDirectConnectionTo(const AActor* srcActor, const AActor* dstActor) const
{
   if (srcActor == nullptr || dstActor == nullptr)
   {
      return false;
   }
   const TWeakObjectPtr<UTATPowerNetworkComponent>* srcCompWeak = _actorToPowerComponentMap.Find(srcActor);
   if (srcCompWeak != nullptr && srcCompWeak->Get() != nullptr)
   {
      return srcCompWeak->Get()->HasDirectConnectionTo(dstActor);
   }
   return false;
}

TConstArrayView<TWeakObjectPtr<AActor>> UTATPowerNetworkSubsystem::GetReverseConnections(const AActor* actor) const
{
   if (const TArray<TWeakObjectPtr<AActor>>* dependencies = _reverseDependencies.Find(actor))
   {
      return MakeArrayView(*dependencies);
   }

   return TConstArrayView<TWeakObjectPtr<AActor>>();
}

UTATPowerNetworkComponent* UTATPowerNetworkSubsystem::FindFirstConnectedPowerSource(const AActor* actor, bool requireEnabled, bool requirePowered) const
{
   if (actor == nullptr)
   {
      return nullptr;
   }

   UTATPowerNetworkComponent* powerSource = nullptr;

   TraversePowerNetworkComponents([requirePowered, &powerSource](UTATPowerNetworkComponent* powerComp)
   {
      check(powerComp != nullptr);
      if (powerComp->PowerNetworkRole == ETATPowerNetworkRole::Provider && (!requirePowered || powerComp->IsPowered()))
      {
         powerSource = powerComp;
         return FTATPowerNetworkTraversalState::StopTraversal;
      }
      return FTATPowerNetworkTraversalState::ContinueTraversal;
   }, requireEnabled, const_cast<AActor*>(actor));

   return powerSource;
}

UTATPowerNetworkComponent* UTATPowerNetworkSubsystem::GetPowerNetworkComponentForActor(const AActor* actor) const
{
   if (actor != nullptr)
   {
      if (const TWeakObjectPtr<UTATPowerNetworkComponent>* weakComp = _actorToPowerComponentMap.Find(actor))
      {
         return weakComp->Get();
      }
   }
   return nullptr;
}

void UTATPowerNetworkSubsystem::AuthorityNotifyPowerComponentStateChanged(UTATPowerNetworkComponent* powerComp)
{
   if (powerComp == nullptr || powerComp->GetOwner() == nullptr || !powerComp->GetOwner()->HasAuthority())
   {
      return;
   }

#if STATS
   const uint64 startCycles = FPlatformTime::Cycles64();
#endif

   // First, find the full connected cluster of power nodes connected to this one (enabled or not) that care about power state change events.
   TArray<UTATPowerNetworkComponent*, TInlineAllocator<32>> powerCluster;
   {
      constexpr bool requireEnabled = false;
      TraversePowerNetworkComponents([&powerCluster](UTATPowerNetworkComponent* comp)
      {
         if (ensure(comp != nullptr) && comp->ReceivePowerNetworkEvents)
         {
            powerCluster.Add(comp);
         }
         return FTATPowerNetworkTraversalState::ContinueTraversal;
      }, requireEnabled, powerComp->GetOwner());
   }

   // Now, do some graph traversal on each of those power components and route power change events if the current network doesn't match their current state.
   for (UTATPowerNetworkComponent* comp : powerCluster)
   {
      static constexpr bool requireEnabled = true;
      static constexpr bool requirePowered = true;
      const bool hasPower = FindFirstConnectedPowerSource(comp->GetOwner(), requireEnabled, requirePowered) != nullptr;
      comp->_AuthorityUpdatePoweredState(hasPower);
   }

#if STATS
   const uint64 elapsedCycles = FPlatformTime::Cycles64() - startCycles;
   UE_LOG(LogTATPowerNetwork, Verbose, TEXT("UTATPowerNetworkSubsystem::NotifyPowerComponentStateChanged updated %i nodes in %llu cycles (%.4f ms)"),
      powerCluster.Num(), elapsedCycles, FPlatformTime::ToMilliseconds64(elapsedCycles));
#endif
}

void UTATPowerNetworkSubsystem::_OnPowerProviderStateChanged(UTATPowerNetworkComponent* powerComp)
{
   check(powerComp != nullptr);
   check(powerComp->PowerNetworkRole == ETATPowerNetworkRole::Provider);

   const bool newPowerState = powerComp->IsPowered();

   static constexpr bool requireEnabled = true;
   AActor* startingActor = powerComp->GetOwner();
   check(startingActor != nullptr);

   // Get the state of other power providers in the same connected cluster
   int32 numOtherEnabledPowerProviders = 0;
   TraversePowerNetworkComponents([&](UTATPowerNetworkComponent* comp)
   {
      check(comp != nullptr);
      if (comp != powerComp && comp->PowerNetworkRole == ETATPowerNetworkRole::Provider && comp->IsPowered())
      {
         ++numOtherEnabledPowerProviders;
      }
      return FTATPowerNetworkTraversalState::ContinueTraversal;
   }, requireEnabled, startingActor);

   // If there are no other enabled power providers, then we need to propagate the event through the network
   if (numOtherEnabledPowerProviders == 0)
   {
      TraversePowerNetworkComponents([newPowerState](UTATPowerNetworkComponent* comp)
      {
         comp->_AuthorityUpdatePoweredState(newPowerState);
         return FTATPowerNetworkTraversalState::ContinueTraversal;
      }, requireEnabled, startingActor);
   }
}

void UTATPowerNetworkSubsystem::_DumpAllPowerNetworkActorsToLog()
{
#if OSE_CHEATS_ENABLED
   UE_LOG(LogTATPowerNetwork, Log, TEXT("==== Power Network Actor List (%d actors) ===="), _network.Num());

   TArray<AActor*> actors;
   for (UTATPowerNetworkComponent* comp : _network)
   {
      if (comp != nullptr && comp->GetOwner() != nullptr)
      {
         actors.Add(comp->GetOwner());
      }
   }

   // How these are sorted isn't particularly important, but because we're building the array from a TSet,
   // sorting by anything is handy so the output order can be deterministic.
   actors.Sort([](AActor& lhs, AActor& rhs)
   {
      // Sort by distance to the world origin.
      return FVector::DistSquared(FVector::ZeroVector, lhs.GetActorLocation()) < FVector::DistSquared(FVector::ZeroVector, rhs.GetActorLocation());
   });

   for (int32 i = 0; i < actors.Num(); i++)
   {
      UTATPowerNetworkComponent* powerSource = FindFirstConnectedPowerSource(actors[i], true, true);
      UE_LOG(LogTATPowerNetwork, Log, TEXT("    [%d] %s (power source = %s)"), i, *GetNameSafe(actors[i]), *GetNameSafe(powerSource));
   }

   if (_network.Num() > 0)
   {
      UE_LOG(LogTATPowerNetwork, Log, TEXT("==== Power Network Links ===="));
      static constexpr bool requireEnabled = false;
      TraversePowerNetworkLinks([&](AActor* actorA, AActor* actorB)
      {
         UE_LOG(LogTATPowerNetwork, Log, TEXT("    [%s] -> [%s]"), *GetNameSafe(actorA), *GetNameSafe(actorB));
         return FTATPowerNetworkTraversalState::ContinueTraversal;
      }, requireEnabled);
   }
#endif
}

void UTATPowerNetworkSubsystem::TogglePowerNetworkDebugVis(APlayerController* pc)
{
#if OSE_CHEATS_ENABLED
   if (pc == nullptr)
   {
      return;
   }

   FTimerHandle* timerHandlePtr = _debugVisTickTimerHandles.Find(pc);
   if (timerHandlePtr != nullptr && timerHandlePtr->IsValid())
   {
      GetWorld()->GetTimerManager().ClearTimer(*timerHandlePtr);
      _debugVisTickTimerHandles.Remove(pc);
      return;
   }

   if (timerHandlePtr == nullptr)
   {
      timerHandlePtr = &_debugVisTickTimerHandles.Add(pc);
   }

   check(timerHandlePtr != nullptr);

   FTimerManagerTimerParameters params{};
   params.bLoop = true;
   params.bMaxOncePerFrame = true;
   static constexpr float tickInterval = 1.0f / 120.0f;
   TWeakObjectPtr<APlayerController> pcWeak = pc;
   GetWorld()->GetTimerManager().SetTimer(*timerHandlePtr, FTimerDelegate::CreateUObject(this, &UTATPowerNetworkSubsystem::_TickPowerNetworkDebugVis, pcWeak), tickInterval, params);
#endif
}

void UTATPowerNetworkSubsystem::_TickPowerNetworkDebugVis(TWeakObjectPtr<APlayerController> pcWeak)
{
#if OSE_CHEATS_ENABLED
   APlayerController* pc = pcWeak.Get();
   if (pc == nullptr)
   {
      _debugVisTickTimerHandles.Remove(pcWeak);
      return;
   }

   UWorld* world = pc->GetWorld();
   check(world != nullptr && world->IsGameWorld());

   FTATPowerNetworkVisualizer visualizer{ world };
   TSet<AActor*, DefaultKeyFuncs<AActor*>, TInlineSetAllocator<64>> drawnActors;

   auto drawActor = [&](AActor* actor)
   {
      check(actor != nullptr);
      if (drawnActors.Contains(actor))
      {
         return;
      }
      drawnActors.Add(actor);

      static constexpr float lineThickness = 2.0f;
      static constexpr FLinearColor powerSurgeColor = FLinearColor(0.25f, 0.25f, 1.0f);
      static constexpr float powerSurgeLineOffset = 25.0f;

      UTATPowerNetworkComponent* powerComp = UTATPowerNetworkComponent::GetPowerNetworkComponent(actor);
      check(powerComp != nullptr);
      if (powerComp->PowerNetworkRole == ETATPowerNetworkRole::Connector)
      {
         FLinearColor baseColor = FLinearColor::White;
         TATPowerNetworkUtils::FDashedLine dashed{};

         if (powerComp->IsPowered() && powerComp->IsEnabled())
         {
            baseColor = FLinearColor::Yellow;
         }
         else if (!powerComp->IsEnabled())
         {
            baseColor = TATPowerNetworkUtils::ApplyColorIntensity(FLinearColor::Red, 0.5f);
            dashed = TATPowerNetworkUtils::FDashedLine(FLinearColor::Black);
         }
         else if (!powerComp->IsPowered())
         {
            baseColor = FLinearColor::Gray;
            dashed = TATPowerNetworkUtils::FDashedLine(FLinearColor::Black);
         }
         visualizer.DrawActorSplineComponent(actor, baseColor, lineThickness, dashed);

         if (powerComp->IsPowerSurgeActive())
         {
            visualizer.DrawActorSplineComponent(actor, powerSurgeColor, lineThickness, {}, powerSurgeLineOffset);
         }
      }
      else
      {
         const float colorIntensity = powerComp->IsPowered() ? 1.0f : 0.33f;
         const FLinearColor boundingBoxColor = TATPowerNetworkUtils::ApplyColorIntensity(TATPowerNetworkUtils::GetColorForRole(powerComp->PowerNetworkRole), colorIntensity);
         visualizer.DrawActorBoundingBox(actor, boundingBoxColor, lineThickness);

         if (powerComp->IsPowerSurgeActive())
         {
            visualizer.DrawActorBoundingBox(actor, powerSurgeColor, lineThickness, powerSurgeLineOffset);
         }
      }
   };

   static constexpr bool requireEnabled = false;
   TraversePowerNetworkLinks([&](AActor* baseActor, AActor* linkedActor) -> bool
   {
      UTATPowerNetworkComponent* baseComp = UTATPowerNetworkComponent::GetPowerNetworkComponent(baseActor);
      UTATPowerNetworkComponent* linkedComp = UTATPowerNetworkComponent::GetPowerNetworkComponent(linkedActor);
      check(baseComp != nullptr);
      check(linkedComp != nullptr);

      static constexpr float lineThickness = 1.0f;

      drawActor(baseActor);
      drawActor(linkedActor);
      visualizer.DrawPowerLink(baseActor, linkedActor, FLinearColor::White, lineThickness);
      visualizer.DrawPowerLink(linkedActor, baseActor, FLinearColor::White, lineThickness, FVector::UpVector * 10.0f);

      return FTATPowerNetworkTraversalState::ContinueTraversal;
   }, requireEnabled);
#endif
}
