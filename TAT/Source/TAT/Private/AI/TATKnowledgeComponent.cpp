// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/TATKnowledgeComponent.h"

// tat
#include "Abilities/TATGameplayTags.h"
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"
#include "AI/Alertness/AlertnessSettings.h"
#include "AI/Detection/TATDetectionSettingsAsset.h"
#include "AI/Perception/TATAIPerceptionComponent.h"
#include "AI/Perception/TATAIPerceptionSystem.h"
#include "AI/Perception/TATAISenseConfig_Hearing.h"
#include "AI/Perception/TATAISense_Sight.h"
#include "AI/Perception/TATHearingTypes.h"
#include "AI/SmartObjects/TATAIIncorrectObjectStateInterface.h"
#include "AI/SmartObjects/TATSmartObjectComponent.h"
#include "AI/StateTrees/TATStateTreeEvents.h"
#include "Character/TATCharacterAIBase.h"
#include "Developer/TATProjectSettings.h"
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Player/TATCharacter.h"
#include "Player/TATPlayerController.h"

// ose
#include "OSECommon.h"
#include "OSEProjectSettings.h"
#include "OSESchedulerTaskSet.h"
#include "OSESchedulerWorldSubsystem.h"
#include "AI/OSEAIController.h"
#include "AI/OSEAIFunctionLibrary.h"
#include "AI/OSEAISettings.h"
#include "AI/Perception/OSEAIVisibilityTargetInterface.h"
#include "AI/Perception/StimInfo.h"
#include "AI/Target/DetectionTargetInterface.h"
#include "Combat/CombatSettings.h"
#include "Detection/OSEDetectionComponent.h"
#include "Traversal/TraversalInterface.h"

// ue4
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATKnowledgeComponent)

DEFINE_LOG_CATEGORY(LogTATKnowledge);

namespace KnowledgeCVars
{
   static int DrawScanForNearbyActors = 0;
   FAutoConsoleVariableRef CVarDebugDrawDrawScanForNearbyActors(
      TEXT("TAT.Perception.DrawScanForNearbyActors"),
      DrawScanForNearbyActors,
      TEXT("Draw debug for scan for nearby actors"),
      ECVF_Default);

   static int DrawLocalPlayerLastKnownLocation = 0;
   FAutoConsoleVariableRef CVarDebugDrawLocalPlayerLastKnownLocation(
      TEXT("TAT.Perception.DrawLocalPlayerLastKnownLocation"),
      DrawLocalPlayerLastKnownLocation,
      TEXT("Draw debug the last known location of the local player"),
      ECVF_Default);
   
   static int DrawObservationDebug = 0;
   FAutoConsoleVariableRef CVarDebugDrawObservationDebug(
      TEXT("TAT.Perception.DrawObservationDebug"),
      DrawObservationDebug,
      TEXT("Draw the debug for observations"),
      ECVF_Default);
   
   static int DrawVisibilityDebug = 0;
   FAutoConsoleVariableRef CVarDebugDrawVisibilityDebug(
      TEXT("TAT.Perception.DrawVisibilityDebug"),
      DrawVisibilityDebug,
      TEXT("Draw the debug for visiblity"),
      ECVF_Default);
}

namespace KnowledgeHelpers
{
   float GetStimulusStrength(const FActorPerceptionInfo* perceptionInfo, const FAISenseID& senseID)
   {
      return perceptionInfo &&
             perceptionInfo->LastSensedStimuli.IsValidIndex(senseID) &&
             perceptionInfo->LastSensedStimuli[senseID].IsValid() &&
             perceptionInfo->LastSensedStimuli[senseID].WasSuccessfullySensed() &&
             !perceptionInfo->LastSensedStimuli[senseID].IsExpired()
             ? perceptionInfo->LastSensedStimuli[senseID].Strength : 0.0f;
   }

   bool IsKnowledgeSourceIndirect(EKnowledgeSource source)
   {
      return source == EKnowledgeSource::Shared || source == EKnowledgeSource::Alert;
   }
}

namespace SharedKnowledgeHelpers
{
   // Returns true if 'a' is considered a higher priority than 'b'
   FORCEINLINE bool IsHigherPriority(const FTATSharedTarget& a, const FTATSharedTarget& b)
   {
      // TODO : consider the shared knowledge type
      // i.e. a suspicious thief might take priority over an object out of place
      return true;
   }
}

// static 
UTATKnowledgeComponent* UTATKnowledgeComponent::TryGet(const AActor* actor)
{
   if (const ATATAIController* controller = Cast<ATATAIController>(UOSECommon::GetController(actor)))
   {
      return controller->GetTATKnowledgeComponent();
   }
   return nullptr;
}

// static
UTATKnowledgeComponent* UTATKnowledgeComponent::TryGetKnowledgeComponent(AActor* actor, EBranchValidity& outValidity)
{
   UTATKnowledgeComponent* knowledge = TryGet(actor);
   outValidity = IsValid(knowledge) ? EBranchValidity::Valid : EBranchValidity::Invalid;
   return knowledge;
}

FTATActorKnowledge::FTATActorKnowledge()
{
   _detectionValue = 0.0f;
}

void FTATActorKnowledge::Init(UTATKnowledgeComponent* owner, TWeakObjectPtr<AActor> actor)
{
   // should exist at init time
   _ownerComponent = owner;
   check(_ownerComponent.IsValid());
   
   // should exist at init time
   _actor = actor;
   check(_actor.Get());

   // can never change; cache at init time
   _isPlayer = UOSECommon::IsAPlayer(_actor.Get());
}

void FTATActorKnowledge::OnAboutToBeRemoved(ATATCharacterAIBase& ownerCharacter)
{
   // resets detection and pushes over an empty detection state to the character
   ResetDetectionState(ownerCharacter);

   // reset visibility and push over an empty vis state to the character
   ResetVisibilityState(ownerCharacter);

   // give owning component a chance for last minute cleanup
   // TODO: I want to get rid of this state entirely but this is here in the meantime...
   _ownerComponent->_OnActorKnowledgeAboutToBeRemoved(*this);
}

void FTATActorKnowledge::TickVisibility(float deltaTime, ATATCharacterAIBase& ownerCharacter, bool isCurrentlySeen, float sightStrength)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_TATKnowledgeComponent_TickVisibility);

   const float now = _GetNowTimestamp();
   const bool wasVisible = _isVisible;

   // update internal visibility state
   _hasAnyDoNotSeeTag = _HasAnyDoNotSeeTag();
   _isVisible = isCurrentlySeen && !_hasAnyDoNotSeeTag;
   _sightStrength = _isVisible ? sightStrength : 0.0f;

   // update known locations based on visibility
   if (const AActor* actor = GetActor())
   {
      if (_isVisible)
      {
         // visible, so update all this state
         SetLastKnownVisibleLocation(actor->GetActorLocation());
         UE_VLOG_LOCATION(&ownerCharacter, LogTATKnowledge, Verbose, actor->GetActorLocation(), 10.f, FColor::Red,
                          TEXT("Last Known Stim Location Set via ticking visibility for %s"), *GetNameSafe(actor));
      }
      else if (!_hasAnyDoNotSeeTag && !KnowledgeHelpers::IsKnowledgeSourceIndirect(_knowledgeSource)) // If player is forcibly not seen at a perception level, also do not update lastKnownLocation
      {
         // If not visible, but was recently visible, magically update lastKnownLocation
         const float timeSinceVisible = now - _lastVisibilityTimestamp;
         const FAlertnessSettings& alertnessSettings = UAlertnessSettingsConfig::GetAlertnessSettings();
         const float noVisibilityLocationSeconds = IsPlayer() ? alertnessSettings.NoVisibilityLocationSecondsPlayers : alertnessSettings.NoVisibilityLocationSecondsAI;
         if (timeSinceVisible <= noVisibilityLocationSeconds)
         {
            // keep the current timestamp, just update the location
            SetLastKnownVisibleLocationWithTimestamp(actor->GetActorLocation(), _lastVisibilityTimestamp);
            UE_VLOG_LOCATION(&ownerCharacter, LogTATKnowledge, Verbose, actor->GetActorLocation(), 10.f, FColor::Red,
                             TEXT("Last Known Stim Location Set via ticking visibility (!_hasAnyDoNotSeeTag) for %s"), *GetNameSafe(actor));
         }
      }
   }

   _TrySendVisibilityStateToOwningCharacter(ownerCharacter, wasVisible, _isVisible);

   if (wasVisible != _isVisible)
   {
      _ownerComponent->OnActorVisibilityChanged.Broadcast(GetActor(), _isVisible);
      _ownerComponent->OnVisibilityKnowledgeChanged.Broadcast(*this);
   }

   _TickVisibleActorInCorrectState();
}

void FTATActorKnowledge::TickDetection(float deltaTime, const FTATDetectionSettings& detectionSettings, ATATCharacterAIBase& ownerCharacter)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_TATKnowledgeComponent_TickDetection);

   // ASSUMPTION: this is ticked after visibility, since we rely on it for detection
   // TODO data-drive more of this logic?
   
   // we're in either the detection state for this alertness level or identified
   const EActorDetectionState prevDetectionState = GetDetectionState();
   
   // calculate current detection value
   const float detectionValueDelta = _CalculateDeltaDetectionValue(deltaTime, detectionSettings, ownerCharacter);
   _detectionValue = FMath::Clamp(_detectionValue + detectionValueDelta, 0.0f, 1.0f);

#if ENABLE_DRAW_DEBUG
   if(KnowledgeCVars::DrawObservationDebug)
   {
      FColor debugColor = FColor::Yellow;
      switch (GetDetectionState())
      {
      case EActorDetectionState::Observing:
         debugColor = FColor::Green;
         break;
      case EActorDetectionState::Identified:
         debugColor = FColor::Red;
         break;
      case EActorDetectionState::Identifying:
         debugColor = FColor::Yellow;
         break;
      }
      DrawDebugLine(
         ownerCharacter.GetWorld(),
         ownerCharacter.GetActorLocation(),
         FMath::Lerp(ownerCharacter.GetActorLocation(), GetLastKnownLocation(), _detectionValue),
         debugColor
      );
   }
#endif
   
   if (prevDetectionState != GetDetectionState())
   {
      _ownerComponent->_OnDetectionStateChanged(*this, prevDetectionState);
      _TrySendDetectionChangedUpdateToTargetActor(ownerCharacter, prevDetectionState);
   }
   _TrySendDetectionStateToRelevantCharacters(ownerCharacter);
}

void FTATActorKnowledge::ResetDetectionState(ATATCharacterAIBase& ownerCharacter)
{
   // back to zero
   _detectionValue = 0.0f;
   _TrySendDetectionStateToRelevantCharacters(ownerCharacter);
}

void FTATActorKnowledge::ResetVisibilityState(ATATCharacterAIBase& ownerCharacter)
{
   const bool wasVisible = _isVisible;
   _isVisible = false;
   _hasAnyDoNotSeeTag = false;
   _sightStrength = 0.0f;
   _TrySendVisibilityStateToOwningCharacter(ownerCharacter, wasVisible, _isVisible);
}

bool FTATActorKnowledge::IsSupiciousAlly() const
{
   if (GetAttitude() == EOSETeamAttitude::Friendly)
   {
      // Me as a character
      if (const ATATCharacterAIBase* myCharacter = _ownerComponent->GetAICharacter())
      {
         // An ally that is a player character
         if (const ATATCharacter* playerCharacter = Cast<ATATCharacter>(GetActor()))
         {
            const UTATProjectSettings& settings = UTATProjectSettings::Get();
            const bool isInDisguise = playerCharacter->HasMatchingGameplayTag(settings.DisguiseActiveTag);
            const bool isBehavingSuspiciously = playerCharacter->IsBehavingSuspiciously(myCharacter->GetAlertnessLevel());
            return (isInDisguise && isBehavingSuspiciously);
         }
      }
   }
   return false;
}

void FTATActorKnowledge::_TickVisibleActorInCorrectState()
{
   // simple case: we don't even check for incorrect states on things we can't see or are not valid
   const AActor* actor = GetActor();
   if (!_isVisible || !IsValid(actor))
   {
      _isVisibleActorInCorrectState = false;
      return;
   }

   if (actor->Implements<UTATAIIncorrectObjectStateInterface>())
   {
      _isVisibleActorInCorrectState = ITATAIIncorrectObjectStateInterface::Execute_AuthorityIsObjectInCorrectState(actor, true);
   }
   else
   {
      _isVisibleActorInCorrectState = false;
   }
}

bool FTATActorKnowledge::_HasAnyDoNotSeeTag() const
{
   if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(GetActor()))
   {
      const UOSEAISettings& settings = UOSEAISettings::Get();
      return tagInterface->HasAnyMatchingGameplayTags(settings.DoNotSeeActorTags);
   }
   return false;
}

