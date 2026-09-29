// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWardZone.h"

// tat
#include "Online/TATGameState.h"
#include "Environment/TATWardZoneSubsystem.h"
#include "AI/Perception/TATAISense_Hearing.h"
#include "Player/TATPlayerController.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Interactables/OSEInteractionHelpers.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWardZone)

DEFINE_LOG_CATEGORY_STATIC(LogTATWardZone, Log, All);

namespace WardZoneHelpers
{
   bool IsValidMapVariationLoadingState(ETATMapVariationLoadingState state)
   {
      return state == ETATMapVariationLoadingState::CompleteNoVariation || state == ETATMapVariationLoadingState::CompleteWithVariation;
   }

   bool CharacterMatchesTagQuery(ACharacter* character, const FGameplayTagQuery& tagQuery)
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(character))
      {
         FGameplayTagContainer tagContainer;
         asc->GetOwnedGameplayTags(tagContainer);
         return tagQuery.Matches(tagContainer);
      }
      return false;
   }

   bool IsLocallyControlledPlayerCharacter(ACharacter* character)
   {
      return character != nullptr && character->GetController() != nullptr && character->GetController()->IsLocalPlayerController();
   }

   bool IsValidGameplayEffectHandle(const FActiveGameplayEffectHandle& handle)
   {
      if (handle.IsValid())
      {
         if (UAbilitySystemComponent* asc = handle.GetOwningAbilitySystemComponent())
         {
            return asc->GetActiveGameplayEffect(handle) != nullptr;
         }
      }
      return false;
   }
}

ATATWardZone::ATATWardZone()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;

   bReplicates = true;
   NetDormancy = DORM_Initial;

   _wardZoneState.IsActive = WardInitiallyActive;
}

void ATATWardZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(ATATWardZone, _wardZoneState);
   DOREPLIFETIME_CONDITION(ATATWardZone, ZoneSize, COND_InitialOnly);
}

void ATATWardZone::BeginPlay()
{
   Super::BeginPlay();

   if (UTATWardZoneSubsystem* wardZoneSubsystem = GetWorld()->GetSubsystem<UTATWardZoneSubsystem>())
   {
      wardZoneSubsystem->RegisterWardZone(this);
   }

   // Set up initial state
   if (_wardZoneState.IsActive != WardInitiallyActive)
   {
      FlushNetDormancy();
      _wardZoneState.IsActive = WardInitiallyActive;
   }

   // Initial active changed event to allow setting up visual state in blueprints
   constexpr bool initialStateChangeIsRecent = false;
   OnWardZoneActiveChanged(_wardZoneState.IsActive, initialStateChangeIsRecent);

   if (HasAuthority())
   {
      _isReadyToProcessActorOverlaps = false;

      bool shouldBindOnWorldBeginPlay = true;
      if (const ATATGameState* gameState = Cast<ATATGameState>(GetWorld()->GetGameState()))
      {
         if (UTATMapVariationMgrComponent* mapVariationMgr = gameState->GetMapVariationMgr())
         {
            const ETATMapVariationLoadingState state = mapVariationMgr->GetCurrentMapVariationLoadingState();
            if (!WardZoneHelpers::IsValidMapVariationLoadingState(state))
            {
               mapVariationMgr->OnMapVariationMgrStateChanged.AddDynamic(this, &ThisClass::_HandleMapStateChanged);
            }
            else
            {
               _HandleMapStateChanged(state);
            }

            shouldBindOnWorldBeginPlay = false;
         }
      }

      if (shouldBindOnWorldBeginPlay)
      {
         _WaitForWorldBeginPlayOrTrigger();
      }
   }
   else
   {
      _HandleInitialOverlaps();
      _isReadyToProcessActorOverlaps = true;
   }
}

void ATATWardZone::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UTATWardZoneSubsystem* wardZoneSubsystem = GetWorld()->GetSubsystem<UTATWardZoneSubsystem>())
   {
      wardZoneSubsystem->UnregisterWardZone(this);
   }

   Super::EndPlay(endPlayReason);
}

