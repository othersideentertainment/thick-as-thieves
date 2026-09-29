// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/Electrical/TATPowerNetworkComponent.h"

// tat
#include "Interactables/Electrical/TATPowerNetworkInterface.h"
#include "Interactables/Electrical/TATPowerNetworkSubsystem.h"
#include "Interactables/Electrical/TATPowerSource.h"

// ue
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPowerNetworkComponent)

UTATPowerNetworkComponent::UTATPowerNetworkComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   SetIsReplicatedByDefault(true);
   bWantsInitializeComponent = true;
}

// static
UTATPowerNetworkComponent* UTATPowerNetworkComponent::GetPowerNetworkComponent(const AActor* actor)
{
   if (actor == nullptr)
   {
      return nullptr;
   }

   // Getting the component from the power network interface is the fastest approach, but is only supported for actor classes defined in native
   if (const ITATPowerNetworkInterface* powerNetworkInterface = Cast<ITATPowerNetworkInterface>(actor))
   {
      if (UTATPowerNetworkComponent* powerNetworkComponent = powerNetworkInterface->GetPowerNetworkComponent())
      {
         return powerNetworkComponent;
      }
   }

   // Blueprint defined actors with a power network component won't have the interface, so we'll need to find the component another way

   // The power subsystem should have the component cached - if it does, we can avoid searching the actor's entire component array
   if (UWorld* world = actor->GetWorld())
   {
      if (UTATPowerNetworkSubsystem* powerSubsystem = world->GetSubsystem<UTATPowerNetworkSubsystem>())
      {
         if (UTATPowerNetworkComponent* result = powerSubsystem->GetPowerNetworkComponentForActor(actor))
         {
            return result;
         }
      }
   }

   // Search the actor's entire component array as a fallback
   return actor->GetComponentByClass<UTATPowerNetworkComponent>();
}

#if WITH_EDITOR

void UTATPowerNetworkComponent::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   const FName propName = propertyChangedEvent.GetPropertyName();
   if (propName == GET_MEMBER_NAME_CHECKED(UTATPowerNetworkComponent, PowerNetworkRole)
      || (PowerNetworkRole == ETATPowerNetworkRole::Connector && propName == GET_MEMBER_NAME_CHECKED(UTATPowerNetworkComponent, ConnectorLinks))
      || (PowerNetworkRole == ETATPowerNetworkRole::Provider && propName == GET_MEMBER_NAME_CHECKED(UTATPowerNetworkComponent, PowerLink))
      || (PowerNetworkRole == ETATPowerNetworkRole::Consumer && propName == GET_MEMBER_NAME_CHECKED(UTATPowerNetworkComponent, PowerLink))
      )
   {
      _RegisterConnections();
   }
}
#endif

void UTATPowerNetworkComponent::OnRegister()
{
   Super::OnRegister();

   if (UWorld* world = GetWorld())
   {
      if (UTATPowerNetworkSubsystem* powerSubsystem = world->GetSubsystem<UTATPowerNetworkSubsystem>())
      {
         powerSubsystem->AddActor(GetOwner());
         _RegisterConnections();
      }
   }
}

void UTATPowerNetworkComponent::OnUnregister()
{
   Super::OnUnregister();

   if (UWorld* world = GetWorld())
   {
      if (UTATPowerNetworkSubsystem* powerSubsystem = world->GetSubsystem<UTATPowerNetworkSubsystem>())
      {
         powerSubsystem->RemoveActor(GetOwner());
         // maybe skip if tearing down?
         for (AActor* actor : GetLinkSlotActors())
         {
            powerSubsystem->UnregisterConnection(GetOwner(), actor);
         }
      }
   }
}

void UTATPowerNetworkComponent::InitializeComponent()
{
   Super::InitializeComponent();

   _powerNetworkState.IsEnabled = StartEnabled;

   if (UWorld* world = GetWorld())
   {
      if (UTATPowerNetworkSubsystem* powerSubsystem = world->GetSubsystem<UTATPowerNetworkSubsystem>())
      {
         powerSubsystem->AddActor(GetOwner());
      }
   }
}

void UTATPowerNetworkComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(UTATPowerNetworkComponent, _powerNetworkState)
}

bool UTATPowerNetworkComponent::IsEnabled() const
{
   return _powerNetworkState.IsEnabled;
}