bool FTATActorKnowledge::_HasTargetBlockDetectionTags() const
{
   if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(GetActor()))
   {
      const UOSEAISettings& settings = UOSEAISettings::Get();
      return tagInterface->HasAnyMatchingGameplayTags(settings.TargetBlockDetectionTags);
   }
   return false;
}

bool FTATActorKnowledge::_IsCrouching() const
{
   if (const ITraversalInterface* traversalInterface = Cast<ITraversalInterface>(GetActor()))
   {
      return traversalInterface->IsCrouching();
   }
   return false;
}

float FTATActorKnowledge::_GetNowTimestamp() const
{
   check(_ownerComponent.IsValid());
   return _ownerComponent->GetWorld()->GetTimeSeconds();
}

EActorDetectionState FTATActorKnowledge::_CalculateDetectionState(EAlertnessLevel alertnessLevel) const
{
   // leave identified alone, we want to let that naturally time-out
   if (GetDetectionState() == EActorDetectionState::Identified)
      return EActorDetectionState::Identified;

   // otherwise, we want to use whatever the min is for the alertness level
   return _GetMinDetectionStateForAlertnessLevel(alertnessLevel);
}

float FTATActorKnowledge::_CalculateDeltaDetectionValue(float deltaTime, const FTATDetectionSettings& detectionSettings, const ATATCharacterAIBase& ownerCharacter) const
{
   const AActor* actor = GetActor();
   check(actor);

   const EAlertnessLevel alertnessLevel = ownerCharacter.GetAlertnessLevel();
   const float distanceCm = actor->GetDistanceTo(&ownerCharacter);
   const float currentDetectionValue = GetDetectionValue();

   if (_CanActorBeDetected(actor, alertnessLevel))
   {
      const FTATDetectionRampSettings* rampSettings = detectionSettings.RampSettings.Find(alertnessLevel);
      if (!rampSettings)
      {
         UE_LOG(LogTATKnowledge, Error, TEXT("No detection ramp settings for alert level %s!"), *UEnum::GetValueAsString(alertnessLevel));
         return 0.0f;
      }

      const float ownerCharacterHeight = ownerCharacter.GetDefaultHalfHeight() * 2.f;
      if(FMath::IsNearlyZero(ownerCharacterHeight) || ownerCharacterHeight < 0.f)
      {
         UE_LOG(LogTATKnowledge, Error, TEXT("Owning character height is zero or lower for actor %s!"), *actor->GetName());
         return 0.0f;         
      }
      
      const FVector positionDifference = actor->GetActorLocation() - ownerCharacter.GetActorLocation();
      const float heightDiff = FMath::Abs(positionDifference.Z);

      const float heightDiffAsPercentOfOwnerCharacterHeight = heightDiff / ownerCharacterHeight;
      
      const float heightModifier = rampSettings->HeightDifferenceToStrengthMultiplierCurve
                                      ? rampSettings->HeightDifferenceToStrengthMultiplierCurve->GetFloatValue(
                                         heightDiffAsPercentOfOwnerCharacterHeight)
                                      : 1.0f;

      const float weaponModifier = UTATItemFunctionLibrary::DoesActorHaveWeaponEquipped(actor)
                                      ? rampSettings->WeaponEquippedDetectionMultiplier
                                      : 1.0f;

      float crouchModifier = 1.0f;
      if (_IsCrouching() && distanceCm > rampSettings->CrouchSettings.MinimumDistance)
      {
         crouchModifier = rampSettings->CrouchSettings.DetectionMultiplier;
      }

      float targetTagModifierValue = 1.0f;
      if (const IGameplayTagAssetInterface* targetTagInterface = Cast<IGameplayTagAssetInterface>(actor))
      {
         for (const FTATTagDetectionRampModifier& tagModifier : detectionSettings.GameplayTagRampModifiers)
         {
            if (targetTagInterface->HasMatchingGameplayTag(tagModifier.Tag))
            {
               targetTagModifierValue *= tagModifier.Multipliers.Get(alertnessLevel);
            }
         }
      }

      float sourceTagModifierValue = 1.0f;
      if (const IGameplayTagAssetInterface* sourceTagInterface = Cast<IGameplayTagAssetInterface>(&ownerCharacter))
      {
         for (const FTATTagDetectionRampModifier& tagModifier : detectionSettings.SourceGameplayTagRampModifiers)
         {
            if (sourceTagInterface->HasMatchingGameplayTag(tagModifier.Tag))
            {
               sourceTagModifierValue *= tagModifier.Multipliers.Get(alertnessLevel);
            }
         }
      }

      float stealthScoreModifier = 1.f;
      // how long should it take to ramp to the top of this detection level, given our current alertness level?
      // if we are in the critical range it should take N seconds to increase to the top of this detection level
      float secondsRequired = rampSettings->CriticalTimeSeconds;
      const float distancePastCriticalCm = (distanceCm - rampSettings->CriticalRange);
      if (distancePastCriticalCm > 0.0f)
      {
         // add on time for distance beyond that
         const float distancePastCriticalMeters = distancePastCriticalCm / 100.0f;
         secondsRequired += (rampSettings->AgnosticTimeSecondsPerMeter * distancePastCriticalMeters);
         if(const ITATStealthScoreInterface* stealthScoreInterface = Cast<ITATStealthScoreInterface>(actor))
         {
            stealthScoreModifier = 1.f - stealthScoreInterface->GetStealthScore();
         }
      }

      // How much progress we make for this frame, based on how long it should take overall
      const float newDetectionDelta = deltaTime / secondsRequired;
      const float newDetectionDeltaWithModifiers = (newDetectionDelta * weaponModifier * crouchModifier * targetTagModifierValue * sourceTagModifierValue * heightModifier * _sightStrength * stealthScoreModifier);

      // If adding this delta would go outside the 0.0 to 1.0 range, clamp it
      const float clampedNewDetectionDeltaWithModifiers = FMath::Clamp(newDetectionDeltaWithModifiers, -currentDetectionValue, 1.0f - currentDetectionValue);
      return clampedNewDetectionDeltaWithModifiers;
   }
   else if (GetDetectionValue() > 0.0f)
   {
      const FTATDetectionDecaySettings& decaySettings = detectionSettings.DecaySettings;
      const float now = actor->GetWorld()->GetTimeSeconds();

      const float lastVisibilityTimeStamp = GetLastVisibilityTimestamp();
      const float timeSinceVisible = now - lastVisibilityTimeStamp;
      const bool hasVisibilityTimestamp = lastVisibilityTimeStamp >= 0.0f;
      const bool meetsVisibilityRequirements = hasVisibilityTimestamp && timeSinceVisible >= decaySettings.MinSecondsSinceVisibleBeforeDecay;
      const bool meetsDurationRequirements = ((now - GetDetectionStateChangedTimestamp()) > decaySettings.MinSecondsInCurrentDetectionLevelBeforeDecay);

      if (meetsDurationRequirements && meetsVisibilityRequirements)
      {
         const float detectionDecayPerSecond = 1.0f / decaySettings.DetectionDecaySeconds;
         const float distanceModifier = decaySettings.DecayDistanceCurve ? decaySettings.DecayDistanceCurve->GetFloatValue(distanceCm) : 1.0f;
         const float collidingGeometryMultiplier = ownerCharacter.IsCurrentlyOverlappingStaticGeometry() ? decaySettings.DecayMultiplierIfNPCCollidingWithGeometry : 1.f;
         
         const float decayDelta = (detectionDecayPerSecond * deltaTime * distanceModifier * collidingGeometryMultiplier);
         return -decayDelta;
      }
   }
   return 0.0f;
}

void FTATActorKnowledge::_TrySendDetectionStateToRelevantCharacters(ATATCharacterAIBase& ownerCharacter)
{
   UOSEDetectionComponent* ownerCharacterDetectionComponent = ownerCharacter.GetDetectionComponent();
   const float detectionValue = GetDetectionValue();
   AActor* actor = GetActor();
   if (IsPlayer())
   {
      ownerCharacterDetectionComponent->AuthorityUpdatePlayerActorDetectionValue(actor, GetDetectionState(), detectionValue);
   }

   if (IAIDetectionTargetInterface* detectionTargetInterface = Cast<IAIDetectionTargetInterface>(actor))
   {
      detectionTargetInterface->AuthorityOnDetectionValueUpdateForState(&ownerCharacter, GetDetectionState(), detectionValue);
   }
}

void FTATActorKnowledge::_TrySendDetectionChangedUpdateToTargetActor(ATATCharacterAIBase& ownerCharacter, EActorDetectionState previousState) const
{
   AActor* actor = GetActor();
   if (IAIDetectionTargetInterface* detectionTargetInterface = Cast<IAIDetectionTargetInterface>(actor))
   {
      detectionTargetInterface->AuthorityOnDetectionStateChanged(&ownerCharacter, previousState, GetDetectionState());
   }
}

void FTATActorKnowledge::_TrySendVisibilityStateToOwningCharacter(ATATCharacterAIBase& ownerCharacter, const bool wasVisible, const bool isVisible) const
{
   UOSEDetectionComponent* ownerCharacterDetectionComponent = ownerCharacter.GetDetectionComponent();
   // update interfaces / state on the owning character
   if (wasVisible != isVisible)
   {
      if (IOSEAIVisibilityTargetInterface* visInterface = Cast<IOSEAIVisibilityTargetInterface>(GetActor()))
      {
         if (isVisible)
         {
            visInterface->AuthorityOnEnterVisibleByActor(&ownerCharacter);
         }
         else
         {
            visInterface->AuthorityOnExitVisibleByActor(&ownerCharacter);
         }
      }

      if (IsPlayer())
      {
         ownerCharacterDetectionComponent->AuthorityUpdatePlayerActorVisibility(GetActor(), isVisible);
      }
   }
}

void FTATActorKnowledge::_ClearDetectionValueForOtherAlertnessLevel(EAlertnessLevel alertnessLevel)
{
   _detectionValue = 0;
}

EActorDetectionState FTATActorKnowledge::_GetMinDetectionStateForAlertnessLevel(EAlertnessLevel alertnessLevel)
{
   switch (alertnessLevel)
   {
   case EAlertnessLevel::Neutral:
      return EActorDetectionState::Observing;
   case EAlertnessLevel::Suspicious:
   case EAlertnessLevel::Alerted:
   case EAlertnessLevel::Combat:
      return EActorDetectionState::Identifying;
   default:
      unimplemented();
      return EActorDetectionState::Observing;
   }
}

bool FTATActorKnowledge::_CanActorBeDetected(const AActor* actor, EAlertnessLevel alertnessLevel) const
{
   return GetIsVisible() && !_HasTargetBlockDetectionTags();
}

EActorDetectionState FTATActorKnowledge::GetDetectionState() const
{
   if(FMath::IsNearlyZero(_detectionValue))
   {
      return EActorDetectionState::Observing;
   }
   if(_detectionValue < 1.f)
   {
      return EActorDetectionState::Identifying;
   }
   return EActorDetectionState::Identified;
}

void FTATActorKnowledge::SetLastKnownStimLocation(const FVector& lastKnownStimLocation)
{
   SetLastKnownStimLocationWithTimestamp(lastKnownStimLocation, _GetNowTimestamp());
}

void FTATActorKnowledge::SetLastKnownVisibleLocation(const FVector& lastKnownVisibleLocation)
{
   SetLastKnownVisibleLocationWithTimestamp(lastKnownVisibleLocation, _GetNowTimestamp());
}

void FTATActorKnowledge::SetLastKnownStimLocationWithTimestamp(const FVector& lastKnownStimLocation, float timeStamp)
{
   _lastKnownStimLocation = lastKnownStimLocation;
   _lastStimTimestamp = timeStamp;
}

void FTATActorKnowledge::SetLastKnownVisibleLocationWithTimestamp(const FVector& lastKnownVisibleLocation, float timeStamp)
{
   _lastKnownVisibleLocation = lastKnownVisibleLocation;
   _lastVisibilityTimestamp = timeStamp;
}

const FVector& FTATActorKnowledge::GetLastKnownLocation() const
{
   if (_isVisible)
   {
      return _lastKnownVisibleLocation;
   }
   else if (_actor.Get())
   {
      // early-out: if either is invalid use the other
      if (_lastKnownVisibleLocation == FAISystem::InvalidLocation)
         return _lastKnownStimLocation;
      else if (_lastKnownStimLocation == FAISystem::InvalidLocation)
         return _lastKnownVisibleLocation;

      // pick the one w/ the latest timestamp
      if (_lastVisibilityTimestamp > _lastStimTimestamp)
         return _lastKnownVisibleLocation;
      else if (_lastStimTimestamp > _lastVisibilityTimestamp)
         return _lastKnownStimLocation;
      else
      {
         // if the last known vis/stim locations came on the same frame, we can pick the closer one
         const FVector& currentLocation = _actor->GetActorLocation();
         return FVector::DistSquared(currentLocation, _lastKnownStimLocation) < FVector::DistSquared(currentLocation, _lastKnownVisibleLocation)
            ? _lastKnownStimLocation : _lastKnownVisibleLocation;
      }
   }

   // last stim location is probably a good enough fallback?
   return _lastKnownStimLocation;
}