void ATATWardZone::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   if (HasAuthority())
   {
      // Remove characters from _authorityWardedEffects if their gameplay effect is no longer valid
      _authorityWardedEffects.RemoveAllSwap([](FTATWardEffectEntry& entry) {
         return !WardZoneHelpers::IsValidGameplayEffectHandle(entry.EffectHandle);
      });

      // Apply the ward to any character inside the zone that became eligible this frame
      for (ACharacter* character : _overlappingCharacters)
      {
         if (!_authorityWardedEffects.Contains(character) && _AuthorityDoesCharacterMatchWardCriteria(character))
         {
            _AuthorityApplyWardToCharacter(character);
         }
      }

      // Stop ticking if we have no characters to track
      if (_overlappingCharacters.Num() == 0 && _authorityWardedEffects.Num() == 0)
      {
         SetActorTickEnabled(false);
      }
   }
}

void ATATWardZone::NotifyActorBeginOverlap(AActor* otherActor)
{
   Super::NotifyActorBeginOverlap(otherActor);
   _HandleActorOverlapBegin(otherActor);
}

void ATATWardZone::NotifyActorEndOverlap(AActor* otherActor)
{
   Super::NotifyActorEndOverlap(otherActor);
   _HandleActorOverlapEnd(otherActor);
}

bool ATATWardZone::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if ((IsWardZoneActive() && !AllowInteractToDeactivate) || (!IsWardZoneActive() && !AllowInteractToActivate))
   {
      return false;
   }
   if (!_localPlayerAllowInteraction && WardZoneHelpers::IsLocallyControlledPlayerCharacter(interactingCharacter))
   {
      return false;
   }
   return true;
}

void ATATWardZone::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   const FTATWardZoneInteractConfig& cfg = IsWardZoneActive() ? WardDeactivatePrompt : WardActivatePrompt;
   if (cfg.IsHold)
   {
      prompt.HoldAction = cfg.Prompt;
      prompt.HoldActionTag = cfg.ActionTag;
   }
   else
   {
      prompt.PressAction = cfg.Prompt;
      prompt.PressActionTag = cfg.ActionTag;
   }
   prompt.InteractStatusTag = cfg.StatusTag;
}

FInteractStartResult ATATWardZone::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   FInteractStartResult result;

   if (IsWardZoneActive() && AllowInteractToDeactivate)
   {
      if (WardDeactivatePrompt.IsHold)
      {
         result.Delay = WardDeactivatePrompt.HoldDuration;
         result.HoldAnimationTag = InteractAnimationTag;
      }
      else
      {
         // instant
         SetWardZoneActive(false);
         _FireActivationToggleGameplayCue(interactingCharacter);
         result.InstantAnimationTag = InteractAnimationTag;
      }
   }
   else if (!IsWardZoneActive() && AllowInteractToActivate)
   {
      if (WardActivatePrompt.IsHold)
      {
         result.Delay = WardActivatePrompt.HoldDuration;
         result.HoldAnimationTag = InteractAnimationTag;
      }
      else
      {
         // instant
         SetWardZoneActive(true);
         _FireActivationToggleGameplayCue(interactingCharacter);
         result.InstantAnimationTag = InteractAnimationTag;
      }
   }

   result.bWaitForDelay = result.Delay > 0;
   return result;
}

bool ATATWardZone::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (HasAuthority() && context.IsComplete() && !context.bWasCanceled)
   {
      const bool isActive = IsWardZoneActive();
      if (isActive && WardDeactivatePrompt.IsHold)
      {
         SetWardZoneActive(false);
         _FireActivationToggleGameplayCue(interactingCharacter);
      }
      else if (!isActive && WardActivatePrompt.IsHold)
      {
         SetWardZoneActive(true);
         _FireActivationToggleGameplayCue(interactingCharacter);
      }
   }
   return true;
}

void ATATWardZone::ShowHighlight_Implementation(bool showHighlight)
{

}

bool ATATWardZone::IsCharacterInZone(ACharacter* character) const
{
   return character != nullptr && _overlappingCharacters.Contains(character);
}

bool ATATWardZone::AuthorityIsCharacterWarded(ACharacter* character) const
{
   if (character == nullptr)
   {
      return false;
   }
   return _authorityWardedEffects.Contains(character);
}

