// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/OSEInteractableToggleSwitchComponent.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"
#include "Interactables/OSEInteractableToggle.h"

// ue
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEInteractableToggleSwitchComponent)
DEFINE_LOG_CATEGORY_STATIC(LogOSEInteractableToggleSwitchComponent, Log, All);

UOSEInteractableToggleSwitchComponent::UOSEInteractableToggleSwitchComponent()
{
   SetIsReplicatedByDefault(true);

   bWantsInitializeComponent = true;
   ComponentTags.Add(UOSEInteractionHelpers::kInteractTag);
}

void UOSEInteractableToggleSwitchComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UOSEInteractableToggleSwitchComponent, _lastInteractData);
}

void UOSEInteractableToggleSwitchComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UWorld* world = GetWorld())
   {
      world->GetTimerManager().ClearTimer(_toggleDelayTimer);
   }

   Super::EndPlay(endPlayReason);
}

void UOSEInteractableToggleSwitchComponent::InitializeComponent()
{
   Super::InitializeComponent();

   if (GetWorld()->IsGameWorld())
   {
      // Attempt to use owning actor if unassigned
      if (!_syncedToggle)
      {
         UE_LOG(LogOSEInteractableToggleSwitchComponent, VeryVerbose, TEXT("[%s] _syncedToggle actor unassigned, attempting to cast owner %s as fallback...")
            , *GetName()
            , *GetOwner()->GetName());

         _syncedToggle = Cast<AOSESyncedToggle>(GetOwner());
         UE_CLOG(_syncedToggle == nullptr, LogOSEInteractableToggleSwitchComponent, Error, TEXT("[%s] _syncedToggle actor unassigned, and owner %s is not an AOSESyncedToggle!")
            , *GetName()
            , *GetOwner()->GetName());
      }
   }
}

bool UOSEInteractableToggleSwitchComponent::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   // Block interaction if waiting for delay before toggling target actor
   if (_toggleDelayTimer.IsValid())
   {
      return false;
   }

   if (!_syncedToggle)
   {
      return false;
   }

   if (_usageBehavior == EOSEInteractableToggleSwitchUsageBehavior::UsageTogglesSyncedActor)
   {
      // If we can't toggle the synced actor, see if we should allow switch-interaction anyway
      if (_syncedToggle->IsInPermanentToggledState())
      {
         // TODO: check if in the middle of a timeline transition (to avoid snapping on rapid interaction)
         return _allowCosmeticToggleInPermanentSwitchState;
      }

      return _syncedToggle->CanSwitchState();
   }

   // TODO: check if in the middle of a timeline transition (to avoid snapping on rapid interaction)
   // Allow interaction if cosmetic-only (i.e. doesn't rely on synced actor)
   return _usageBehavior == EOSEInteractableToggleSwitchUsageBehavior::UsageCosmeticOnly;
}

void UOSEInteractableToggleSwitchComponent::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   if (_syncedToggle)
   {
      prompt.PressAction = _togglePrompt;
   }
}

FInteractStartResult UOSEInteractableToggleSwitchComponent::StartInteract_Implementation(ACharacter* interactingCharacter)
{

   if (_syncedToggle)
   {
      // Toggle after delay if specified
      if (_toggleDelaySeconds > 0)
      {
         constexpr bool looping = false;
         UE_LOG(LogOSEInteractableToggleSwitchComponent, Verbose, TEXT("[%s] Toggling %s in %f seconds"), *GetName(), *_syncedToggle->GetName(), _toggleDelaySeconds);
         GetWorld()->GetTimerManager().SetTimer(_toggleDelayTimer, this, &UOSEInteractableToggleSwitchComponent::_OnToggleDelayElapsed, _toggleDelaySeconds, looping);
      }
      // We locally predict the timer, but not the actual toggle (to prevent locally-predicted interaction during the delay)
      else if(GetOwner()->HasAuthority())
      {
         _AuthorityToggleSyncedToggle();
      }
   }
   else
   {
      UE_LOG(LogOSEInteractableToggleSwitchComponent, Error, TEXT("[%s] unassigned _interactableToggle!"), *GetName());
   }
   
   // Replicate interact data to remote clients
   GetOwner()->FlushNetDormancy();
   const FOSEInteractableToggleData oldServerInteractData = _lastInteractData;

   _lastInteractData.Character = interactingCharacter;
   _lastInteractData.ServerTimeSeconds = UOSEInteractionHelpers::GetServerTimeForWrite(this);

   // "Predict" toggle event for non-server (to handle ephemeral SFX)
   _OnRep_ServerLastInteractData(oldServerInteractData);

   FInteractStartResult result;
   result.InstantAnimationTag = _interactInstantAnimationTag;
   return result;
}