bool FTATActorKnowledge::GetHasAnyPathToLastKnownLocation(const float checkPathToLocationFrequencyInSeconds)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATActorKnowledge_GetHasAnyPathToLastKnownLocation);
   UpdatePathToLastKnownLocation(checkPathToLocationFrequencyInSeconds);
   return _hasAnyPathToLastKnownLocation;
}

bool FTATActorKnowledge::GetHasFullPathToLastKnownLocation(const float checkPathToLocationFrequencyInSeconds)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATActorKnowledge_GetHasFullPathToLastKnownLocation);
   UpdatePathToLastKnownLocation(checkPathToLocationFrequencyInSeconds);
   return _hasFullPathToLastKnownLocation;
}

float FTATActorKnowledge::GetPathLengthToLastKnownLocation(const float checkPathToLocationFrequencyInSeconds)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATActorKnowledge_GetPathLengthToLastKnownLocation);
   UpdatePathToLastKnownLocation(checkPathToLocationFrequencyInSeconds);
   return _pathLengthToLastKnownLocation;
}

void FTATActorKnowledge::UpdatePathToLastKnownLocation(float checkPathToLocationFrequencyInSeconds)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATActorKnowledge_UpdatePathToLastKnownLocation);
   const float now = _GetNowTimestamp();
   if (const AAIController* aiController = Cast<AAIController>(_ownerComponent->GetOwner()))
   {
      const float timeSinceLastCheck = now - GetPathCheckedTimeStamp();
      if (timeSinceLastCheck >= checkPathToLocationFrequencyInSeconds)
      {      
         const FOSEAINavMeshCalcPathResult result = UOSEAIFunctionLibrary::CalcNavMeshPathLengthFromCurrentLocation(aiController->GetPawn(), GetLastKnownLocation());
         SetPathLengthToLastKnownLocation(result.PathLength);
         SetHasAnyPathToLastKnownLocation(result.HasPartialOrFullPath());
         SetHasFullPathToLastKnownLocation(result.HasFullPath());
         SetPathCheckedTimeStamp(now);
      }
   }
}

float FTATActorKnowledge::GetDetectionValue() const
{
   return _detectionValue;
}

void FTATActorKnowledge::SetDetection(const float x)
{
   _detectionValue = x;
}

void FTATActorKnowledge::SetIdentifiedStateDecayPaused(bool paused)
{
   if (paused != _identifiedStateDecayPaused)
   {
      _identifiedStateDecayPaused = paused;
   }
}

void FTATActorKnowledge::UpdateAttitudeToOwner()
{
   check(_ownerComponent.IsValid());
   if (const AController* ownerPC = Cast<AController>(_ownerComponent->GetOwner()))
   {
      _attitude = UOSETeamFunctionLibrary::GetTeamAttitude(ownerPC->GetPawn(), _actor.Get());
   }
}

void FTATLocationKnowledge::Init(const UTATKnowledgeComponent* owner, const UObject* contextObject, uint32 contextHash)
{
   // should exist at init time
   _ownerComponent = owner;
   check(_ownerComponent.IsValid());

   // Context object is non-optional
   _contextObject = contextObject;
   check(_contextObject.IsValid());

   // Context hash is optional, for a sub-identifier within _contextObject.
   _contextHash = contextHash;
}

bool FTATLocationKnowledge::Matches(const UObject* contextObject, uint32 contextHash) const
{
   return (_contextObject == contextObject) && (_contextHash == contextHash);
}

void FTATLocationKnowledge::SetLocation(FVector location)
{
   constexpr float tolerance = 0.1f;
   if (!_location.Equals(location, tolerance))
   {
      _location = location;

      // Reset cached values when location changes.
      _hasAnyPathToLocation = false;
      _hasFullPathToLocation = false;
      _pathLengthToLocation = 0.0f;
      _pathCheckedTimeStamp = 0.0f;
   }
}

float FTATLocationKnowledge::_GetNowTimestamp() const
{
   check(_ownerComponent.IsValid());
   return _ownerComponent->GetWorld()->GetTimeSeconds();
}

bool FTATLocationKnowledge::GetHasAnyPathToLocation(const float checkPathToLocationFrequencyInSeconds)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATLocationKnowledge_GetHasAnyPath);
   UpdatePathToLocation(checkPathToLocationFrequencyInSeconds);
   return _hasAnyPathToLocation;
}

bool FTATLocationKnowledge::GetHasFullPathToLocation(const float checkPathToLocationFrequencyInSeconds)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATLocationKnowledge_GetHasFullPath);
   UpdatePathToLocation(checkPathToLocationFrequencyInSeconds);
   return _hasFullPathToLocation;
}

float FTATLocationKnowledge::GetPathLengthToLocation(const float checkPathToLocationFrequencyInSeconds)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATLocationKnowledge_GetPathLength);
   UpdatePathToLocation(checkPathToLocationFrequencyInSeconds);
   return _pathLengthToLocation;
}

void FTATLocationKnowledge::UpdatePathToLocation(float checkPathToLocationFrequencyInSeconds)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATLocationKnowledge_UpdatePath);
   const float now = _GetNowTimestamp();
   if (const AAIController* aiController = Cast<AAIController>(_ownerComponent->GetOwner()))
   {
      const float timeSinceLastCheck = now - GetPathCheckedTimeStamp();
      if (timeSinceLastCheck >= checkPathToLocationFrequencyInSeconds)
      {
         const FOSEAINavMeshCalcPathResult result = UOSEAIFunctionLibrary::CalcNavMeshPathLengthFromCurrentLocation(aiController->GetPawn(), GetLocation());
         SetPathLengthToLocation(result.PathLength);
         SetHasAnyPathToLocation(result.HasPartialOrFullPath());
         SetHasFullPathToLocation(result.HasFullPath());
         SetPathCheckedTimeStamp(now);
      }
   }
}

void FTATSharedTarget::Reset()
{
   *this = FTATSharedTarget();
}

bool FTATSharedTarget::HasTargetBeenHandled() const
{
   if (State == FTATSharedTarget::EState::None)
   {
      return false;
   }

   if (UTATKnowledgeComponent* knowledgeComp = Instigator.Get())
   {
      // If the stim instigator OR the problematic actor is now invalid as it's been destroyed
      // We _no longer_ need help. So we need to trigger the forgetting behaviour by true.
      if (const FStimInfo* targetStimInfo = knowledgeComp->_FindStimInfo(TargetStimId))
      {
         if (!targetStimInfo->Instigator.IsValid())
         {
            UE_LOG(LogTATKnowledge, Log, TEXT("%s HasTargetBeenHandled, Shared Target has been invalidated for stim %s")
               , *knowledgeComp->GetOwner()->GetName()
               , *targetStimInfo->ToString());
            return true;
         }
      }
      else if (!TargetActor.IsValid())
      {
         UE_LOG(LogTATKnowledge, Log, TEXT("%s HasTargetBeenHandled, Shared Target actor has been invalidated")
            , *knowledgeComp->GetOwner()->GetName());
         return true;
      }
   }

   return false;
}

FString FTATSharedTarget::ToString() const
{
   // TODO : include more information?
   return TypeTag.ToString();
}

UTATKnowledgeComponent::UTATKnowledgeComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
}

namespace TATKnowledgeComponent
{
   static FName SchedulerWorkloadGroupName(TEXT("TATKnowledgeComponent"));
}

void UTATKnowledgeComponent::BeginPlay()
{
   Super::BeginPlay();

   UTATAIPerceptionSystem::RegisterResettableKnowledgeContainer(GetWorld(),this);
   
   const UOSEAISettings& settings = UOSEAISettings::Get();
   _genericStimSettingsDataTable = settings.StimSettingsTable.LoadSynchronous();
   _hearingStimSettingsDataTable = UTATAISettings::GetHearingStimSettings();
   ensure(_hearingStimSettingsDataTable);
   ensure(_genericStimSettingsDataTable);

   _tatAIController = Cast<ATATAIController>(GetOwner());
   if (_tatAIController)
   {
      _tatAIController->OnPossessedPawn.AddUniqueDynamic(this, &UTATKnowledgeComponent::_OnPossessedPawn);
      _tatAIController->OnUnPossessedPawn.AddUniqueDynamic(this, &UTATKnowledgeComponent::_OnUnPossessedPawn);
      if(UOSEIndividualAttitudeComponent* attitudeComponent = _tatAIController->GetAttitudeComponent())
      {
         attitudeComponent->OnIndividualAttitudeChangedForActor.AddUObject(this, &ThisClass::OnIndividualAttitudeChanged);
      }
      _OnPossessedPawn(_tatAIController->GetPawn());
   }

   // used by UConsiderationInput_FakeCyclicalIntelligence
   _fakeCylicalIntelligenceTimeOffset = FMath::FRandRange(MinFakeCylicalIntelligenceTimeOffset, MaxFakeCylicalIntelligenceTimeOffset);

   // Jitter ScanFor... Times to prevent them from clustering as much
   const float now = GetWorld()->GetTimeSeconds();
   _lastNearbyActorsScanTime = now - FMath::FRandRange(0.f, NearbyActorsDetectionFrequencyInSeconds);

   if(UseScheduler)
   {
      UWorld* world = GetWorld();
      UOSESchedulerWorldSubsystem* schedulerWorldSubsystem = world->GetSubsystem<UOSESchedulerWorldSubsystem>();
      PrimaryComponentTick.UnRegisterTickFunction();
   
      schedulerWorldSubsystem->CreateOrAddScheduledTask(
         TATKnowledgeComponent::SchedulerWorkloadGroupName,
         *this,
         [world] { return MakeShareable(new FOSESchedulerTaskSet(TG_PrePhysics, world)); }
      );
   }
}

void UTATKnowledgeComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if(UseScheduler)
   {
      if(UWorld* world = GetWorld())
      {
         UOSESchedulerWorldSubsystem* schedulerWorldSubsystem = world->GetSubsystem<UOSESchedulerWorldSubsystem>();
         schedulerWorldSubsystem->RemoveScheduledTask(TATKnowledgeComponent::SchedulerWorkloadGroupName, *this);
      }
   }
   
   UTATAIPerceptionSystem::UnregisterResettableKnowledgeContainer(GetWorld(),this);
   
   Super::EndPlay(endPlayReason);
}

void UTATKnowledgeComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_TATKnowledgeComponent_Tick);
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATKnowledgeComponent::TickComponent);

   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   // No need to tick if we don't have a perception component or a character.
   if (!_perceptionComponent || !_aiCharacter)
   {
      return;
   }

   const float now = GetWorld()->GetTimeSeconds();

   // all of the actors we have knowledge about
   _TickKnownActors(now, deltaTime);

   // clear out any locations we don't need anymore
   _TickKnownLocations(now, deltaTime);

#if ENABLE_DRAW_DEBUG
   _TickDebugDrawLocalPlayerLastKnownLocation();
#endif // ENABLE_DRAW_DEBUG
}

