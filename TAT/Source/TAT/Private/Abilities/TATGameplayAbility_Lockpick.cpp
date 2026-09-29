// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATGameplayAbility_Lockpick.h"

// tat
#include "AbilitySystemComponent.h"
#include "Developer/TATEditorSettings.h"
#include "Lockpicking/TATLockpickingSettings.h"

// ose
#include "Player/OSEPlayerStats.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_Lockpick)
DEFINE_LOG_CATEGORY_STATIC(LogTATGameplayAbility_Lockpick, Log, All);

UTATGameplayAbility_Lockpick::UTATGameplayAbility_Lockpick()
{
   
}

#if WITH_EDITOR
EDataValidationResult UTATGameplayAbility_Lockpick::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   // Ensure minigame variation asset is assigned
   if (!IsValid(LockpickMinigameVariationDataAsset))
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("_lockpickMinigameVariationDataAsset is unassigned!"))));
   }

   return context.GetNumErrors() + context.GetNumWarnings() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

#if OSE_CHEATS_ENABLED
void UTATGameplayAbility_Lockpick::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, ownerInfo, activationInfo, triggerEventData);

   const UTATEditorSettings& editorSettings = UTATEditorSettings::Get();
   if (IsLocallyControlled() && editorSettings.DisableLockpickingMinigame)
   {
      // auto-end the ability without cancellation to bypass the minigame
      const bool bReplicateEndAbility = true;
      const bool bWasCancelled = false;
      EndAbility(handle, ownerInfo, activationInfo, bReplicateEndAbility, bWasCancelled);
   }
}
#endif // OSE_CHEATS_ENABLED

AActor* GetActorFromLockPickInterface(UObject* object)
{
   if(AActor* asActor = Cast<AActor>(object))
   {
      return asActor;
   }
   if (const UActorComponent* asActorComponent = Cast<UActorComponent>(object))
   {
      return asActorComponent->GetOwner();
   }
   return nullptr;
}

void UTATGameplayAbility_Lockpick::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled)
{
   // Clear any internally managed cues/abilities
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo())
   {
      FGameplayTagContainer abilityTags = _lastTriggeredAbility.GetSingleTagContainer();
      asc->CancelAbilities(&abilityTags);
      _lastTriggeredAbility = FGameplayTag::EmptyTag;
   }
   
   if(AActor* lockpickActor = GetActorFromLockPickInterface(_lockpickableActor.GetObject()))
   {
      lockpickActor->OnEndPlay.RemoveAll(this);
   }

   if (TScriptInterface<ILockpickableInterface> lockpickActor = GetLockpickableActor())
   {
      if (FTATOnRequestCancelLockpicking* cancelLockpickingDelegate = lockpickActor->GetOnRequestCancelLockpickingDelegate())
      {
         cancelLockpickingDelegate->RemoveAll(this);
      }

      // wasCancelled is used as an heuristic distinguishing a successful lockpick from a cancelled attempt 
      // (make sure to call EndAbility() on a successful lockpick, and CancelAbility() otherwise)
      if (wasCancelled)
      {
         lockpickActor->OnLockpickCancelled();
      }
      else
      {
         lockpickActor->Unlock();
         UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(actorInfo->AvatarActor.Get(), UTATLockpickingSettings::GetLockpickingSettingsRef().LocksPickedPlayerStat);
      }
   }

   Super::EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);
}

void UTATGameplayAbility_Lockpick::InitFromLockpickable(TScriptInterface<ILockpickableInterface> lockpickableActor, const FTATLockpickMinigameVariation& lockpickMinigameVariation)
{
   _lockpickableActor = lockpickableActor;
   _lockpickMinigameVariation = lockpickMinigameVariation;
   CurrentTrackTimeSeconds = 0.0f;

   // Clear out any data from previous lockpickings
   _currentTrackLayerStack.Clear();

   // Bind to EndPlay so we can cancel minigame on unexpected destruction
   if(AActor* lockpickActor = GetActorFromLockPickInterface(_lockpickableActor.GetObject()))
   {
      lockpickActor->OnEndPlay.AddDynamic(this, &UTATGameplayAbility_Lockpick::_OnLockpickableActorEndPlay);
   }
   
   if (TScriptInterface<ILockpickableInterface> lockpickActor = GetLockpickableActor())
   {
      _currentTrackIndex = lockpickActor->GetLockpickCurrentTrack();
      _lockpickMinigameVariation.ValidateTrackIndex(_currentTrackIndex);

      if (FTATOnRequestCancelLockpicking* cancelLockpickingDelegate = lockpickActor->GetOnRequestCancelLockpickingDelegate())
      {
         cancelLockpickingDelegate->AddUObject(this, &UTATGameplayAbility_Lockpick::_OnLockpickableActorCancelledLockpicking);
      }
   }
   else
   {
      _currentTrackIndex = INDEX_NONE;
   }
}