bool UTATPowerNetworkComponent::IsPowered() const
{
   if (!_powerNetworkState.IsEnabled)
   {
      return false;
   }

   if (AlwaysActAsPowered)
   {
      return true;
   }

   // for power providers, query the actor directly
   if (PowerNetworkRole == ETATPowerNetworkRole::Provider)
   {
      if (ITATPowerNetworkInterface* powerInterface = Cast<ITATPowerNetworkInterface>(GetOwner()))
      {
         if (TOptional<bool> result = powerInterface->IsPowerNetworkProviderPowered())
         {
            return *result;
         }
      }
   }

   return _powerNetworkState.IsPowered;
}

bool UTATPowerNetworkComponent::IsPowerSurgeActive() const
{
   return _powerNetworkState.IsPowerSurge;
}

void UTATPowerNetworkComponent::AuthoritySetEnabled(bool newEnabled)
{
   check(GetOwner()->HasAuthority());
   if (_powerNetworkState.IsEnabled == newEnabled)
   {
      return;
   }

   GetOwner()->FlushNetDormancy();

   const FTATPowerNetworkComponentState prevState = _powerNetworkState;
   _powerNetworkState.IsEnabled = newEnabled;

   // If we get disabled during a power surge, immediately end the power surge
   if (!newEnabled && _powerNetworkState.IsPowerSurge)
   {
      ensure(_authorityPowerSurgeCount > 0);
      _authorityPowerSurgeCount = 0;
      _powerNetworkState.IsPowerSurge = false;
   }

   _OnRep_PowerNetworkState(_powerNetworkState);

   // Propagate the change across the power network
   _AuthorityUpdatePowerNetworkState();
}

bool UTATPowerNetworkComponent::AuthorityTriggerPowerSurge(float durationSeconds, bool propagateAcrossPowerNetwork, bool propagateEnabledOnly)
{
   check(GetOwner()->HasAuthority());
   // (2025-02-26): Disabling the requirement that the component be powered to power surge for now.
   //               Requiring a link to an external power source was not in the stated design, and they are
   //               inconsistently powered in the level, which will be perceived as a bug. Should revisit
   //               w/ design to determine whether we want to support this, and and whether we want to
   //               *require* this. (If optional-but-possible, possibly more of a self-powered thing than this)
   if (durationSeconds <= 0.0f /*|| !IsPowered()*/)
   {
      return false;
   }

   FTimerHandle handle;
   static constexpr bool looping = false;

   if (propagateAcrossPowerNetwork)
   {
      UTATPowerNetworkSubsystem* powerSubsystem = GetWorld()->GetSubsystem<UTATPowerNetworkSubsystem>();
      if (powerSubsystem == nullptr)
      {
         return false;
      }

      powerSubsystem->TraversePowerNetworkComponents([](UTATPowerNetworkComponent* comp)
      {
         check(comp != nullptr);
         comp->_AuthorityActivatePowerSurge();
         return FTATPowerNetworkTraversalState::ContinueTraversal;
      }, propagateEnabledOnly, GetOwner());

      GetWorld()->GetTimerManager().SetTimer(handle, FTimerDelegate::CreateUObject(this, &UTATPowerNetworkComponent::_AuthorityOnPowerSurgeEnded), durationSeconds, looping);
   }
   else
   {
      // Simple non-propagation version. Just activate it locally and then deactivate after the specified interval.
      _AuthorityActivatePowerSurge();

      GetWorld()->GetTimerManager().SetTimer(handle, FTimerDelegate::CreateLambda([weakSelf = MakeWeakObjectPtr(this)]()
      {
         if (UTATPowerNetworkComponent* self = weakSelf.Get())
         {
            self->_AuthorityDeactivatePowerSurge();
         }
      }), durationSeconds, looping);
   }

   return true;
}

int32 UTATPowerNetworkComponent::NumLinkSlots() const
{
   switch (PowerNetworkRole)
   {
   case ETATPowerNetworkRole::Junction:
      return 0;
   case ETATPowerNetworkRole::Connector:
      return 2;
   case ETATPowerNetworkRole::Provider:
      // fallthrough
   case ETATPowerNetworkRole::Consumer:
      return 1;
   default:
      break;
   }
   return 0;
}

bool UTATPowerNetworkComponent::GetActorConnectedToLinkSlot(int32 index, AActor*& outConnectedActor) const
{
   TArrayView<TObjectPtr<AActor>> connectedActors = GetLinkSlotActors();
   if (connectedActors.IsValidIndex(index))
   {
      outConnectedActor = connectedActors[index];
      return true;
   }
   outConnectedActor = nullptr;
   return false;
}