void UTATKnowledgeComponent::_TickKnownActors(float now, float deltaTime)
{
   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   check(_aiCharacter); // nullchecked by caller; sanity

   if(!ensure(DetectionSettingsAsset)) return;

   // Update detection info for the actors we're tracking, removing any invalid or outdated entries.
#if DO_CHECK
   // Modifying the array can cause the array to resize, meaning the pointers are then looking at invalid data
   // this flag is there to check that we don't accidentally modify that array as a by-product of one of the many
   // event that are fired
   // This guard value class resets the value on deconstruction of the guard value
   TGuardValue<bool> guard(_bCheckNoModificationsToActorArray, true);
#endif
   for (auto iterator = _actors.CreateIterator(); iterator; ++iterator)
   {
      FTATActorKnowledge& actorKnowledge = *iterator;
      const AActor* trackedActor = actorKnowledge.GetActor();
      if (!IsValid(trackedActor))
      {
         // TODO: If this entry was for a player, we need to notify our AI character somehow.
         // Actor is not valid! Delete this entry.
         iterator.RemoveCurrent();
         continue;
      }

      if (_CanForgetActor(actorKnowledge, now))
      {
         actorKnowledge.OnAboutToBeRemoved(*_aiCharacter);
         iterator.RemoveCurrent();
         continue;
      }
      
      const FActorPerceptionInfo* perceivedInfo = _perceptionComponent->GetActorInfo(*trackedActor);// tick visibility
      const FAISenseID sightSenseID = UAISense::GetSenseID<UTATAISense_Sight>();
      bool isCurrentlySeen = perceivedInfo && perceivedInfo->IsSenseActive(sightSenseID);
      
      if(_aiCharacter->IsUnconscious() || _detectionDisabled)
      {
         // force downed characters to not see anything.
         isCurrentlySeen = false;
      }
      
      EOSETeamAttitude newAttitude = UOSETeamFunctionLibrary::GetTeamAttitude(_aiCharacter, trackedActor);
      // If we require a specific tag on the actor for visibility, then check here, only if they are hostile
      if(_requiredTagsForVisibility.IsValid() && newAttitude == EOSETeamAttitude::Hostile)
      {
         // For example, a civilian can only "SEE" an actor if they have the suspicious tag,
         // so we do the initial check then && it against a tag container check.
         if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(trackedActor))
         {
            isCurrentlySeen &= tagInterface->HasAnyMatchingGameplayTags(_requiredTagsForVisibility);
         }
      }
      
      const float sightStrength = _aiCharacter->IsCurrentlyOverlappingStaticGeometry()
                                     ? (isCurrentlySeen ? 1.f : 0)
                                     : KnowledgeHelpers::GetStimulusStrength(perceivedInfo, sightSenseID);
      actorKnowledge.TickVisibility(deltaTime, *_aiCharacter, isCurrentlySeen, sightStrength);
      const bool newVisibility = actorKnowledge.GetIsVisible();

      bool didResetDetectionState = false;
      // defer hostile->ally attitude changes until they are visible, but immediately switch to hostile
      // if that changes. This should be relatively safe as long as detection level remains at 0 if the
      // actor is not perceived to be hostile.
      if (newAttitude != actorKnowledge.GetAttitude() && (newVisibility || newAttitude == EOSETeamAttitude::Hostile))
      {
         actorKnowledge.SetAttitude(newAttitude);
         // Reset detection states since we are changing attitudes.
         actorKnowledge.ResetDetectionState(*_aiCharacter);
         didResetDetectionState = true;
      }

#if ENABLE_DRAW_DEBUG
      if(KnowledgeCVars::DrawVisibilityDebug)
      {
         FColor debugColor = newVisibility ? FColor::Green : FColor::Orange;
         if(didResetDetectionState)
         {
            debugColor = FColor::Red;
         }
         DrawDebugLine(_aiCharacter->GetWorld(), _aiCharacter->GetActorLocation(), actorKnowledge.GetLastKnownLocation(), debugColor);
      }
#endif
            
      // tick actor detection state and value
      actorKnowledge.TickDetection(deltaTime, DetectionSettingsAsset->DetectionSettings, *_aiCharacter);

      if (!perceivedInfo && actorKnowledge.GetKnowledgeSource() == EKnowledgeSource::Sense)
      {
         // We know nothing about this actor, so forget it.
         actorKnowledge.OnAboutToBeRemoved(*_aiCharacter);
         iterator.RemoveCurrent();
         continue;
      }
   }
}

void UTATKnowledgeComponent::_TickNearbyActors(float now, float deltaTime)
{
   if (EnableScanForNearbyActors && now >= _lastNearbyActorsScanTime + NearbyActorsDetectionFrequencyInSeconds)
   {
      _lastNearbyActorsScanTime = now;
      _FindNearbyActors();
   }
}

void UTATKnowledgeComponent::_TickKnownLocations(float now, float deltaTime)
{
   for (auto iterator = _contextLocations.CreateIterator(); iterator; ++iterator)
   {
      const FTATLocationKnowledge& locationKnowledge = *iterator;
      if (_CanForgetLocation(locationKnowledge, now))
      {
         iterator.RemoveCurrent();
         continue;
      }
   }
}

bool UTATKnowledgeComponent::GetLastKnownActorLocation(AActor* actor, FVector& lastKnownLocation) const
{
   const FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor);
   if (actorKnowledge && FAISystem::IsValidLocation(actorKnowledge->GetLastKnownLocation()))
   {
      lastKnownLocation = actorKnowledge->GetLastKnownLocation();
      return true;
   }
   return false;
}

bool UTATKnowledgeComponent::GetTimeSinceActorVisible(AActor* actor, float& timeSinceVisible) const
{
   const FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor);
   if (actorKnowledge && actorKnowledge->GetLastVisibilityTimestamp() >= 0.0f)
   {
      const float now = GetWorld()->GetTimeSeconds();
      timeSinceVisible = now - actorKnowledge->GetLastVisibilityTimestamp();
      return true;
   }
   return false;
}

bool UTATKnowledgeComponent::IsActorVisible(const AActor* actor) const
{
   const FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor);
   return actorKnowledge && actorKnowledge->GetIsVisible();
}

EActorDetectionState UTATKnowledgeComponent::GetActorDetectionState(const AActor* actor) const
{
   if (const FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      return actorKnowledge->GetDetectionState();
   }
   return EActorDetectionState::Observing;
}

void UpdateLastSeenTimestampAndForceIdentified(FTATActorKnowledge& actorKnowledge, const FVector& lastKnownLocation, float lastSeenTimestamp)
{
   // to avoid stomping more recent timestamps with potentially old timestamps we're going to only update if the caller of this knows something we don't (eg has a newer
   // timestamp).  because external squads/guards may have older knowledge of this actor than we do ourselves.
   if (lastSeenTimestamp > actorKnowledge.GetLastVisibilityTimestamp())
   {
      actorKnowledge.SetLastKnownVisibleLocationWithTimestamp(lastKnownLocation, lastSeenTimestamp);
   }
   if (lastSeenTimestamp > actorKnowledge.GetLastStimTimestamp())
   {
      actorKnowledge.SetLastKnownStimLocationWithTimestamp(lastKnownLocation, lastSeenTimestamp);
   }
   actorKnowledge.SetDetection(1.f);
}

void UTATKnowledgeComponent::ForceActorDetectionStateIdentified(const AActor* actor, FVector lastKnownLocation, float lastSeenTimestamp, bool isSharedKnowledge /*= false*/)
{
   if (IsValid(actor) == false)
      return;
   if(lastSeenTimestamp < 0)
   {
      lastSeenTimestamp = GetWorld()->GetTimeSeconds();
   }
   const EKnowledgeSource source = isSharedKnowledge ? EKnowledgeSource::Shared : EKnowledgeSource::Alert;
   FTATActorKnowledge& actorKnowledge = _FindOrAddActorKnowledge(const_cast<AActor*>(actor), source); // TODO: fix const_cast
   actorKnowledge.SetKnowledgeSource(source);
   UpdateLastSeenTimestampAndForceIdentified(actorKnowledge, lastKnownLocation, lastSeenTimestamp);
   UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose, lastKnownLocation, 10.f, FColor::Red,
                    TEXT("Last Known Stim Location Set via FORCED identification for %s"), *GetNameSafe(actor));
}

bool UTATKnowledgeComponent::HasAnyIdentifiedTargetWithAttitude(EOSETeamAttitude attitude) const
{
   for (const FTATActorKnowledge& actorKnowledge : _actors)
   {
      if (actorKnowledge.GetAttitude() == attitude && actorKnowledge.GetDetectionState() == EActorDetectionState::Identified)
         return true;
   }
   return false;
}

float UTATKnowledgeComponent::GetDetectionValue(const AActor* actor) const
{
   if (const FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      return actorKnowledge->GetDetectionValue();
   }
   return -1.0f;
}

void UTATKnowledgeComponent::SetIdentifiedStateDecayPaused(const AActor* actor, bool paused)
{
   if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      return actorKnowledge->SetIdentifiedStateDecayPaused(paused);
   }
}

void UTATKnowledgeComponent::SetStimInvestigationState(int stimId, AActor* investigatingActor, EStimInvestigationState investigationState)
{
   if (FStimInfo* stim = _FindStimInfo(stimId))
   {
      stim->InvestigatingActor = investigatingActor;
      stim->InvestigationState = investigationState;
   }
}

void UTATKnowledgeComponent::SetStimCanBeForgotten(int stimId, bool canBeForgotten)
{
   if (FStimInfo* stim = _FindStimInfo(stimId))
   {
      stim->CanBeForgotten = canBeForgotten;
   }
   else
   {
      UE_LOG(LogTATKnowledge, Log, TEXT("%s SetStimCanBeForgotten called for a stim (%d) that could not be found!")
         , *GetOwner()->GetName()
         , stimId);
   }
}

FStimInfo UTATKnowledgeComponent::GetStimInfo(int stimId)
{
   if (FStimInfo* stim = _FindStimInfo(stimId))
   {
      return *stim;
   }
   return FStimInfo::Invalid;
}

const FStimInfo* UTATKnowledgeComponent::GetStimInfo(int stimId) const
{
   return _FindStimInfo(stimId);
}

void UTATKnowledgeComponent::SetStimInvestigationState(const int stimID, const EStimInvestigationState state)
{
   if (FStimInfo* stim = _FindStimInfo(stimID))
   {
      stim->InvestigationState = state;
   }
}

void UTATKnowledgeComponent::OnIndividualAttitudeChanged(const AActor* actor)
{
   FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor);
   if (actorKnowledge != nullptr)
   {
      actorKnowledge->UpdateAttitudeToOwner();
   }
}

const FTATHearingEventStimSettings* UTATKnowledgeComponent::_GetHearingEventStimSettingsForTag(const FName tag) const
{
   return _hearingStimSettingsDataTable ? _hearingStimSettingsDataTable->FindRow<FTATHearingEventStimSettings>(tag, TEXT("TATKnowledgeComponent")) : nullptr;
}

void UTATKnowledgeComponent::_UpdateVisibilityLogForActor(const FTATActorKnowledge& knowledge, float distance, float distanceMultiplier, float detectionRate) const
{
#if WITH_GAMEPLAY_DEBUGGER
   FVisibilityLog& visibilityLog = knowledge.GetVisibilityLog();
   visibilityLog.DetectionRate = detectionRate;
   visibilityLog.Distance = distance;
   visibilityLog.DistanceMultiplier = distanceMultiplier;
   visibilityLog.Timestamp = GetWorld()->GetTimeSeconds();
#endif
}

TArray<AActor*> UTATKnowledgeComponent::FindKnownEnemyActors() const
{
   TArray<AActor*> actors;
   for(const FTATActorKnowledge& actorKnowledge : _actors)
   {
      if (actorKnowledge.GetAttitude() != EOSETeamAttitude::Hostile)
         continue;
      actors.Add(actorKnowledge.GetActor());
   }
   return actors;
}

void UTATKnowledgeComponent::_ClearKnownActors()
{
   for (auto iterator = _actors.CreateIterator(); iterator; ++iterator)
   {
      FTATActorKnowledge& knowledge = *iterator;
      const AActor* trackedActor = knowledge.GetActor();
      if (IsValid(trackedActor) == false)
      {
         iterator.RemoveCurrent();
      }
      else
      {
         knowledge.OnAboutToBeRemoved(*_aiCharacter);
         iterator.RemoveCurrent();
      }
   }
   _contextLocations.Empty();
}

const FTATActorKnowledge* UTATKnowledgeComponent::GetActorKnowledge(const AActor* actor) const
{
   if (!actor)
   {
      return nullptr;
   }

   const FTATActorKnowledge* knowledge = _actors.FindByPredicate([actor](const FTATActorKnowledge& info)
   {
      const AActor* itActor = info.GetActor();
      return itActor && itActor == actor;
   });

   return knowledge;
}

bool UTATKnowledgeComponent::DoesAnyPathToLastKnownLocationExist(const AActor* actor)
{
   if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      return actorKnowledge->GetHasAnyPathToLastKnownLocation(CheckPathToLocationFrequencyInSeconds);
   }
   return false;
}

bool UTATKnowledgeComponent::DoesAnyPathToSmartObjectSlotLocationExist(const USmartObjectComponent* smartObjectComponent, 
   const FSmartObjectSlotHandle& slotHandle)
{
   FTATLocationKnowledge& locationKnowledge = _FindOrAddSmartObjectSlotLocationKnowledge(smartObjectComponent, slotHandle);
   return locationKnowledge.GetHasAnyPathToLocation(CheckPathToLocationFrequencyInSeconds);
}

bool UTATKnowledgeComponent::DoesFullPathToLastKnownLocationExist(const AActor* actor)
{
   if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      return actorKnowledge->GetHasFullPathToLastKnownLocation(CheckPathToLocationFrequencyInSeconds);
   }
   return false;
}