void ATATWardZone::SetWardZoneActive(bool newActive)
{
   if (!HasAuthority() || _wardZoneState.IsActive == newActive)
   {
      return;
   }

   FlushNetDormancy();
   const FTATWardZoneState prevState = _wardZoneState;
   _wardZoneState.IsActive = newActive;
   _wardZoneState.LastChangeTime = GetWorld()->GetTimeSeconds();
   _OnRep_WardZoneState(prevState);

   // Remove all wards now that we're deactivated
   if (!_wardZoneState.IsActive && RemoveWardGameplayEffectOnDeactivate)
   {
      TArray<ACharacter*, TInlineAllocator<8>> wardedCharacters;
      for (const FTATWardEffectEntry& entry : _authorityWardedEffects)
      {
         wardedCharacters.AddUnique(entry.Character.Get());
      }
      for (ACharacter* character : wardedCharacters)
      {
         _AuthorityRemoveWardFromCharacter(character);
      }
   }
}

void ATATWardZone::TickLocalPlayerVisualState(ACharacter* character, bool firstTick, FTATWardZonePlayerVisibilityState& prevState)
{
   check(character != nullptr);

   FTATWardZonePlayerVisibilityState newState{
      .State = ETATWardZoneVisualState::Nearby_ZoneVisible,
      .ZoneActive = _wardZoneState.IsActive,
   };

   // Only evaluate visual state if the zone is active (inactive zones always default to Nearby_ZoneVisible)
   if (newState.ZoneActive)
   {
      // If the player is nearby, determine if the zone should be visible or not
      if (FVector::DistSquared(character->GetActorLocation(), GetActorLocation()) < FMath::Square(VisualStateMaxDistance))
      {
         auto boolToNearbyVisualState = [](bool zoneVisible) -> ETATWardZoneVisualState
         {
            return zoneVisible ? ETATWardZoneVisualState::Nearby_ZoneVisible : ETATWardZoneVisualState::Nearby_ZoneHidden;
         };

         switch (VisualStateNearbyCriteria)
         {
         case ETATWardZoneVisibleCriteria::AlwaysVisible:
            newState.State = ETATWardZoneVisualState::Nearby_ZoneVisible;
            break;
         case ETATWardZoneVisibleCriteria::AlwaysHidden:
            newState.State = ETATWardZoneVisualState::Nearby_ZoneHidden;
            break;
         case ETATWardZoneVisibleCriteria::WardApplicationCriteria:
            newState.State = boolToNearbyVisualState(WardZoneHelpers::CharacterMatchesTagQuery(character, WardApplicationCriteria));
            break;
         case ETATWardZoneVisibleCriteria::WardApplicationCriteriaInverse:
            newState.State = boolToNearbyVisualState(!WardZoneHelpers::CharacterMatchesTagQuery(character, WardApplicationCriteria));
            break;
         case ETATWardZoneVisibleCriteria::CustomTagQuery:
            newState.State = boolToNearbyVisualState(WardZoneHelpers::CharacterMatchesTagQuery(character, VisualStateNearbyVisibleTagQuery));
            break;
         default:
            checkNoEntry();
         }
      }
      else
      {
         // No need to do any tag queries for players that are far away from the ward zone
         newState.State = ETATWardZoneVisualState::FarAway;
      }
   }

   // Bail if the state has not changed
   if (newState == prevState && !firstTick)
   {
      return;
   }

   // Update previous state
   prevState = newState;

   // We know state changed, so call into blueprints to update visuals
   OnLocalPlayerVisualStateChange(character, newState.State, newState.ZoneActive);
}

void ATATWardZone::OnLocalPlayerVisualStateChange_Implementation(ACharacter* character, ETATWardZoneVisualState state, bool isWardZoneActive)
{

}

bool ATATWardZone::_AuthorityDoesCharacterMatchWardCriteria(ACharacter* character) const
{
   check(HasAuthority());

   if (!_wardZoneState.IsActive)
   {
      return false;
   }

   if (character == nullptr)
   {
      return false;
   }

   const bool isPlayerCharacter = character->GetPlayerState() != nullptr;
   if ((!ApplyWardToPlayerCharacters && isPlayerCharacter) || (!ApplyWardToAICharacters && !isPlayerCharacter))
   {
      return false;
   }

   // If the character is already warded, skip them
   if (_authorityWardedEffects.Contains(character))
   {
      return false;
   }

   // Check if the ward was applied to the character recently
   if (WardImmunityDuration > 0)
   {
      if (const float* lastApplicationTime = _authorityWardTriggerTime.Find(character))
      {
         const float currentTime = GetWorld()->GetTimeSeconds();
         if (currentTime - *lastApplicationTime < WardImmunityDuration)
         {
            return false;
         }
      }
   }

   // If we have no gameplay tag-based target criteria, allow all characters
   if (WardApplicationCriteria.IsEmpty() || WardZoneHelpers::CharacterMatchesTagQuery(character, WardApplicationCriteria))
   {
      return true;
   }

   return false;
}