TArrayView<TObjectPtr<AActor>> UTATPowerNetworkComponent::GetLinkSlotActors() const
{
   UTATPowerNetworkComponent* nonConstThis = const_cast<UTATPowerNetworkComponent*>(this);

   switch (PowerNetworkRole)
   {
   case ETATPowerNetworkRole::Junction:
      break;
   case ETATPowerNetworkRole::Connector:
      return TArrayView<TObjectPtr<AActor>>(nonConstThis->ConnectorLinks, 2);
   case ETATPowerNetworkRole::Provider:
      // fallthrough
   case ETATPowerNetworkRole::Consumer:
      return TArrayView<TObjectPtr<AActor>>(&nonConstThis->PowerLink, 1);
   default:
      break;
   }

   return TArrayView<TObjectPtr<AActor>>();
}

bool UTATPowerNetworkComponent::HasDirectConnectionTo(const AActor* actor, int32* outSlotIndex) const
{
   if (actor != nullptr)
   {
      TArrayView<TObjectPtr<AActor>> slots = GetLinkSlotActors();
      for (int32 i = 0; i < slots.Num(); i++)
      {
         if (actor == slots[i])
         {
            if (outSlotIndex != nullptr)
            {
               *outSlotIndex = i;
            }
            return true;
         }
      }
   }

   if (outSlotIndex != nullptr)
   {
      *outSlotIndex = INDEX_NONE;
   }
   return false;
}

bool UTATPowerNetworkComponent::HasReverseConnectionTo(const AActor* actor) const
{
   if (UWorld* world = GetWorld())
   {
      if (UTATPowerNetworkSubsystem* powerSubsystem = world->GetSubsystem<UTATPowerNetworkSubsystem>())
      {
         return powerSubsystem->GetReverseConnections(GetOwner()).Contains(actor);
      }
   }
   return false;
}

bool UTATPowerNetworkComponent::GetWorldLocationForLinkedActor(const AActor* linkedActor, FVector& worldLocation, bool forDebugVis) const
{
   if (linkedActor == nullptr)
   {
      worldLocation = FVector::ZeroVector;
      return false;
   }

   auto getWorldLocationForIndex = [&](int32 index) -> bool
   {
      // Try and get the location directly from the power actor
      if (ITATPowerNetworkInterface* ownerPowerInterface = Cast<ITATPowerNetworkInterface>(GetOwner()))
      {
         if (TOptional<FVector> loc = ownerPowerInterface->GetPowerNetworkWorldLocationForConnectionIndex(index, forDebugVis))
         {
            worldLocation = *loc;
            return true;
         }
      }
      return false;
   };

   // Do we have any connection to this actor?
   int32 linkIndex = INDEX_NONE;
   if (HasDirectConnectionTo(linkedActor, &linkIndex) && getWorldLocationForIndex(linkIndex))
   {
      return true;
   }
   if (HasReverseConnectionTo(linkedActor) && getWorldLocationForIndex(INDEX_NONE))
   {
      return true;
   }

   worldLocation = FVector::ZeroVector;
   return false;
}

bool UTATPowerNetworkComponent::GetWorldLocationForConnectorIndex(int32 index, FVector& worldLocation, bool forDebugVis) const
{
   AActor* owner = GetOwner();
   if (owner == nullptr)
   {
      worldLocation = FVector::ZeroVector;
      return false;
   }
   if (ITATPowerNetworkInterface* ownerPowerInterface = Cast<ITATPowerNetworkInterface>(GetOwner()))
   {
      if (TOptional<FVector> loc = ownerPowerInterface->GetPowerNetworkWorldLocationForConnectionIndex(index, forDebugVis))
      {
         worldLocation = *loc;
         return true;
      }
   }
   // May as well default to returning the actor location for failures
   worldLocation = owner->GetActorLocation();
   return false;
}

void UTATPowerNetworkComponent::_RegisterConnections()
{
   UWorld* world = GetWorld();
   UTATPowerNetworkSubsystem* powerSubsystem = (world != nullptr) ? world->GetSubsystem<UTATPowerNetworkSubsystem>() : nullptr;
   if (powerSubsystem == nullptr)
   {
      return;
   }

   AActor* owner = GetOwner();
   TArrayView<TObjectPtr<AActor>> currentSlots = GetLinkSlotActors();

#if WITH_EDITOR
   for (TWeakObjectPtr<AActor> weakPrevious : _editorRegisteredConnections)
   {
      AActor* previous = weakPrevious.Get();
      if (previous && !currentSlots.Contains(previous))
      {
         powerSubsystem->UnregisterConnection(owner, previous);
      }
   }
   _editorRegisteredConnections.Reset();
#endif

   for(AActor* actor : currentSlots)
   {
      if (actor)
      {
         powerSubsystem->RegisterConnection(owner, actor);
#if WITH_EDITOR
         _editorRegisteredConnections.Add(actor);
#endif
      }
   }
}

