// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/Auras/TATAuraVisibilityTargetComponent.h"

// ue
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h"

// tat
#include "Graphics/Auras/TATAurasWorldSubsystem.h"
#include "Graphics/Auras/TATAuraVisibilityPerceiverComponent.h"

// ose
#include "Character/OSECharacterBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogTATAuraVisibilityTarget, Log, All);

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAuraVisibilityTargetComponent)


UTATAuraVisibilityTargetComponent::UTATAuraVisibilityTargetComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   SetIsReplicatedByDefault(true);
}

void UTATAuraVisibilityTargetComponent::BeginPlay()
{
   Super::BeginPlay();

   UTATAurasWorldSubsystem* aiStateWorldSubsystem = GetWorld()->GetSubsystem<UTATAurasWorldSubsystem>();
   check(IsValid(aiStateWorldSubsystem));

   // Register self with world subsystem so perceivers can find us
   aiStateWorldSubsystem->RegisterAuraVisibilityTarget(MakeWeakObjectPtr(this));
      
   // Bind to any perceivers removed in the future, so we know to clear any associated state
   aiStateWorldSubsystem->OnAuraVisibilityPerceiverUnregistered.AddUObject(this, &UTATAuraVisibilityTargetComponent::_OnAuraPerceiverUnregisteredWithWorld);
}

void UTATAuraVisibilityTargetComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   // Unregister self with any aura perceivers
   UTATAurasWorldSubsystem* aiStateWorldSubsystem = GetWorld()->GetSubsystem<UTATAurasWorldSubsystem>();
   if (IsValid(aiStateWorldSubsystem))
   {
      for (TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent> auraPerceiver : aiStateWorldSubsystem->GetAuraVisibilityPerceivers())
      {
         check(auraPerceiver.IsValid());
         check(auraPerceiver->ShouldEvaluateAnyAuraSenses());
         _OnAuraPerceiverUnregisteredWithWorld(auraPerceiver);
      }

      // Unbind from aura perceiver unregister events
      aiStateWorldSubsystem->OnAuraVisibilityPerceiverUnregistered.RemoveAll(this);

      // Unregister ourselves from world subsystem to avoid perceivers evaluating senses on us (or our auras being rendered)
      aiStateWorldSubsystem->UnregisterAuraVisibilityTarget(MakeWeakObjectPtr(this));
   }

   Super::EndPlay(endPlayReason);
}


void UTATAuraVisibilityTargetComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UTATAuraVisibilityTargetComponent, _sharedAuraVisibility);
}

void UTATAuraVisibilityTargetComponent::_OnAuraPerceiverUnregisteredWithWorld(TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver)
{
   check(auraVisibilityPerceiver.IsValid());
   check(auraVisibilityPerceiver->ShouldEvaluateAnyAuraSenses());

   UE_LOG(LogTATAuraVisibilityTarget, Verbose, TEXT("%s - detected aura perceiver %s unregistered with world subsystem! unregistering self with perceiver...")
      , *GetOwner()->GetName()
      , *auraVisibilityPerceiver->GetOwner()->GetName());


   // Update both local / share aura state arrays, removing entries pertaining to this perceiver
   _auraPerceivedVisibilityStateArrayAuthority.RemoveAuraEntriesForPerceiver(this, auraVisibilityPerceiver);
   _auraPerceivedVisibilityStateArrayLocalOnly.RemoveAuraEntriesForPerceiver(this, auraVisibilityPerceiver);

   // Refresh shared aura visibility in case it changed
   if (GetOwner()->HasAuthority())
   {
      _sharedAuraVisibility = _auraPerceivedVisibilityStateArrayAuthority.ResolveLocalAuraVisibility();
   }

   // Refresh visuals for players
   if(!IsNetMode(NM_DedicatedServer))
   {
      _RefreshAuraPerceivedVisibility();
   }
}

void UTATAuraVisibilityTargetComponent::_OnRep_SharedAuraVisibility(ETATAuraVisibilityType oldSharedVisibility)
{
   if (_sharedAuraVisibility != _currentAuraVisibility)
   {
      _RefreshAuraPerceivedVisibility();
   }
}