void ATATWardZone::SetWardZoneInteractableForLocalPlayer(bool newInteractable)
{
   if (ATATPlayerController::GetLocalTATPlayerController(this) == nullptr)
   {
      return;
   }
   _localPlayerAllowInteraction = newInteractable;
}

bool ATATWardZone::IsWardZoneInteractableForLocalPlayer() const
{
   if (IsWardZoneActive() && !AllowInteractToDeactivate)
   {
      return false;
   }
   if (!IsWardZoneActive() && !AllowInteractToActivate)
   {
      return false;
   }
   if (ATATPlayerController::GetLocalTATPlayerController(this) != nullptr)
   {
      return _localPlayerAllowInteraction;
   }
   return true;
}

void ATATWardZone::_AuthorityApplyWardToCharacter(ACharacter* character)
{
   check(HasAuthority());
   check(character != nullptr);

   const int32 prevNumWardedEffects = _authorityWardedEffects.Num();

   // Update the last application time for cooldown use
   _authorityWardTriggerTime.FindOrAdd(character) = GetWorld()->GetTimeSeconds();

   const FVector characterLocation = character->GetActorLocation();

   if (WardGameplayEffects.Num())
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(character))
      {
         FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
         if (effectContext.IsValid())
         {
            const FVector wardLocation = GetActorLocation();
            const FVector damageHitNormal = (characterLocation - wardLocation).GetSafeNormal();

            effectContext.AddOrigin(wardLocation);
            effectContext.AddSourceObject(this);
            effectContext.AddHitResult(FHitResult{ character, character->GetCapsuleComponent(), characterLocation, damageHitNormal });
         }

         for (const FTATWardZoneGameplayEffect& effect : WardGameplayEffects)
         {
            FGameplayEffectSpecHandle specHandle = asc->MakeOutgoingSpec(effect.WardGameplayEffect, effect.EffectLevel, effectContext);
            if (specHandle.IsValid())
            {
               for (const FTATWardZoneGameplayEffectParam& param : effect.SetByCallerParams)
               {
                  if (param.Tag.IsValid())
                  {
                     if (FGameplayEffectSpec* spec = specHandle.Data.Get())
                     {
                        spec->SetSetByCallerMagnitude(param.Tag, param.Value);
                     }
                  }
               }

               const FActiveGameplayEffectHandle handle = asc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
               if (handle.IsValid())
               {
                  _authorityWardedEffects.Emplace(character, handle);
               }
            }
         }
      }
   }

   if (WardNoiseStim.IsValid())
   {
      UTATAISense_Hearing::ReportNoiseEvent(this, WardNoiseStim, characterLocation, character);
   }

   // Make sure tick is enabled if we warded a character
   if (prevNumWardedEffects == 0 && _authorityWardedEffects.Num() > 0 && !IsActorTickEnabled())
   {
      SetActorTickEnabled(true);
   }

   AuthorityOnCharacterApplyWard(character);

   _MulticastWardApplied(character);
   ForceNetUpdate();
}

void ATATWardZone::_AuthorityRemoveWardFromCharacter(ACharacter* character)
{
   check(HasAuthority());
   check(character != nullptr);


   TWeakObjectPtr<ACharacter> weakCharacter = character;
   for (int i = _authorityWardedEffects.Num() - 1; i >= 0; --i)
   {
      const FTATWardEffectEntry& entry = _authorityWardedEffects[i];
      if (entry.Character == weakCharacter)
      {
         if (UAbilitySystemComponent* asc = entry.EffectHandle.GetOwningAbilitySystemComponent())
         {
            asc->RemoveActiveGameplayEffect(entry.EffectHandle);
         }
         _authorityWardedEffects.RemoveAtSwap(i);
      }
   }

   AuthorityOnCharacterRemoveWard(character);
}