void UOSEInteractableToggleSwitchComponent::_SetUsageBehavior(EOSEInteractableToggleSwitchUsageBehavior usageBehavior)
{
   if (_usageBehavior != usageBehavior)
   {
      UE_LOG(LogOSEInteractableToggleSwitchComponent, Verbose, TEXT("_usageBehavior changed (%s => %s)"), *UEnum::GetValueAsString(_usageBehavior), *UEnum::GetValueAsString(usageBehavior));
      _usageBehavior = usageBehavior;
   }
}

void UOSEInteractableToggleSwitchComponent::_AuthorityToggleSyncedToggle()
{
   check(GetOwner()->HasAuthority());
   check(_syncedToggle);
   
   if (_usageBehavior == EOSEInteractableToggleSwitchUsageBehavior::UsageTogglesSyncedActor)
   {
      UE_LOG(LogOSEInteractableToggleSwitchComponent, Verbose, TEXT("[%s] toggling %s"), *GetName(), *_syncedToggle->GetName());
      _syncedToggle->Toggle();
   }
}

void UOSEInteractableToggleSwitchComponent::_OnToggleDelayElapsed()
{
   // We locally predict the timer, but not the actual toggle (to prevent locally-predicted interaction during the delay)
   if (GetOwner()->HasAuthority())
   {
      if (_syncedToggle)
      {
         _AuthorityToggleSyncedToggle();
      }
      else
      {
         UE_LOG(LogOSEInteractableToggleSwitchComponent, Error, TEXT("[%s] _OnToggleDelayElapsed() | _syncedToggle actor not found!"), *GetName());
      }
   }

   // Should never be called without valid timer handle
   ensure(_toggleDelayTimer.IsValid());
   _toggleDelayTimer.Invalidate();
}

void UOSEInteractableToggleSwitchComponent::_OnRep_ServerLastInteractData(const FOSEInteractableToggleData& oldInteractData)
{
   // Ignore a replicated interact time that's within our local-predict-dedupe threshold to avoid duplicate callbacks for a locally-predicted interaction
   if (!GetOwner()->HasAuthority())
   {
      if (_lastInteractData.IsReplicatedDuplicate(oldInteractData, _clientLocalPredictDedupeThresholdSeconds))
      {
         UE_LOG(LogOSEInteractableToggleSwitchComponent, VeryVerbose, TEXT("Ignoring an interact with client-replicated delta of %f seconds (probably replicated duplicate)")
            , _lastInteractData.ServerTimeSeconds - oldInteractData.ServerTimeSeconds);
         return;
      }
   }

   OnToggled.Broadcast(_lastInteractData.Character.Get());

   // Notify BP for ephemeral effects
   if (!UOSEInteractionHelpers::IsOld(this, _lastInteractData.ServerTimeSeconds, _recentInteractThresholdSeconds))
   {
      OnRecentlyToggled.Broadcast(_lastInteractData.Character.Get());
   }
}

FString FOSEInteractableToggleData::ToString() const
{
   return FString::Printf(TEXT("FOSEInteractableToggleData(%f, %s)"), ServerTimeSeconds, Character.IsValid() ? *Character.Get()->GetName() : TEXT("NULL"));
}