void UTATAuraVisibilityTargetComponent::UpdateAuraPerceiverSenseEntry(TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver, FGameplayTag auraVisibilityPerceptionTag, bool isBeingPerceived)
{
   check(auraVisibilityPerceiver.IsValid());
   check(auraVisibilityPerceiver->ShouldEvaluateAnyAuraSenses());
   check(auraVisibilityPerceiver->ShouldEvaluateAuraSense(auraVisibilityPerceptionTag));

   // Select aura state array to update depending on whether sense is local-only or shared
   const FTATAuraVisibilityPerceiverSense* auraSense = auraVisibilityPerceiver->GetAuraSense(auraVisibilityPerceptionTag);
   check(auraSense);
   FTATAuraVisibilityStateArray& auraStateArray = auraSense->IsLocalSense() ? _auraPerceivedVisibilityStateArrayLocalOnly : _auraPerceivedVisibilityStateArrayAuthority;

   // Update existing state entry if present
   const bool visibilityChanged = auraStateArray.UpdateAuraPerceiverSenseEntry(this, auraVisibilityPerceiver, auraVisibilityPerceptionTag, isBeingPerceived);
   if (visibilityChanged)
   {
      if (auraSense->IsSharedSense())
      {
         check(GetOwner()->HasAuthority());
         _sharedAuraVisibility = _auraPerceivedVisibilityStateArrayAuthority.ResolveLocalAuraVisibility();
      }

      // Manually refresh perceived aura visibility for listen server host
      if (!IsNetMode(NM_DedicatedServer))
      {
         _RefreshAuraPerceivedVisibility();
      }
   }
}

void UTATAuraVisibilityTargetComponent::OnAuraPersistenceTimerElapsed(TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver, FGameplayTag auraSenseTag)
{
   check(auraVisibilityPerceiver.IsValid());

   UE_LOG(LogTATAuraVisibilityTarget, Verbose, TEXT("%s OnAuraPersistenceTimerElapsed() | perceiver %s sense %s has worn off! Refreshing aura state...")
      , *GetOwner()->GetName()
      , *auraVisibilityPerceiver->GetOwner()->GetName()
      , *auraSenseTag.ToString());

   // Clear timer on associated aura state
   const FTATAuraVisibilityPerceiverSense* auraSense = auraVisibilityPerceiver->GetAuraSense(auraSenseTag);
   check(auraSense);
   FTATAuraVisibilityStateArray& auraStateArray = auraSense->IsLocalSense() ? _auraPerceivedVisibilityStateArrayLocalOnly : _auraPerceivedVisibilityStateArrayAuthority;
   auraStateArray.ClearAuraSensePersistenceTimer(this, auraVisibilityPerceiver.Get(), auraSenseTag);

   // If this timer was associated with a shared sense, we need to re-compute _sharedAuraVisibility so it reps to clients
   if (GetOwner()->HasAuthority())
   {
      if (auraSense->IsSharedSense())
      {
         _sharedAuraVisibility = _auraPerceivedVisibilityStateArrayAuthority.ResolveLocalAuraVisibility();
      }
   }

   // Force an aura visuals refresh on local player machines
   if (!IsNetMode(NM_DedicatedServer))
   {
      _RefreshAuraPerceivedVisibility();
   }
}

ETATAuraVisibilityType UTATAuraVisibilityTargetComponent::GetAuraPerceivedVisibilityForLocalPlayer() const
{
   if (IsNetMode(NM_DedicatedServer))
   {
      UE_LOG(LogTATAuraVisibilityTarget, Warning, TEXT("GetAuraPerceivedVisibilityForLocalPlayer() called by %s on dedicated server!"), *GetOwner()->GetName());
      return ETATAuraVisibilityType::None;
   }

   return _currentAuraVisibility;
}

void UTATAuraVisibilityTargetComponent::_RefreshAuraPerceivedVisibility()
{
   check(!IsNetMode(NM_DedicatedServer));

   const ETATAuraVisibilityType auraVisibilityLocalOnly = _auraPerceivedVisibilityStateArrayLocalOnly.ResolveLocalAuraVisibility();
   const ETATAuraVisibilityType newAuraVisibility = FMath::Max(auraVisibilityLocalOnly, _sharedAuraVisibility);

   // Track perceived aura visibility change and notify bp
   if (_currentAuraVisibility != newAuraVisibility)
   {
      _currentAuraVisibility = newAuraVisibility;
      OnAuraVisibilityRefreshed.Broadcast(newAuraVisibility);
   }
}