void ATATWardZone::OnWardZoneActiveChanged_Implementation(bool isActive, bool isRecentChange)
{

}

void ATATWardZone::OnCharacterEnterZone_Implementation(ACharacter* character)
{

}

void ATATWardZone::OnCharacterLeaveZone_Implementation(ACharacter* character)
{

}

void ATATWardZone::_MulticastWardApplied_Implementation(ACharacter* character)
{
   if (GetNetMode() != NM_DedicatedServer)
   {
      OnCharacterApplyWardCosmetic(character);
   }
}

void ATATWardZone::_HandleMapStateChanged(const ETATMapVariationLoadingState currentState)
{
   if (WardZoneHelpers::IsValidMapVariationLoadingState(currentState))
   {
      _WaitForWorldBeginPlayOrTrigger();
   }
}

void ATATWardZone::_WaitForWorldBeginPlayOrTrigger()
{
   UWorld* world = GetWorld();
   if (world == nullptr)
   {
      return;
   }

   if (world->HasBegunPlay())
   {
      _OnWorldBeginPlay();
   }
   else
   {
      world->OnWorldBeginPlay.AddUObject(this, &ThisClass::_OnWorldBeginPlay);
   }
}

void ATATWardZone::_OnWorldBeginPlay()
{
   UWorld* world = GetWorld();
   check(world != nullptr);
   world->OnWorldBeginPlay.RemoveAll(this);
   _HandleInitialOverlaps();
}

void ATATWardZone::_HandleInitialOverlaps()
{
   TArray<AActor*> overlappingActors;
   GetOverlappingActors(overlappingActors, ACharacter::StaticClass());
   _isReadyToProcessActorOverlaps = true;
   for (AActor* overlappingActor : overlappingActors)
   {
      _HandleActorOverlapBegin(overlappingActor);
   }
}

void ATATWardZone::_HandleActorOverlapBegin(AActor* actor)
{
   if (!_isReadyToProcessActorOverlaps)
   {
      return;
   }

   ACharacter* character = Cast<ACharacter>(actor);
   if (character == nullptr || _overlappingCharacters.Contains(character))
   {
      return;
   }

   const int32 prevNumOverlappingCharacters = _overlappingCharacters.Num();

   _overlappingCharacters.Add(character);

   if (HasAuthority() && _AuthorityDoesCharacterMatchWardCriteria(character))
   {
      _AuthorityApplyWardToCharacter(character);
   }

   OnCharacterEnterZone(character);

   // On authority, enable tick if we have characters to monitor
   if (HasAuthority() && prevNumOverlappingCharacters == 0 && _overlappingCharacters.Num() > 0 && !IsActorTickEnabled())
   {
      SetActorTickEnabled(true);
   }
}

void ATATWardZone::_HandleActorOverlapEnd(AActor* actor)
{
   if (!_isReadyToProcessActorOverlaps)
   {
      return;
   }

   ACharacter* character = Cast<ACharacter>(actor);
   if (character == nullptr || !_overlappingCharacters.Contains(character))
   {
      return;
   }

   _overlappingCharacters.Remove(character);

   if (RemoveWardGameplayEffectOnLeaveZone && HasAuthority() && _authorityWardedEffects.Contains(character))
   {
      _AuthorityRemoveWardFromCharacter(character);
   }

   OnCharacterLeaveZone(character);
}

void ATATWardZone::_OnRep_WardZoneState(const FTATWardZoneState& prevState)
{
   if (prevState.IsActive == _wardZoneState.IsActive)
   {
      return;
   }
   const bool isRecentChange = !UOSEInteractionHelpers::IsOld(this, _wardZoneState.LastChangeTime);
   OnWardZoneActiveChanged(_wardZoneState.IsActive, isRecentChange);
}

void ATATWardZone::_FireActivationToggleGameplayCue(ACharacter* character)
{
   check(character != nullptr);

   if (ActivationToggleGameplayCue.IsValid())
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(character))
      {
         // Execute gameplay cue on character, passing ourselves along so we can replicate a call to BroadcastToggled()
         FGameplayCueParameters params;
         params.SourceObject = this;
         asc->ExecuteGameplayCue(ActivationToggleGameplayCue, params);
      }
   }
}
