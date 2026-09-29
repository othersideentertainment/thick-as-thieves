// (c) 2022-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Alertness/TATAlertnessComponent.h"

// tat
#include "AI/Alertness/TATAlertnessAsset.h"
#include "Abilities/TATGameplayTags.h"

// ose
#include "AI/Alertness/OSEAlertnessAsset.h"

// ue
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAlertnessComponent)

UTATAlertnessComponent::UTATAlertnessComponent()
   : Super()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
}

void UTATAlertnessComponent::BeginPlay()
{
   Super::BeginPlay();
   if (GetOwner()->HasAuthority())
   {
      // only tick on the server
      SetComponentTickEnabled(true);
   }
   // cache
   _tatAlertnessSettingsAsset = Cast<UTATAlertnessSettingsAsset>(AlertnessSettingsAsset);
}

void UTATAlertnessComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _decayState, params);
}

void UTATAlertnessComponent::TickComponent(const float deltaTime, const ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   check(GetOwner()->HasAuthority());

   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   //_AuthorityTickAlertnessDecay(deltaTime);
}

float UTATAlertnessComponent::GetSecondsUntilAlertnessDecay() const
{
   float durationRemaining = _decayState.DecayDurationRemaining;
   if (AGameStateBase* gs = GetWorld()->GetGameState())
   {
      // don't interpolate a cooldown unless we're already < the total
      if (durationRemaining < _decayState.TotalDecayDuration)
      {
         const float serverNow = gs->GetServerWorldTimeSeconds();

         // how long has it been since the server updated this state?
         // NOTE:  Keep in-sync with decay logic/multiplier in _AuthorityTickAlertnessDecay so that this can be predictive on clients
         const float timeElapsed = serverNow - _decayState.ServerTimestamp;
         durationRemaining = FMath::Max(0.0f, durationRemaining - (timeElapsed * _decayState.DecayMultiplier));
      }
   }
   return durationRemaining;
}

float UTATAlertnessComponent::GetNormalizedAlertnessDecayValue() const
{
   const float totalDuration = GetTotalSecondsForAlertnessDecay();
   if (totalDuration > 0.0f)
   {
      return GetSecondsUntilAlertnessDecay() / totalDuration;
   }
   return 0.0f;
}

bool UTATAlertnessComponent::IsAlertnessCoolingDown() const
{
   const float durationRemaining = GetSecondsUntilAlertnessDecay();
   return durationRemaining < GetTotalSecondsForAlertnessDecay() && durationRemaining > 0.0f;
}

void UTATAlertnessComponent::_AuthorityTickAlertnessDecay(float deltaTime)
{
   _CheckHasAuthority();

   if (!AlertnessSettingsAsset)
      return;

   const float now = GetWorld()->GetTimeSeconds();
   const EAlertnessLevel alertnessLevel = GetAlertnessLevel();

   // If already neutral, nothing to decay
   // (otherwise it is writing the replicated timestamp every frame) 
   if (alertnessLevel == EAlertnessLevel::Neutral)
   {
      return;
   }

   if (_AuthorityDoesOwnAlertnessDecayForAlernessLevel(GetAlertnessLevel()))
   {
      if (_AuthorityCanDecayAlertnessToNeutral())
      {
         _decayState.DecayMultiplier = _AuthorityGetAlertnessDecayMultiplier();
         const float decayDelta = (deltaTime * _decayState.DecayMultiplier);
         _decayState.DecayDurationRemaining = FMath::Max(0.0f, _decayState.DecayDurationRemaining - decayDelta);
         _decayState.TotalDecayDuration = _AuthorityGetTotalSecondsForAlertnessDecay();
         _decayState.ServerTimestamp = now;

         MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _decayState, this);
      }
      else
      {
         _AuthorityResetAlertLevelDecayTimer();
      }
   }
   else
   {
      // copy decay state from whatever else owns it
      _decayState.DecayMultiplier = _AuthorityGetAlertnessDecayMultiplier();
      _decayState.DecayDurationRemaining = _AuthorityGetSecondsUntilAlertnessDecay();
      _decayState.TotalDecayDuration = _AuthorityGetTotalSecondsForAlertnessDecay();
      _decayState.ServerTimestamp = now;

      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _decayState, this);
   }

   if (_AuthorityGetSecondsUntilAlertnessDecay() <= 0.0f)
   {
      // current spec has us always decaying into a Neutral state, never down through the other states, those are only used on the ramp up
      constexpr EAlertnessLevel targetAlertnessLevel = EAlertnessLevel::Neutral;
      if (targetAlertnessLevel < alertnessLevel)
      {
         _SetAlertnessLevel(GetOwner(), targetAlertnessLevel);
      }
      _AuthorityResetAlertLevelDecayTimer();
   }
}

void UTATAlertnessComponent::_AuthorityResetAlertLevelDecayTimer()
{
   _CheckHasAuthority();

   _decayState.DecayMultiplier = 1.0f;
   _decayState.TotalDecayDuration = _AuthorityGetTotalSecondsForAlertnessDecay();
   _decayState.DecayDurationRemaining = _decayState.TotalDecayDuration;
   _decayState.ServerTimestamp = GetWorld()->GetTimeSeconds();

   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _decayState, this);
}

float UTATAlertnessComponent::_AuthorityGetTotalSecondsForAlertnessDecay() const
{
   _CheckHasAuthority();
   if (AlertnessSettingsAsset)
   {
      const FOSEAlertnessDecaySettings& decaySettings = AlertnessSettingsAsset->DecaySettings;
      return decaySettings.GetSecondsToDecayAlertnessLevel(GetAlertnessLevel());
   }
   return 0.0f;
}

void UTATAlertnessComponent::_OnAlertnessLevelChangeRequested()
{
   _AuthorityResetAlertLevelDecayTimer();
}

void UTATAlertnessComponent::_OnAlertnessLevelChangeRefreshed()
{
   _AuthorityResetAlertLevelDecayTimer();
}

void UTATAlertnessComponent::_AuthorityBroadcastAlertLevelChanged(
   EAlertnessLevel oldAlertnessLevel,
   AActor* instigator) const
{

   FGameplayEventData payload;
   payload.EventTag = TAG_AI_Behavior_AlertnessTransition;
   payload.Instigator = instigator;
   payload.Target = instigator;
   switch(oldAlertnessLevel)
   {
      case EAlertnessLevel::Neutral:
         payload.TargetTags.AddTag(TAG_AI_Alertness_Neutral);
         break;
      case EAlertnessLevel::Suspicious:
         payload.TargetTags.AddTag(TAG_AI_Alertness_Suspicious);
         break;
      case EAlertnessLevel::Alerted:
         payload.TargetTags.AddTag(TAG_AI_Alertness_Alerted);
         break;
      case EAlertnessLevel::Combat:
         payload.TargetTags.AddTag(TAG_AI_Alertness_Combat);
         break;
      default: ;
   }
   UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), payload.EventTag, payload);
   
   Super::_AuthorityBroadcastAlertLevelChanged(oldAlertnessLevel, instigator);
}

void UTATAlertnessComponent::_OnRep_DecayState()
{
   // Everything consuming this is just ticking, may not need an event here...?
}
