// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATCupboardDoorComponent.h"

// ue
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

// tat
#include "Graphics/TATHighlightStateMgrComponent.h"
#include "Interactables/TATLockConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCupboardDoorComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATCupboardDoorComponent, Log, All);

UTATCupboardDoorComponent::UTATCupboardDoorComponent() :
   UStaticMeshComponent()
{
   SetIsReplicatedByDefault(true);
}

void UTATCupboardDoorComponent::BeginPlay()
{
   Super::BeginPlay();

   ComponentTags.AddUnique(UOSEInteractionHelpers::kInteractNoHighlightTag);
   _lockConfig.RandomizeLockLevel(GetOwner());
}

void UTATCupboardDoorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UTATCupboardDoorComponent, _state);
   DOREPLIFETIME(UTATCupboardDoorComponent, _locked);
   DOREPLIFETIME(UTATCupboardDoorComponent, _lockpickCurrentTrack);
   
}

void UTATCupboardDoorComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
}

bool UTATCupboardDoorComponent::IsInteractable_Implementation(ACharacter* const interactingCharacter) const
{
   return true;
}

FLockInteractContext UTATCupboardDoorComponent::MakeLockContext(ACharacter* interactingCharacter) const
{
   FLockInteractContext ctx;
   ctx.bIsLocked = _canEverLock && _locked;
   ctx.bIsLockRelevant = _canEverLock && _state.bIsOn == false;
   ctx.bAllowsKey = _lockConfig.KeyTag.IsValid();
   ctx.bHasKey = ctx.bIsLockRelevant && _lockConfig.DoesCharacterHaveKey(interactingCharacter);
   ctx.bCanInteractorLockpick = _lockConfig.CanBeLockpicked && FTATLockConfig::CanActorLockpick(interactingCharacter);
   ctx.bCanBePickedInCurrentDirection = ctx.bIsLockRelevant;
   ctx.bIsLockedInCurrentDirection = _locked && ctx.bIsLockRelevant;
   ctx.bCanBeRelockedInCurrentDirection = !_locked && ctx.bIsLockRelevant; 
   ctx.bAreAllSidesLocked = true;
   return ctx;
}

void UTATCupboardDoorComponent::GetInteractPrompt_Implementation(ACharacter* const interactingCharacter, FInteractPrompt& prompt)
{
   const FLockInteractContext lockContext = MakeLockContext(interactingCharacter);
   if (lockContext.bIsLockedInCurrentDirection == false)
   {
      prompt.PressAction = IsOpen() ? _closePrompt : _openPrompt;
   }
   _lockConfig.AddToPrompt(prompt, lockContext, interactingCharacter);
}

FInteractStartResult UTATCupboardDoorComponent::StartInteract_Implementation(ACharacter* const interactingCharacter)
{
   const FLockInteractContext lockContext = MakeLockContext(interactingCharacter);
   FInteractStartResult result;
   if(_lockConfig.TryHandleInteractStart(this, interactingCharacter, lockContext, result))
   {
      return result;
   }
   if(lockContext.bIsLockedInCurrentDirection == false)
   {
      _Toggle();
   }
   return result;
}

bool UTATCupboardDoorComponent::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   const FLockInteractContext lockContext = MakeLockContext(interactingCharacter);
   if (context.IsProbablyInstant() && !lockContext.bIsLockedInCurrentDirection)
   {
      _Toggle();
   }
   else if (context.IsComplete())
   {
      _lockConfig.HandleInteractComplete(this, interactingCharacter, lockContext);
   }
   return false;
}

void UTATCupboardDoorComponent::ShowHighlight_Implementation(const bool showHighlight)
{
   UTATHighlightStateMgrComponent::HighlightComponent(this, showHighlight);
}

void UTATCupboardDoorComponent::OnLockpickTrackCompleted(int32 trackIndex)
{
   if (trackIndex < _lockpickCurrentTrack)
   {
      UE_LOG(LogTATCupboardDoorComponent, Warning, TEXT("OnLockpickTrackCompleted() called for a track that was not active"));
      return;
   }
   else if (IsLocked() == false)
   {
      UE_LOG(LogTATCupboardDoorComponent, Warning, TEXT("OnLockpickTrackCompleted() called on an unlocked actor"));
      return;
   }

   GetOwner()->FlushNetDormancy();
   _lockpickCurrentTrack = trackIndex + 1;
}

void UTATCupboardDoorComponent::SetLocked(bool newIsLocked)
{
   if (newIsLocked == _locked) return;

   GetOwner()->FlushNetDormancy();
   _locked = newIsLocked;
   
   if (GetOwner()->HasAuthority())
   {
      // Reset the tracks completed
      _lockpickCurrentTrack = 0;
   }
}

void UTATCupboardDoorComponent::_OnRep_State(const FOSEToggleState& previousState)
{
   SyncTimelines.Broadcast(_state);

   if (previousState.bIsOn != _state.bIsOn)
   {
      OnDoorToggled.Broadcast(_state.bIsOn);
      // TODO: magic numbers
      if (!_state.IsOld(this, 0.75f))
      {
         OnDoorRecentlyToggled.Broadcast(_state.bIsOn);
      }
   }
}

void UTATCupboardDoorComponent::_Toggle()
{
   GetOwner()->FlushNetDormancy();
   const auto oldState = _state;
   _state.bIsOn = !_state.bIsOn;
   _state.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);

   _onRequestCancelLockpicking.Broadcast();
   _OnRep_State(oldState);
}