TScriptInterface<ILockpickableInterface> UTATGameplayAbility_Lockpick::GetLockpickableActor() const
{
   if (_lockpickableActor.IsValid())
   {
      return _lockpickableActor.ToScriptInterface();
   }

   UE_LOG(LogTATGameplayAbility_Lockpick, Warning, TEXT("GetLockpickableActor() called with invalid _lockpickableActor reference!"));
   return nullptr;
}

void UTATGameplayAbility_Lockpick::TickTrackProgress(float deltaTime)
{
   // If called on invalid curve, track, or pressure point data, early out and skip the minigame
   // TODO: implement data validation to catch this
   if (!_lockpickMinigameVariation.HasValidData())
   {
      UE_LOG(LogTATGameplayAbility_Lockpick, Error, TEXT("TickTrackProgress() called with invalid lockpick minigame variation data! Ending minigame..."));
      OnAllTracksReachedEnd();
      return;
   }

   // Update the track time and clamp within bounds
   CurrentTrackTimeSeconds += _GetTrackSpeed() * deltaTime;
   CurrentTrackTimeSeconds = FMath::Clamp(CurrentTrackTimeSeconds, 0.f, _currentTrackLayerStack.GetDurationSeconds());

   // Cache our last section before updating cached data to track when sections change
   const FTATLockpickTrackSection* previousSection = _currentTrackLayerStack.GetCurrentSection();

   // Update our cached data and notify blueprint
   _currentTrackLayerStack.UpdateTrackTime(CurrentTrackTimeSeconds);
   OnTrackProgressChanged(_currentTrackIndex, CurrentTrackTimeSeconds);

   if (_currentTrackLayerStack.IsAtEnd())
   {
      OnTrackReachedEnd(_currentTrackIndex, _AreRequiredSectionsComplete());
   }
   else if (previousSection != _currentTrackLayerStack.GetCurrentSection())
   {
      // If we've changed active sections, trigger the gameplay events associated with the new section
      _TriggerCurrentSectionEvents();
   }
}

void UTATGameplayAbility_Lockpick::TryStartNewTrack()
{
   if (TScriptInterface<ILockpickableInterface> lockpickableActor = GetLockpickableActor())
   {
      CurrentTrackTimeSeconds = 0.0f;

      // Go to the next track if we've just finished one
      if (_currentTrackLayerStack.IsValid())
      {
         _lockpickMinigameVariation.ValidateTrackIndex(++_currentTrackIndex);
      }

      if (_currentTrackIndex != INDEX_NONE)
      {
         const FTATLockpickMinigameTrackDefinition& trackDefinition = _lockpickMinigameVariation.GetTrackDefinition(_currentTrackIndex);
         _currentTrackLayerStack.Build(trackDefinition);
         OnNewTrackStarted();

         // Trigger the initial section's events
         _TriggerCurrentSectionEvents();
      }
      else
      {
         OnAllTracksReachedEnd();
      }
   }
}

float UTATGameplayAbility_Lockpick::_GetTrackSpeed() const
{
   if (const FTATLockpickTrackSection* currentSection = _currentTrackLayerStack.GetCurrentSection())
   {
      return currentSection->TrackSpeed;
   }
   else
   {
      // If GetCurrentSection() returns nullptr, we've reahced the end of this track
      return 0.0f;
   }
}

void UTATGameplayAbility_Lockpick::_TriggerCurrentSectionEvents()
{
   UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo();
   check(asc);

   bool removeExistingAbility = true;
   FGameplayTag newAbility = FGameplayTag::EmptyTag;

   const FTATLockpickTrackSection* currentSection = _currentTrackLayerStack.GetCurrentSection();
   check(currentSection);

   const UTATLockpickingSettings& lockpickSettings = UTATLockpickingSettings::GetLockpickingSettingsRef();
   const FTATLockpickTrackSectionTypeConfig& sectionConfig = lockpickSettings.TrackSectionConfigs.FindChecked(currentSection->SectionType);

   removeExistingAbility = (_lastTriggeredAbility != FGameplayTag::EmptyTag);
   newAbility = sectionConfig.GameplayAbilityTag;

   if (removeExistingAbility)
   {
      FGameplayTagContainer abilityTags = _lastTriggeredAbility.GetSingleTagContainer();
      asc->CancelAbilities(&abilityTags);
      _lastTriggeredAbility = FGameplayTag::EmptyTag;
   }

   if (newAbility != FGameplayTag::EmptyTag)
   {
      FGameplayEventData payload;
      payload.EventTag = newAbility;
      payload.Instigator = GetOwningActorFromActorInfo();
      SendGameplayEvent(newAbility, payload);
      _lastTriggeredAbility = newAbility;
   }
}