bool UTATKnowledgeComponent::DoesFullPathToSmartObjectSlotLocationExist(const USmartObjectComponent* smartObjectComponent, 
   const FSmartObjectSlotHandle& slotHandle)
{
   FTATLocationKnowledge& locationKnowledge = _FindOrAddSmartObjectSlotLocationKnowledge(smartObjectComponent, slotHandle);
   return locationKnowledge.GetHasFullPathToLocation(CheckPathToLocationFrequencyInSeconds);
}

float UTATKnowledgeComponent::GetPathLengthToLastKnownLocation(const AActor* actor)
{
   if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      return actorKnowledge->GetPathLengthToLastKnownLocation(CheckPathToLocationFrequencyInSeconds);
   }
   return INDEX_NONE;
}

float UTATKnowledgeComponent::GetPathLengthToSmartObjectSlotLocation(const USmartObjectComponent* smartObjectComponent,
   const FSmartObjectSlotHandle& slotHandle)
{
   FTATLocationKnowledge& locationKnowledge = _FindOrAddSmartObjectSlotLocationKnowledge(smartObjectComponent, slotHandle);
   return locationKnowledge.GetPathLengthToLocation(CheckPathToLocationFrequencyInSeconds);
}

bool UTATKnowledgeComponent::IsVisibleActorInCorrectState(const AActor* actor) const
{
   if (const FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      return actorKnowledge->GetIsVisibleActorInCorrectState();
   }
   // if we know nothing about it, we shouldn't stay it's in an incorrect state
   return true;
}

void UTATKnowledgeComponent::SetIsInvestigatingActor(const AActor* actor, bool isInvestigating)
{
   if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      actorKnowledge->SetIsInvestigatingActor(isInvestigating);
   }
}

const TArray<FVector>& UTATKnowledgeComponent::GetInvestigationLocations(const AActor* actor) const
{
   if (const FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      return actorKnowledge->GetInvestigationLocations();
   }

   static TArray<FVector> sEmptyVec;
   return sEmptyVec;
}

void UTATKnowledgeComponent::AddInvestigationLocation(const AActor* actor, const FVector& location)
{
   if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      actorKnowledge->AddInvestigationLocation(location);
   }
}

void UTATKnowledgeComponent::ClearInvestigationLocations(const AActor* actor)
{
   if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      actorKnowledge->ClearInvestigationLocations();
   }
}

FTATActorKnowledge& UTATKnowledgeComponent::_FindOrAddActorKnowledge(AActor* newActor, EKnowledgeSource knowledgeSource /*= EKnowledgeSource::Sense*/)
{
   check(newActor);

   FTATActorKnowledge* actorKnowledge = GetActorKnowledge(newActor);
   if (!actorKnowledge)
   {
#if DO_CHECK
      check(_bCheckNoModificationsToActorArray == false);
#endif
      actorKnowledge = &_actors.Emplace_GetRef(FTATActorKnowledge());

      actorKnowledge->Init(this, newActor);
      actorKnowledge->UpdateAttitudeToOwner();
      actorKnowledge->SetKnowledgeSource(knowledgeSource);
      OnActorKnowledgeAboutToBeAdded.Broadcast(*actorKnowledge);
   }
   check(actorKnowledge);
   return *actorKnowledge;
}

FStimInfo& UTATKnowledgeComponent::_FindOrAddStimInfo(const FVector& location,
                                                      AActor* instigator,
                                                      const EStimType stimType,
                                                      const EStimSeverity stimSeverity,
                                                      const FName tag,
                                                      const float strength,
                                                      const int32 globalId,
                                                      const FStimDatabaseQuery& query) const
{
   check(IsValid(_aiCharacter));

   UOSEStimDatabase& db = _GetStimDatabase();

   if (FStimInfo* stimInfo = db.FindStimInfo(query))
   {
      UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose, location, 10.f, FColor::Red, TEXT("Detected known stim: %s"), *stimInfo->ToString());
      db.AddStimPerceivedActor(*stimInfo, _aiCharacter);
      // update the timestamp so we don't expire too soon.
      stimInfo->Timestamp = GetWorld()->GetTimeSeconds();
      return *stimInfo;
   }
   
   FStimInfo& newStim = db.AddStimInfo(_aiCharacter, location, instigator, stimType, tag, stimSeverity, strength, globalId);
   UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose, location, 10.f, FColor::Red, TEXT("Detected unknown stim: %s"), *newStim.ToString());
   return newStim;
}

const FStimInfo* UTATKnowledgeComponent::_FindStimInfo(int stimId) const
{
   UOSEStimDatabase& db = _GetStimDatabase();
   return db.FindStimInfo(stimId);
}

bool UTATKnowledgeComponent::RememberTargetToShare(const FTATSharedTarget& sharedTarget)
{
   if (!sharedTarget.IsValid())
   {
      return false;
   }

   const bool hasPreviousShareTarget = _targetToShare.IsValid();
   if (!hasPreviousShareTarget || SharedKnowledgeHelpers::IsHigherPriority(sharedTarget, _targetToShare))
   {
      // Clear any necessary states from the previous share target, if we had one.
      if (hasPreviousShareTarget)
      {
         ForgetTargetToShare();
      }

      _targetToShare = sharedTarget;
      _targetToShare.Instigator = this;
      _targetToShare.State = FTATSharedTarget::EState::PartnerNeeded;
      _GatherSharableTargetContextTags(sharedTarget, _targetToShare.ContextTags);

      const bool hasCurrentStimTarget = (_targetToShare.TargetStimId != INDEX_NONE);
      if (hasCurrentStimTarget)
      {
         SetStimCanBeForgotten(_targetToShare.TargetStimId, false);
      }

      return true;
   }
   else
   {
      UE_LOG(LogTATKnowledge, Warning, TEXT("%s | RememberTargetToShare passed shared target (%s) that was lower priority than the existing tracked (%s). Throwing away...")
         , *GetOwner()->GetName()
         , *sharedTarget.ToString()
         , *_targetToShare.ToString());
      return false;
   }
}

void UTATKnowledgeComponent::ForgetTargetToShare()
{
   if (_targetToShare.TargetStimId != INDEX_NONE)
   {
      SetStimCanBeForgotten(_targetToShare.TargetStimId, true);
   }

   // Clear any bindings we had on our partner (if any)
   // before forgetting about them.
   if (_targetToShare.State == FTATSharedTarget::EState::PartnerFound)
   {
      TrySetPartnerForSharedTarget(nullptr);
   }

   _targetToShare.Reset();
}

void UTATKnowledgeComponent::ForgetTargetShardWithMe()
{
   _targetSharedWithMe.Reset();
}

bool UTATKnowledgeComponent::IsActorValidSharePartner(AActor* actor) const
{
   // We can't report a target to an unconscious partner (duh).
   const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(actor);
   if (tagInterface == nullptr)
   {
      return false;
   }
   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   if (tagInterface->HasMatchingGameplayTag(settings.ConditionUnconsciousTag))
   {
      return false;
   }

   // If the potential partner is at Alerted or Combat alertness levels, they're already dealing with something.
   const IOSEAlertnessInterface* actorAlertness = Cast<const IOSEAlertnessInterface>(actor);
   if (actorAlertness == nullptr)
   {
      return false;
   }
   const EAlertnessLevel alertness = actorAlertness->GetAlertnessLevel();
   if (alertness >= EAlertnessLevel::Alerted)
   {
      return false;
   }

   // All AI should probably have private space components, but if not treat them as if they're not allowed in any
   // in which case the querier shouldn't have to worry about navigating to them in a private space.
   UTATPrivateSpaceCharacterComponent* querierPrivateSpaceComponent = UTATPrivateSpaceCharacterComponent::TryGet(GetOwner());
   if (UTATPrivateSpaceCharacterComponent* partnerPrivateSpaceComponent = UTATPrivateSpaceCharacterComponent::TryGet(actor))
   {
      // ASSUMPTION : if an actor is allowed in a private zone, that is where they are primarily meant to be.
      // The only current way that AI are marked as being allowed to be in a private zone is if an actor spawns within one.
      // Given that AI spawn near where they are intended to patrol/perform Neutral behaviors, this should be a safe assumption (for now).

      // We don't want to allow an actor to navigate to another AI if they are not allowed in the private zone the other is mean to be within.
      const FGameplayTagContainer& actorAllowedPrivateZones = partnerPrivateSpaceComponent->AuthorityGetAllAllowedPrivateZone();
      if (querierPrivateSpaceComponent == nullptr && actorAllowedPrivateZones.Num() > 0)
      {
         return false;
      }
      for (const FGameplayTag& actorAllowedPrivateZoneTag : actorAllowedPrivateZones)
      {
         if (!querierPrivateSpaceComponent->AuthorityIsAllowedInPrivateZone(actorAllowedPrivateZoneTag))
         {
            return false;
         }
      }
   }

   return true;
}

bool UTATKnowledgeComponent::TrySetPartnerForSharedTarget(ATATCharacterAIBase* newPartner)
{
   if (!_targetToShare.IsValid())
   {
      UE_LOG(LogTATKnowledge, Error, TEXT("%s | TrySetPartnerForSharedTarget called with no cached target to share!")
         , *GetOwner()->GetName());
      return false;
   }

   const FTATSharedTarget::EState state = _targetToShare.State;
   if (newPartner != nullptr)
   {
      const bool canAcceptNewPartner = (state == FTATSharedTarget::EState::PartnerNeeded);
      if (!canAcceptNewPartner)
      {
         UE_LOG(LogTATKnowledge, Error, TEXT("%s | TrySetPartnerForSharedTarget called when the cached target to share did not need a partner!")
            , *GetOwner()->GetName());
         return false;
      }

      if (!IsActorValidSharePartner(newPartner))
      {
         UE_LOG(LogTATKnowledge, Error, TEXT("%s | TrySetPartnerForSharedTarget called with an invalid partner!")
            , *GetOwner()->GetName());
         return false;
      }

      // Bind to partner being destroyed.
      newPartner->OnDestroyed.AddDynamic(this, &UTATKnowledgeComponent::_OnSharePartnerDestroyed);

      // Bind to partner's unconscious state.
      {
         UAbilitySystemComponent* asc = newPartner->GetAbilitySystemComponent();
         check(asc != nullptr);

         const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
         _onSharePartnerUnconsciousDelegateHandle = 
            asc->RegisterGameplayTagEvent(settings.ConditionUnconsciousTag).AddUObject(this, &UTATKnowledgeComponent::_OnIsUnconsciousTagChanged);
      }

      // Bind to partner's alertness state.
      {
         const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(newPartner);
         check(alertnessInterface != nullptr);

         UOSEAlertnessComponent* alertnessComponent = alertnessInterface->GetAlertnessComponent();
         check(alertnessComponent != nullptr);

         alertnessComponent->AuthorityOnAlertnessLevelChanged.AddUniqueDynamic(this, &UTATKnowledgeComponent::_OnSharePartnerAlertnessChanged);
      }

      _targetToShare.Partner = UTATKnowledgeComponent::TryGet(newPartner->GetController());
      ensure(_targetToShare.Partner.IsValid());
      _targetToShare.State = FTATSharedTarget::EState::PartnerFound;
   }
   else
   {
      const bool canReleaseCurrentPartner = (state == FTATSharedTarget::EState::PartnerFound);
      if (!canReleaseCurrentPartner)
      {
         UE_LOG(LogTATKnowledge, Error, TEXT("%s | TrySetPartnerForSharedTarget called with nullptr when a partner was not previously found!")
            , *GetOwner()->GetName());
         return false;
      }

      UTATKnowledgeComponent* oldPartnerKnowledge = _targetToShare.Partner.Get();
      check(oldPartnerKnowledge != nullptr);

      ATATCharacterAIBase* oldPartner = oldPartnerKnowledge->GetAICharacter();
      check(oldPartner != nullptr);

      // Unbind from partner being destroyed.
      oldPartner->OnDestroyed.RemoveDynamic(this, &UTATKnowledgeComponent::_OnSharePartnerDestroyed);

      // Unbind from partner's unconscious state
      {
         UAbilitySystemComponent* asc = oldPartner->GetAbilitySystemComponent();
         check(asc != nullptr);

         const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
         asc->RegisterGameplayTagEvent(settings.ConditionUnconsciousTag).Remove(_onSharePartnerUnconsciousDelegateHandle);
         _onSharePartnerUnconsciousDelegateHandle.Reset();
      }

      // Unbind from partner's alertness state
      {
         const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(oldPartner);
         check(alertnessInterface != nullptr);

         UOSEAlertnessComponent* alertnessComponent = alertnessInterface->GetAlertnessComponent();
         check(alertnessComponent != nullptr);

         alertnessComponent->AuthorityOnAlertnessLevelChanged.RemoveDynamic(this, &UTATKnowledgeComponent::_OnSharePartnerAlertnessChanged);
      }

      _targetToShare.Partner = nullptr;
      _targetToShare.State = FTATSharedTarget::EState::PartnerNeeded;

      _OnSharePartnerLost();
   }

   // Once TrySetPartnerForSharedTarget has been called, the first search for a partner has occured.
   // We don't want to use magic knowledge after the first search, so disable it.
   _targetToShare.CanPartnerSearchUseMagicKnowledge = false;

   return true;
}