void UTATPowerNetworkComponent::_AuthorityUpdatePoweredState(bool newPowered)
{
   check(GetOwner()->HasAuthority());

   if (newPowered == _powerNetworkState.IsPowered)
   {
      return;
   }

   GetOwner()->FlushNetDormancy();

   const FTATPowerNetworkComponentState prevState = _powerNetworkState;

   // If power gets turned off during a power surge, immediately end the power surge
   if (!newPowered && _powerNetworkState.IsPowerSurge)
   {
      ensure(_authorityPowerSurgeCount > 0);
      _authorityPowerSurgeCount = 0;
      _powerNetworkState.IsPowerSurge = false;
   }

   _powerNetworkState.IsPowered = newPowered;

   _OnRep_PowerNetworkState(prevState);
}

void UTATPowerNetworkComponent::_AuthorityUpdatePowerNetworkState()
{
   check(GetOwner()->HasAuthority());
   UWorld* world = GetWorld();
   check(world != nullptr);
   if (UTATPowerNetworkSubsystem* powerSubsystem = world->GetSubsystem<UTATPowerNetworkSubsystem>())
   {
      powerSubsystem->AuthorityNotifyPowerComponentStateChanged(this);
   }
}

void UTATPowerNetworkComponent::_OnPowerStateChanged(const FTATPowerNetworkComponentState& prevState, const FTATPowerNetworkComponentState& newState)
{
   check(prevState != newState);

   OnPowerStateChanged.Broadcast(prevState, newState);

   if (prevState.IsEnabled != newState.IsEnabled)
   {
      OnEnabledChanged.Broadcast(newState.IsEnabled);
   }

   if (prevState.IsPowered != newState.IsPowered)
   {
      OnPoweredChanged.Broadcast(newState.IsPowered);
   }

   if (prevState.IsPowerSurge != newState.IsPowerSurge)
   {
      OnPowerSurgeChanged.Broadcast(newState.IsPowerSurge);
   }
}

void UTATPowerNetworkComponent::_AuthorityOnPowerSurgeEnded()
{
   check(GetOwner()->HasAuthority());
   UTATPowerNetworkSubsystem* powerSubsystem = GetWorld()->GetSubsystem<UTATPowerNetworkSubsystem>();
   if (!ensureMsgf(powerSubsystem != nullptr, TEXT("Failed to end power surge - power network subsystem was null")))
   {
      return;
   }

   // Deactivate for all connected components.
   // Any components that were disabled during the power surge should have their power surge disabled already.
   static constexpr bool requireEnabled = true;
   powerSubsystem->TraversePowerNetworkComponents([](UTATPowerNetworkComponent* comp)
   {
      check(comp != nullptr);
      comp->_AuthorityDeactivatePowerSurge();
      return FTATPowerNetworkTraversalState::ContinueTraversal;
   }, requireEnabled, GetOwner());
}

void UTATPowerNetworkComponent::_AuthorityActivatePowerSurge()
{
   check(GetOwner()->HasAuthority());
   const bool triggerPowerSurge = _authorityPowerSurgeCount == 0;
   ++_authorityPowerSurgeCount;
   if (triggerPowerSurge)
   {
      ensure(!_powerNetworkState.IsPowerSurge);
      GetOwner()->FlushNetDormancy();
      const FTATPowerNetworkComponentState prevState = _powerNetworkState;
      _powerNetworkState.IsPowerSurge = true;
      _OnRep_PowerNetworkState(prevState);
   }
}

void UTATPowerNetworkComponent::_AuthorityDeactivatePowerSurge()
{
   check(GetOwner()->HasAuthority());
   if (!_powerNetworkState.IsPowerSurge)
   {
      ensure(_authorityPowerSurgeCount == 0);
      return;
   }

   const bool disablePowerSurge = _authorityPowerSurgeCount == 1;
   _authorityPowerSurgeCount = FMath::Max(0, _authorityPowerSurgeCount - 1);
   if (disablePowerSurge)
   {
      check(_powerNetworkState.IsPowerSurge);
      GetOwner()->FlushNetDormancy();
      const FTATPowerNetworkComponentState prevState = _powerNetworkState;
      _powerNetworkState.IsPowerSurge = false;
      _OnRep_PowerNetworkState(prevState);
   }
}

void UTATPowerNetworkComponent::_OnRep_PowerNetworkState(const FTATPowerNetworkComponentState& prevState)
{
   if (prevState == _powerNetworkState)
   {
      return;
   }

   _OnPowerStateChanged(prevState, _powerNetworkState);
}