void UTATGameplayAbility_Lockpick::_OnLockpickableActorEndPlay(AActor* actor, EEndPlayReason::Type endPlayReason)
{
   AActor* lockpickActor = GetActorFromLockPickInterface(_lockpickableActor.GetObject());
   checkf(lockpickActor == actor, TEXT("_OnLockpickableActorEndPlay() called with unexpected actor %s (expected _lockpickableActor %s!)")
      , *actor->GetName()
      , *_lockpickableActor.GetObject()->GetName());

   UE_LOG(LogTATGameplayAbility_Lockpick, Verbose, TEXT("Lockpickable actor %s broadcast EndPlay (reason = %s), cancelling ability...")
      , *actor->GetName()
      , *UEnum::GetValueAsString(endPlayReason));

   // Don't need to replicate, the actor's end-play should replicate to client and produce a callback
   constexpr bool replicateCancel = false;
   CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, replicateCancel);
}

void UTATGameplayAbility_Lockpick::_OnLockpickableActorCancelledLockpicking()
{
   UE_LOG(LogTATGameplayAbility_Lockpick, Verbose, TEXT("Lockpickable actor %s requested to cancel lockpicking, cancelling ability..."), *GetNameSafe(_lockpickableActor.GetObject()));
   constexpr bool replicateCancel = true;
   CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, replicateCancel);
}

bool UTATGameplayAbility_Lockpick::_AreRequiredSectionsComplete() const
{
   switch (SectionRequirement)
   {
   case ETATLockpickSectionRequirement::RequireAll:
      return _currentTrackLayerStack.AreSectionsInteractedWith(RequiredSectionTag);
   case ETATLockpickSectionRequirement::RequireOne:
      // It would be silly to fail if there are none at all
      return _currentTrackLayerStack.IsAnySectionInteractedWith(RequiredSectionTag) || !_currentTrackLayerStack.HasSectionType(RequiredSectionTag);
   case ETATLockpickSectionRequirement::RequireNone:
      return true;
   default:
      checkNoEntry();
      return false;
   }
}

void UTATGameplayAbility_Lockpick::TriggerTrackInteract()
{
   if (const FTATLockpickTrackSection* currentSection = _currentTrackLayerStack.GetCurrentSection())
   {
      const UTATLockpickingSettings& lockpickSettings = UTATLockpickingSettings::GetLockpickingSettingsRef();
      const FTATLockpickTrackSectionTypeConfig& sectionConfig = lockpickSettings.TrackSectionConfigs.FindChecked(currentSection->SectionType);
      if (sectionConfig.InteractedSectionType.IsValid())
      {
         if (TScriptInterface<ILockpickableInterface> lockpickActor = GetLockpickableActor())
         {
            _currentTrackLayerStack.AddInteractionTime(CurrentTrackTimeSeconds);

            OnTrackInteractionTriggered(_currentTrackIndex, CurrentTrackTimeSeconds);

            // Trigger the new section's gameplay events
            _TriggerCurrentSectionEvents();
         }
      }
   }
}

void UTATGameplayAbility_Lockpick::MarkTrackCompleted()
{
   // Only mark progress on authority, as the benefit for predicting this is relatively low
   if (!HasAuthority(&CurrentActivationInfo))
   {
      return;
   }

   // Don't bother marking progress for the last track, since it will complete anyways
   if (_currentTrackIndex + 1 >= _lockpickMinigameVariation.GetTrackCount())
   {
      return;
   }

   if (TScriptInterface<ILockpickableInterface> lockpickActor = GetLockpickableActor())
   {
      lockpickActor->OnLockpickTrackCompleted(_currentTrackIndex);
   }
}

int32 UTATGameplayAbility_Lockpick::GetCurrentTrack() const
{
   return _currentTrackIndex;
}