bool UTATKnowledgeComponent::ShareRememberedTarget(UTATKnowledgeComponent* otherKnowledge)
{
   if (!ensureMsgf(HasTargetToShare(), TEXT("[%s] ShareRememberedTarget called but %s didn't have any target to share!"),
      *GetName(), *GetOwner()->GetName()))
   {
      return false;
   }

   if (otherKnowledge == nullptr)
   {
      UE_LOG(LogTATKnowledge, Error, TEXT("%s | ShareRememberedTarget called with a null recipient.")
         , *GetOwner()->GetName());
      return false;
   }

   // Set these just before sending the knowledge to the partner to ensure they receive the variable changes
   // If the share is unsuccessful, our shared target will be cleared anyways
   _targetToShare.State = FTATSharedTarget::EState::PartnerFound;
   _targetToShare.Partner = otherKnowledge;

   const bool shareReceived = otherKnowledge->ReceiveSharedTarget(_targetToShare);
   ForgetTargetToShare();
   return shareReceived;
}

bool UTATKnowledgeComponent::HasSharedTargetBeenHandled() const
{
   return _targetToShare.HasTargetBeenHandled();
}

void UTATKnowledgeComponent::_GatherSharableTargetContextTags(const FTATSharedTarget& sharedTarget, FGameplayTagContainer contextTags) const
{
   if (const AActor* targetActor = sharedTarget.TargetActor.Get())
   {
      if (const ITATAreaMarkupInterface* markupInterface = Cast<ITATAreaMarkupInterface>(targetActor))
      {
         contextTags.AppendTags(markupInterface->GetAreaMarkupTags());
      }

      if (const IGameplayTagAssetInterface* tagInterface = Cast< IGameplayTagAssetInterface>(targetActor))
      {
         FGameplayTagContainer actorTags;
         tagInterface->GetOwnedGameplayTags(actorTags);
         contextTags.AppendMatchingTags(actorTags, ProblematicActorTagsToRemember); // TODO : ProblematicActorTagsToRemember should be renamed
      }
   }
   // TODO : any tags for stims? do we care about inside/outside?
}

bool UTATKnowledgeComponent::ReceiveSharedTarget(FTATSharedTarget& sharedTarget)
{
   // Shared target has to be shared by someone at the time it is received
   checkf(sharedTarget.Instigator.IsValid(), TEXT("FTATSharedTarget::Instigator, which specifies who is sharing with us this information, must be valid!"));

   if (!SharedKnowledgeHelpers::IsHigherPriority(sharedTarget, _targetSharedWithMe))
   {
      // We already have shared target that is a higher priority than
      // that which was just shared with us.
      UE_LOG(LogTATKnowledge, Error, TEXT("%s | %s attempted to share %s knowledge when I had higher priority shared target (%s)")
         , *GetOwner()->GetName()
         , *sharedTarget.Instigator->GetName()
         , *sharedTarget.ToString()
         , *_targetSharedWithMe.ToString());
      return false;
   }

   if (AActor* targetActor = sharedTarget.TargetActor.Get())
   {
      // AI should never need help with a target that it doesn't have any last location for.
      // TODO: But apparently it can happen due to knowledge loss nowadays, this will likely result in buggy behavior
      FVector lastKnownLocation;
      if (sharedTarget.Instigator->GetLastKnownActorLocation(targetActor, lastKnownLocation))
      {
         UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose, lastKnownLocation, 10.f, FColor::Red,
            TEXT("Sharing location of %s"), *GetNameSafe(targetActor));
         UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose,
            targetActor ? targetActor->GetActorLocation() : FVector::ZeroVector, 10.f, FColor::Red,
            TEXT("Current location of %s"), *GetNameSafe(targetActor));

         _targetSharedWithMe = sharedTarget;
         OnSharedKnowledgeReceived(_targetSharedWithMe);
         return true;
      }
   }
   else if (const FStimInfo* targetStimInfo = sharedTarget.Instigator->_FindStimInfo(sharedTarget.TargetStimId))
   {
      _targetSharedWithMe = sharedTarget;
      OnSharedKnowledgeReceived(_targetSharedWithMe);
      return true;
   }
   else if (sharedTarget.TargetLocation.IsSet())
   {
      // There's no actor to identify or stim to add, just cache the shared target.
      _targetSharedWithMe = sharedTarget;
      OnSharedKnowledgeReceived(_targetSharedWithMe);
      return true;
   }

   return false;
}

void UTATKnowledgeComponent::_ReevaluateSharePartner()
{
   // We should definitely have a target to share if we're being asked to reevaluate
   // the partner we selected to share the target with.
   ensure(HasTargetToShare());

   const FTATSharedTarget& target = GetTargetToShare();
   ensure(target.State == FTATSharedTarget::EState::PartnerFound);

   UTATKnowledgeComponent* partner = target.Partner.Get();
   if (partner == nullptr)
   {
      // Clear the lost partner.
      TrySetPartnerForSharedTarget(nullptr);
   }
   else if (!IsActorValidSharePartner(partner->GetAICharacter()))
   {
      // Clear the no-longer-valid partner.
      TrySetPartnerForSharedTarget(nullptr);
   }
}

void UTATKnowledgeComponent::_OnSharePartnerLost()
{
   const ATATAIController* controller = GetTATAIController();
   if (!ensureMsgf(controller != nullptr, TEXT("_OnSharePartnerLost unable to retrieve controller!")))
   {
      return;
   }
   
   UStateTreeComponent* stateTreeComponent = Cast<UStateTreeComponent>(controller->GetBrainComponent());
   if (!ensureMsgf(stateTreeComponent != nullptr, TEXT("_OnSharePartnerLost unable to find state tree component for %s!"),
      *controller->GetName()))
   {
      return;
   }

   FStateTreeEvent event;
   event.Tag = TAG_StateTreeEvent_SharePartnerLost;
   stateTreeComponent->SendStateTreeEvent(event);
}

void UTATKnowledgeComponent::_OnSharePartnerDestroyed(AActor* actor)
{
   _ReevaluateSharePartner();
}

void UTATKnowledgeComponent::_OnIsUnconsciousTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   // Partner should have never been selected if previously was unconscious.
   // So this event shouldn't have been bound
   if(newTagCount > 0)
   {
      _ReevaluateSharePartner();
   }
}

void UTATKnowledgeComponent::_OnSharePartnerAlertnessChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel, AActor* alertnessIncreaseInstigator)
{
   _ReevaluateSharePartner();
}

#if WITH_GAMEPLAY_DEBUGGER
const FVisibilityLog* UTATKnowledgeComponent::GetVisibilityLogForActor(const AActor* actor) const
{
   if (const FTATActorKnowledge* knowledge = GetActorKnowledge(actor))
   {
      return &knowledge->GetVisibilityLog();
   }

   return nullptr;
}
#endif

void UTATKnowledgeComponent::ResetKnowledgeOfActor(AActor* actor)
{
   // character has just respawned, reset the knowledge of them.
   // guards will "psychically know" this. But as the actors are currently reused this is the cleanest solution
   // to deal with respawning.
   if (FTATActorKnowledge* knowledge = GetActorKnowledge(actor))
   {
      knowledge->OnAboutToBeRemoved(*_aiCharacter);
      _actors.RemoveAll([actor](const FTATActorKnowledge& info)
      {
         const AActor* itActor = info.GetActor();
         return itActor && itActor == actor;
      });
   }
}

void UTATKnowledgeComponent::_FindNearbyActors()
{
   if (!_aiCharacter)
      return;

   FCollisionQueryParams queryParams(SCENE_QUERY_STAT(TATKnowledgeComponent_FindNearbyActors));
   queryParams.AddIgnoredActor(_aiCharacter);

   const FCollisionShape sphere = FCollisionShape::MakeSphere(NearbyActorsSphereRadius);
   constexpr ECollisionChannel collisionChannel = ECC_Pawn;

   TArray<FOverlapResult> overlaps;
   if (GetWorld()->OverlapMultiByObjectType(overlaps, _aiCharacter->GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(collisionChannel), sphere, queryParams))
   {
      for (const FOverlapResult& overlap : overlaps)
      {
         AActor* actor = overlap.GetActor();
         if (!actor)
            continue;

         const EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(_aiCharacter, actor);
         bool addKnowledge = false;
         EKnowledgeSource knowledgeSource = EKnowledgeSource::AllyScan;
         switch(attitude)
         {
         case EOSETeamAttitude::Friendly:
            {
               addKnowledge = NearbyActorsDetectAllies;
               knowledgeSource = EKnowledgeSource::AllyScan;
            }
            break;
         case EOSETeamAttitude::Hostile:
            {
               addKnowledge = NearbyActorsDetectEnemies;
               knowledgeSource = EKnowledgeSource::EnemyScan;
            }
            break;
         case EOSETeamAttitude::Neutral:
            {
               addKnowledge = NearbyActorsDetectNeutrals;
               knowledgeSource = EKnowledgeSource::NeutralScan;
            }
            break;
         }
         
         if (addKnowledge)
         {
            FVector stimLocation = actor->GetActorLocation();
            FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor);

            // update the actor knowledge if this is new knowledge, or knowledge from the same source
            const bool updateKnowledge = !actorKnowledge || actorKnowledge->GetKnowledgeSource() == knowledgeSource;
            if (!actorKnowledge)
            {  
               actorKnowledge = &_FindOrAddActorKnowledge(actor, knowledgeSource);
            }
            check(actorKnowledge);

            if (updateKnowledge)
            {
               actorKnowledge->SetLastKnownStimLocation(stimLocation); 
               UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose, stimLocation, 10.f, FColor::Red,
                                TEXT("Last Known Stim Location Set via Overlap for %s"), *GetNameSafe(actor));
            }
         }
      }
   }

#if ENABLE_DRAW_DEBUG
   if (KnowledgeCVars::DrawScanForNearbyActors)
   {
      DrawDebugSphere(GetWorld(), _aiCharacter->GetActorLocation(), NearbyActorsSphereRadius, 32, FColor::Blue, false, NearbyActorsDetectionFrequencyInSeconds);
   }
#endif
}

void UTATKnowledgeComponent::_TargetSightPerceptionUpdated(AActor* actor, FAIStimulus stimulus)
{
   if (!stimulus.WasSuccessfullySensed() || !actor)
   {
      return;
   }

   if(actor == _aiCharacter)
   {
      checkf(true, TEXT("AI has seen itself and is trying to register witnessing itself"));
      // The AI character should never see themselves.
      return;
   }
   
#if ENABLE_DRAW_DEBUG
   if(KnowledgeCVars::DrawVisibilityDebug)
   {
      UE_LOG(LogTemp, Error, TEXT("Sight Perception"))
      DrawDebugSphere(actor->GetWorld(), actor->GetActorLocation(), 15.f, 6, FColor::Blue, false, 5.f);
   }
#endif
   // Start tracking this actor if we're not already doing so.
   if (!GetActorKnowledge(actor))
   {
      FTATActorKnowledge* actorKnowledge = &_FindOrAddActorKnowledge(actor);
      if(KnowledgeHelpers::IsKnowledgeSourceIndirect(actorKnowledge->GetKnowledgeSource()))
      {
         // If we currently only have knowledge from a shared or alert source, and now our senses are telling us about the actor
         // we should set the knowledge source to the senses
         actorKnowledge->SetKnowledgeSource(EKnowledgeSource::Sense);
      }

      // consider this a stim instead of visibility location because we have vis every frame already
      actorKnowledge->SetLastKnownStimLocation(actor->GetActorLocation());
      UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose, actor->GetActorLocation(), 10.f, FColor::Red,
                       TEXT("Last Known Stim Location Set via sight perception updated %s"), *GetNameSafe(actor));
   }
}

void UTATKnowledgeComponent::_OnVisualStimEvent(AActor* actor, FAIStimulus stimulus)
{
   // TODO: What do we do with this information? A visual event requires that we already see the target.
   // This just gives us metadata about some action that visually happened on that actor. Such as:
   // Becoming downed, going cloaked, being hit(?), etc.

   // So we already know who this is happening to. The question is, what do we do with this information?

   if (stimulus.WasSuccessfullySensed() && actor)
   {
   }
}

void UTATKnowledgeComponent::_OnTATHearingEvent(AActor* actor, FAIStimulus stimulus)
{
   // We don't need to trigger events for any expired stim.
   if(stimulus.IsExpired())
   {
      return;
   }
   
   // Pull out and remove the global Id, otherwise it'd interfere with
   // the stim settings lookup which uses the Tag.
   const int32 globalId = stimulus.Tag.GetNumber();
   stimulus.Tag.SetNumber(0);

   const FTATHearingEventStimSettings* stimSettings = _GetHearingEventStimSettingsForTag(stimulus.Tag);
   const EStimSeverity stimSeverity = stimSettings ? stimSettings->Severity : EStimSeverity::None;

   FStimDatabaseQuery query;
   if(stimSettings)
   {
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(stimSettings->QueryBitmask), EStimDatabaseQueryBitmaskValues::CheckLocation))
      {
         query.RequireLocation(stimulus.StimulusLocation);
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(stimSettings->QueryBitmask), EStimDatabaseQueryBitmaskValues::CheckInstigator))
      {
         query.RequireInstigator(actor);
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(stimSettings->QueryBitmask), EStimDatabaseQueryBitmaskValues::CheckStimType))
      {
         query.RequireStimType(EStimType::Audio);
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(stimSettings->QueryBitmask), EStimDatabaseQueryBitmaskValues::CheckSeverity))
      {
         query.RequireSeverity(stimSeverity);
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(stimSettings->QueryBitmask), EStimDatabaseQueryBitmaskValues::CheckTag))
      {
         query.RequireTag(stimulus.Tag);
      }
      if(EnumHasAnyFlags(static_cast<EStimDatabaseQueryBitmaskValues>(stimSettings->QueryBitmask), EStimDatabaseQueryBitmaskValues::CheckStrength))
      {
         query.RequireStrength(stimulus.Strength);
      }
      query.RequireGlobalId(globalId);
   }
   FStimInfo& stimInfo = _FindOrAddStimInfo(
      stimulus.StimulusLocation,
      actor,
      EStimType::Audio,
      stimSeverity,
      stimulus.Tag,
      stimulus.Strength,
      globalId,
      query
   );

   stimInfo.Location = stimulus.StimulusLocation;
}

void UTATKnowledgeComponent::_OnDamageEvent(AActor* actor, FAIStimulus stimulus)
{
   if (!actor)
   {
      return;
   }
   
   // We don't need to trigger events for any expired stim.
   if(stimulus.IsExpired())
   {
      return;
   }
   
   const float now = GetWorld()->GetTimeSeconds();

   // Start tracking this actor if we're not already doing so.
   FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor);
   if (!actorKnowledge)
   {
      actorKnowledge = &_FindOrAddActorKnowledge(actor, EKnowledgeSource::OnDamage);  
   }
   check(actorKnowledge);
   actorKnowledge->SetLastKnownStimLocation(actor->GetActorLocation());
      UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose, actor->GetActorLocation(), 10.f, FColor::Red,
                       TEXT("Last Known Stim Location Set via damage for %s"), *GetNameSafe(actor));
   actorKnowledge->SetLastHitTimestamp(now);

   const bool isVisible = actorKnowledge->GetIsVisible();
   if (isVisible)
   {
      // if we can see the target & we've taken damage from them, force the identification 
      ForceActorDetectionStateIdentified(actor, actorKnowledge->GetLastKnownLocation(), now);
   }

   bool shouldSetAttitudeToHostile = isVisible && actorKnowledge->GetAttitude() != EOSETeamAttitude::Friendly;
   if(const ATATAIController* controller = GetTATAIController())
   {
      // We want to stop other NPC's seeing guards as hostile if they take damage from them (in the case of a civilian).
      if(controller->GetShouldSeeGuardsAsHostileWhenDamaged() == false)
      {
         if(const ATATCharacterAIBase* aiCharacter = Cast<ATATCharacterAIBase>(actor))
         {
            if(aiCharacter->TeamCharacter == ETATTeamCharacterType::Guard)
            {
               shouldSetAttitudeToHostile = false;
            }
         }
      }
   }
   if(shouldSetAttitudeToHostile)
   {
      // Spike attitude of instigator IF they are visible
      // NOTE: Disabled as I doubt this feature is coming back.
      //UOSEIndividualAttitudesBlueprintFunctionLibrary::SetIndividualAttitude(
      //   GetOwner(),
      //   actor,
      //   EOSEIndividualAttitude::Hostile
      //);
   }

   if(const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(GetOwner()))
   {
      // Spike to max alertness if you take damage - only passing the instigator if we can see them
      UOSEAlertnessComponent* alertnessComponent = alertnessInterface->GetAlertnessComponent();
      check(alertnessComponent);

      const EAlertnessLevel maxAlertness = alertnessComponent->GetMaxAlertnessLevel();
      alertnessComponent->AuthorityRaiseAlertnessLevelToAtLeast(maxAlertness, isVisible ? actor : nullptr);
   }
   
   const UOSEAISettings& settings = UOSEAISettings::Get();
   const EStimSeverity stimSeverity = GetGenericStimSeverity(stimulus.Tag);
   FStimDatabaseQuery query;
   query.RequireLocation(stimulus.StimulusLocation);
   query.RequireInstigator(actor);
   query.RequireStimType(EStimType::Audio);
   query.RequireSeverity(stimSeverity);
   query.RequireTag(stimulus.Tag);
   query.RequireStrength(stimulus.Strength);
   _FindOrAddStimInfo(stimulus.StimulusLocation, actor, EStimType::Damage, stimSeverity, settings.DamageStimId, stimulus.Strength, -1, query);
}

void UTATKnowledgeComponent::ShareKnowledge(const FStimInfo& stimInfo) const
{
   FStimDatabaseQuery query;
   query.RequireLocation(stimInfo.Location);
   query.RequireInstigator(stimInfo.Instigator.Get());
   query.RequireStimType(stimInfo.Type);
   query.RequireSeverity(stimInfo.Severity);
   query.RequireTag(stimInfo.Tag);
   query.RequireStrength(stimInfo.Strength);
   _FindOrAddStimInfo(stimInfo.Location, stimInfo.Instigator.Get(), stimInfo.Type, stimInfo.Severity, stimInfo.Tag, stimInfo.Strength, -1, query);
}

void UTATKnowledgeComponent::_OnTeamEvent(AActor* actor, FAIStimulus stimulus)
{
   // We don't need to trigger events for any expired stim.
   if(stimulus.IsExpired())
   {
      return;
   }
   
   if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      // refresh attitude
      const float now = GetWorld()->GetTimeSeconds();
      actorKnowledge->UpdateAttitudeToOwner();
      _UpdateActorKnowledgeFromEvent(*actorKnowledge, stimulus.StimulusLocation, now);
      UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose, actor->GetActorLocation(), 10.f, FColor::Red,
                       TEXT("Last Known Stim Location Set via team event for %s"), *GetNameSafe(actor));
   }

   // (03/25/22 - skhanna) See note in `_OnTouchEvent`
   const UOSEAISettings& settings = UOSEAISettings::Get();
   const EStimSeverity stimSeverity = GetGenericStimSeverity(settings.TeamStimId);

   FStimDatabaseQuery query;
   query.RequireLocation(stimulus.StimulusLocation);
   query.RequireInstigator(actor);
   query.RequireStimType(EStimType::Visual);
   query.RequireSeverity(stimSeverity);
   query.RequireTag(settings.TeamStimId);
   query.RequireStrength(stimulus.Strength);
   
   _FindOrAddStimInfo(stimulus.StimulusLocation, actor, EStimType::Visual, stimSeverity, settings.TeamStimId, stimulus.Strength, -1, query);
}

void UTATKnowledgeComponent::_OnTouchEvent(AActor* actor, FAIStimulus stimulus)
{
   // We don't need to trigger events for any expired stim.
   if(stimulus.IsExpired())
      return;

   // no actor? No stim, shouldn't happen but lets not support it anyway.
   if(actor == nullptr)
      return;

   const FVector ownerForward = _aiCharacter->GetActorForwardVector();
   const FVector ownerPosition = _aiCharacter->GetActorLocation();
   const FVector direction = actor->GetActorLocation() - ownerPosition;

   const float directionDot = FVector::DotProduct(ownerForward, direction);
   // if the dot value is less than our allowed one, ignore it.
   // we want to be able to stop touch events from behind the actor.
   if(directionDot < ValidTouchDirectionDotValue)
      return;
   
   const float timestamp = GetWorld()->GetTimeSeconds();

   if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(actor))
   {
      _UpdateActorKnowledgeFromEvent(*actorKnowledge, stimulus.StimulusLocation, timestamp);
      UE_VLOG_LOCATION(GetOwner(), LogTATKnowledge, Verbose, actor->GetActorLocation(), 10.f, FColor::Red,
                       TEXT("Last Known Stim Location Set via touch event for %s"), *GetNameSafe(actor));
   }

   // ignore touch events from the same team or bumping into neutrals, since this touch event turns into a stim
   const EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(_aiCharacter, actor);
   if (attitude == EOSETeamAttitude::Hostile)
   {
      // (11/22/21 - skhanna) Since we only have one touch stim right now (Shoulder Tap), we are setting the tag here.
      // There is no straight forward way to get a tag from blueprints to UEs Touch Sense, if we need to differentiate
      // between different kinds of touch in the future, we can write our own extension for the touch perception.
      // See OSEAIFunctionLibrary::ReportTouchEvent.
      const UOSEAISettings& settings = UOSEAISettings::Get();
      const EStimSeverity stimSeverity = GetGenericStimSeverity(settings.TouchStimId);
      
      FStimDatabaseQuery query;
      query.RequireLocation(stimulus.StimulusLocation);
      query.RequireInstigator(actor);
      query.RequireStimType(EStimType::Physical);
      query.RequireSeverity(stimSeverity);
      query.RequireTag(settings.TouchStimId);
      query.RequireStrength(stimulus.Strength);
      
      _FindOrAddStimInfo(stimulus.StimulusLocation, actor, EStimType::Physical, stimSeverity, settings.TouchStimId, stimulus.Strength, -1, query);
   }
}

void UTATKnowledgeComponent::_UpdateActorKnowledgeFromEvent(FTATActorKnowledge& actorKnowledge, const FVector& stimLocation, float timestamp)
{
   // If we have identified this actor, then update their last known location.
   if (actorKnowledge.GetDetectionState() == EActorDetectionState::Identified)
   {
      actorKnowledge.SetLastKnownStimLocation(stimLocation);
   }
}

void UTATKnowledgeComponent::_OnPossessedPawn(APawn* pawn)
{
   _aiCharacter = Cast<ATATCharacterAIBase>(pawn);

   if (_aiCharacter)
   {
      if (UAbilitySystemComponent* asc = _aiCharacter->GetAbilitySystemComponent())
      {
         const UCombatSettings& combatSettings = UCombatSettings::Get();
         _onAttackedDelegateHandle = asc->AddGameplayEventTagContainerDelegate(FGameplayTagContainer(combatSettings.DefenderEventTag), FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UTATKnowledgeComponent::_OnAttacked));

         const UTATAISettings& tatAISettings = UTATAISettings::Get();
         _onBamboozledDelegateHandle = asc->RegisterGameplayTagEvent(tatAISettings.BamboozledTag).AddUObject(this, &UTATKnowledgeComponent::_OnBamboozledTagChanged);
         const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
         _onUnconsciousDelegateHandle = asc->RegisterGameplayTagEvent(settings.ConditionUnconsciousTag).AddUObject(this, &UTATKnowledgeComponent::_OnOwnUnconsciousTagChanged);
      
         _onDetectionDisabledDelegateHandle = asc->RegisterGameplayTagEvent(TAG_Status_Detection_Disabled).AddUObject(this, &UTATKnowledgeComponent::_OnDetectionBlockedTagChanged);
      }

      // we cache/bind to this here instead of in BeginPlay because we don't want to know about perception things without having a character
      // see also: the unbind in _OnUnPossessedPawn()
      _perceptionComponent = GetOwner()->FindComponentByClass<UTATAIPerceptionComponent>();
      if (_perceptionComponent)
      {
         _perceptionComponent->OnTargetSightPerceptionUpdated.AddUniqueDynamic(this, &UTATKnowledgeComponent::_TargetSightPerceptionUpdated);
         _perceptionComponent->OnVisualStimEvent.AddUniqueDynamic(this, &UTATKnowledgeComponent::_OnVisualStimEvent);
         _perceptionComponent->OnTATHearingEvent.AddUniqueDynamic(this, &UTATKnowledgeComponent::_OnTATHearingEvent);
         _perceptionComponent->OnDamageEvent.AddUniqueDynamic(this, &UTATKnowledgeComponent::_OnDamageEvent);
         _perceptionComponent->OnTeamStimEvent.AddUniqueDynamic(this, &UTATKnowledgeComponent::_OnTeamEvent);
         _perceptionComponent->OnTouchStimEvent.AddUniqueDynamic(this, &UTATKnowledgeComponent::_OnTouchEvent);

         if (!_hearingStimSettingsDataTable)
         {
            UE_LOG(LogTATKnowledge, Error, TEXT("TAT AI %s hasTATAISense_Hearing but is missing the hearing stim settings data table!"), *GetOwner()->GetName());
         }
      }
   }
}

void UTATKnowledgeComponent::_OnUnPossessedPawn()
{
   if (IsValid(_aiCharacter))
   {
      for (FTATActorKnowledge& knowledge : _actors)
      {
         knowledge.OnAboutToBeRemoved(*_aiCharacter);
      }

      if (UAbilitySystemComponent* asc = _aiCharacter->GetAbilitySystemComponent())
      {
         if (_onAttackedDelegateHandle.IsValid())
         {
            const UCombatSettings& settings = UCombatSettings::Get();
            asc->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(settings.DefenderEventTag), _onAttackedDelegateHandle);
         }

         if (_onBamboozledDelegateHandle.IsValid())
         {
            const UTATAISettings& tatAISettings = UTATAISettings::Get();
            asc->UnregisterGameplayTagEvent(_onBamboozledDelegateHandle, tatAISettings.BamboozledTag);
         }
         if(_onUnconsciousDelegateHandle.IsValid())
         {
            const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
            asc->UnregisterGameplayTagEvent(_onUnconsciousDelegateHandle, settings.ConditionUnconsciousTag);
         }
         if (_onDetectionDisabledDelegateHandle.IsValid())
         {
            asc->UnregisterGameplayTagEvent(_onDetectionDisabledDelegateHandle, TAG_Status_Detection_Disabled);
         }
      }
      
      _onUnconsciousDelegateHandle.Reset();
      _onAttackedDelegateHandle.Reset();
      _onBamboozledDelegateHandle.Reset();

      if (UOSEAlertnessComponent* alertnessComponent = _aiCharacter->GetAlertnessComponent())
      {
         alertnessComponent->OnAlertnessLevelChanged.RemoveAll(this);
      }

      if (_perceptionComponent)
      {
         _perceptionComponent->OnTargetSightPerceptionUpdated.RemoveAll(this);
         _perceptionComponent->OnVisualStimEvent.RemoveAll(this);
         _perceptionComponent->OnTATHearingEvent.RemoveAll(this);
         _perceptionComponent->OnDamageEvent.RemoveAll(this);
         _perceptionComponent->OnTeamStimEvent.RemoveAll(this);
         _perceptionComponent->OnTouchStimEvent.RemoveAll(this);
      }
   }

   // Reset our internal state.
   _aiCharacter = nullptr;
   _perceptionComponent = nullptr;
   _hearingStimSettingsDataTable = nullptr;
   _onAttackedDelegateHandle.Reset();
   _actors.Reset();
   _contextLocations.Reset();
}

void UTATKnowledgeComponent::_OnOwnUnconsciousTagChanged(FGameplayTag gameplayTag, int32 newTagCount)
{
   if (_aiCharacter && newTagCount > 0)
   {
      for (FTATActorKnowledge& actorKnowledge : _actors)
      {
         if (actorKnowledge.IsPlayer())
         {
            actorKnowledge.ResetDetectionState(*_aiCharacter);
         }
      }
   }
}

void UTATKnowledgeComponent::_OnAttacked(FGameplayTag matchingTag, const FGameplayEventData* payload)
{
   if (payload && payload->Instigator && _aiCharacter && payload->Target == _aiCharacter)
   {
      const AActor* attacker = payload->Instigator;
      if (FTATActorKnowledge* actorKnowledge = GetActorKnowledge(attacker))
      {
         actorKnowledge->SetLastAttackTimestamp(GetWorld()->GetTimeSeconds());
      }
   }
}

void UTATKnowledgeComponent::_OnBamboozledTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   // we've been bamboozled!  let's forget about our detection on all players
   if (_aiCharacter && newTagCount > 0)
   {
      for (FTATActorKnowledge& actorKnowledge : _actors)
      {
         if (actorKnowledge.IsPlayer())
         {
            actorKnowledge.ResetDetectionState(*_aiCharacter);
         }
      }
   }
}

void UTATKnowledgeComponent::_OnDetectionBlockedTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   _detectionDisabled = newTagCount > 0;
}

bool UTATKnowledgeComponent::_CanForgetActor(const FTATActorKnowledge& actorKnowledge, float currentTime) const
{
   // TODO: This whole thing feels pretty kitchen-sink-y
   const FForgetnessSettings& forgetnessSettings = UAlertnessSettingsConfig::GetForgetnessSettings();

   // don't forget actors if we havent decayed into a low alert level yet (eg reset the encounter)
   if (_tatAIController->GetAlertnessLevel() > forgetnessSettings.ForgetActorMaxAlertnessLevel)
      return false;

   // don't forget actors that are identified
   if (actorKnowledge.GetDetectionState() > forgetnessSettings.ForgetActorMaxDetectionState)
      return false;

   // don't forget actors that have some detection value left to decay
   if (actorKnowledge.GetDetectionValue() > forgetnessSettings.ForgetActorMaxDetectionValue)
      return false;

   if(actorKnowledge.GetIsInvestigatingActor())
      return false;

   // otherwise forget them after the appropriate amount of time has passed
   const float latestTimestamp = FMath::Max(actorKnowledge.GetLastStimTimestamp(), actorKnowledge.GetLastVisibilityTimestamp());  // use whichever is the larger timestamp between stim/vis
   const float timeSinceLastTimestamp = (currentTime - latestTimestamp);
   const bool hasSeenActorRecently = timeSinceLastTimestamp >= forgetnessSettings.ForgetActorsCutoff;
   return hasSeenActorRecently;
}

bool UTATKnowledgeComponent::_CanForgetLocation(const FTATLocationKnowledge& locationKnowledge, float currentTime) const
{
   // otherwise forget them after the appropriate amount of time has passed
   const float latestTimestamp = locationKnowledge.GetPathCheckedTimeStamp();
   const float timeSinceLastTimestamp = (currentTime - latestTimestamp);
   const bool hasPolledLocationRecently = timeSinceLastTimestamp >= ForgetLocationsCutoff;
   return hasPolledLocationRecently;
}

void UTATKnowledgeComponent::_OnActorKnowledgeAboutToBeRemoved(const FTATActorKnowledge& actorKnowledge)
{
   if (const AActor* actor = actorKnowledge.GetActor())
   {
      UTATKnowledgeComponent* partnerKnowledge = _targetToShare.Partner.Get();
      if (partnerKnowledge != nullptr && partnerKnowledge->GetAICharacter() == actor)
      {
         TrySetPartnerForSharedTarget(nullptr);
      }
   }
   OnActorKnowledgeAboutToBeRemoved.Broadcast(actorKnowledge);
}

void UTATKnowledgeComponent::_TickDebugDrawLocalPlayerLastKnownLocation()
{
#if ENABLE_DRAW_DEBUG
   if (KnowledgeCVars::DrawLocalPlayerLastKnownLocation)
   {
      if (const ATATPlayerController* pc = ATATPlayerController::GetLocalTATPlayerController(this))
      {
         if (const FTATActorKnowledge* actorKnowledge = GetActorKnowledge(pc->GetPawn()))
         {
            DrawDebugSphere(GetWorld(), actorKnowledge->GetLastKnownVisibleLocation(), 100.0f, 12, FColor::Green, false, 1.0f, SDPG_World, 2);
            DrawDebugSphere(GetWorld(), actorKnowledge->GetLastKnownStimLocation(), 100.0f, 12, FColor::Blue, false, 5.0f, SDPG_World, 2);
         }
      }
   }
#endif // ENABLE_DRAW_DEBUG
}

void UTATKnowledgeComponent::_OnDetectionStateChanged(const FTATActorKnowledge& actorKnowledge, EActorDetectionState prevDetectionState)
{
   OnDetectionStateChanged.Broadcast(actorKnowledge, prevDetectionState);
}

FGameplayTag UTATKnowledgeComponent::GetHearingStimTag(FName tag) const
{
   if (const FTATHearingEventStimSettings* stimSettings = _GetHearingEventStimSettingsForTag(tag))
   {
      return stimSettings->Tag;
   }
   UE_LOG(LogTATKnowledge, Error, TEXT("UTATKnowledgeComponent::GetHearingStimTag called on %s but is missing the hearing stim settings data table!"), *GetOwner()->GetName());
   return FGameplayTag::EmptyTag;
}

EStimSeverity UTATKnowledgeComponent::GetHearingStimSeverity(FName tag) const
{
   if (const FTATHearingEventStimSettings* stimSettings = _GetHearingEventStimSettingsForTag(tag))
   {
      return stimSettings->Severity;
   }
   UE_LOG(LogTATKnowledge, Error, TEXT("UTATKnowledgeComponent::GetHearingStimSeverity called on %s but is missing the hearing stim settings data table!"), *GetOwner()->GetName());
   return EStimSeverity::None;
}

EStimSeverity UTATKnowledgeComponent::GetGenericStimSeverity(FName tag) const
{
   if(tag.IsNone())
      return EStimSeverity::None;
   const FOSEStimSettings* stimSettings = _genericStimSettingsDataTable ? _genericStimSettingsDataTable->FindRow<FOSEStimSettings>(tag, TEXT("TATKnowledgeComponent")) : nullptr;
   if (stimSettings)
   {
      return stimSettings->Severity;
   }
   UE_LOG(LogTATKnowledge, Error, TEXT("UTATKnowledgeComponent::GetGenericStimSeverity called on %s but is missing the hearing stim settings data table!"), *GetOwner()->GetName());
   return EStimSeverity::None;
}

void UTATKnowledgeComponent::ResetKnowledgeOfAllActors()
{
   _ClearKnownActors();
}

void UTATKnowledgeComponent::ResetDetectionOfAllActors()
{
   for (auto iterator = _actors.CreateIterator(); iterator; ++iterator)
   {
      FTATActorKnowledge& knowledge = *iterator;
      knowledge.ResetDetectionState(*_aiCharacter);
      knowledge.ResetVisibilityState(*_aiCharacter);
   }
}

UOSEStimDatabase& UTATKnowledgeComponent::_GetStimDatabase() const
{
   // assuming this is configured correctly on our controller!
   const IOSEStimDatabaseInterface* dbOwner = Cast<IOSEStimDatabaseInterface>(GetOwner());
   check(dbOwner);
   UOSEStimDatabase* db = dbOwner->AuthorityGetStimDatabase();
   check(db);
   return *db;
}

FTATLocationKnowledge& UTATKnowledgeComponent::_FindOrAddSmartObjectSlotLocationKnowledge(const USmartObjectComponent* smartObjectComponent, 
   const FSmartObjectSlotHandle& slotHandle)
{
   check(smartObjectComponent != nullptr);
   check(slotHandle.IsValid());

   bool added = false;
   FTATLocationKnowledge& locationKnowledge = _FindOrAddLocationKnowledge(smartObjectComponent, GetTypeHash(slotHandle), added);
   if (added)
   {
      // If this smart object slot has just been added, we need to set it's location.
      // Smart objects should be stationary and thus only need to have their locations set once.
      if (USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld()))
      {
         TOptional<FVector> slotLocation = smartObjectSubsystem->GetSlotLocation(slotHandle);
         check(slotLocation.IsSet());

         locationKnowledge.SetLocation(slotLocation.GetValue());
      }

      UE_CLOG(smartObjectComponent->Mobility == EComponentMobility::Type::Movable, 
         LogTATKnowledge,
         Error,
         TEXT("[%s] Smart object slot location knowledge added for a movable smart object (%s in %s)! Location changes will not be reflected..."),
         *GetName(), *smartObjectComponent->GetName(), smartObjectComponent->GetOwner() ? *smartObjectComponent->GetOwner()->GetActorNameOrLabel() : TEXT("UNKNOWN"));
   }
   return locationKnowledge;
}

FTATLocationKnowledge& UTATKnowledgeComponent::_FindOrAddLocationKnowledge(const UObject* contextObject, uint32 contextHash, bool& outAdded)
{
   check(contextObject);

   outAdded = false;

   FTATLocationKnowledge* knowledge = _contextLocations.FindByPredicate([contextObject, contextHash](const FTATLocationKnowledge& info)
   {
      return info.Matches(contextObject, contextHash);
   });

   if (knowledge == nullptr)
   {
      knowledge = &_contextLocations.Emplace_GetRef(FTATLocationKnowledge());
      knowledge->Init(this, contextObject, contextHash);
      outAdded = true;
   }

   return *knowledge;
}
