// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATTimedSwitch.h"

// tat
#include "Interactables/TATToggleResolver.h"

// ue
#include "Components/TimelineComponent.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTimedSwitch)

// Sets default values
ATATTimedSwitch::ATATTimedSwitch()
{
}

float ATATTimedSwitch::GetSecondsUntilReset() const
{
   if(IsOn())
   {
      return FMath::Max(_resetDuration - _GetResetAge(), 0.0f);
   }

   return 0.0f;
}

float ATATTimedSwitch::GetResetProgress() const
{
   return IsOn() ? _GetResetAge() / _resetDuration : 0.f;
}

void ATATTimedSwitch::SyncTimelineForReset(UTimelineComponent* timeline) const
{
   check(timeline);
   constexpr float kCloseEnoughThreshold = 0.5f;

   if(IsOn())
   {
      const float length = timeline->GetTimelineLength();
      const float age =  _GetResetAge();
      
      timeline->SetPlayRate(length / _resetDuration);
      if(age > kCloseEnoughThreshold)
      {
         const float unscaledTarget = FMath::Clamp(age / timeline->GetPlayRate(), 0, length);
         const float scaledDelta = (unscaledTarget - timeline->GetPlaybackPosition()) * timeline->GetPlayRate();

         if (FMath::Abs(scaledDelta) > kCloseEnoughThreshold)
         {
            // TODO: is it correct to fire events?
            timeline->SetPlaybackPosition(unscaledTarget, true);
         }
      }
   }
   else
   {
      // This making some assumptions
      timeline->SetPlaybackPosition(0, true);
      timeline->Stop();
   }
}

#if WITH_EDITOR
void ATATTimedSwitch::CheckForErrors()
{
   Super::CheckForErrors();

   if(_targetResolver.IsValid())
   {
      _targetResolver.Get().Validate([this](const FText& message)
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this, FText::FromString(GetActorNameOrLabel())))
            ->AddToken(FTextToken::Create(FText::FormatOrdered(INVTEXT("TargetResolver: {0}"), message)));
      });
   }
}
#endif

void ATATTimedSwitch::BeginPlay()
{
   Super::BeginPlay();

   if(_targetResolver.IsValid())
   {
      _resolvedTarget = _targetResolver.Get().ResolveToggle(this);
   }
   else
   {
      _resolvedTarget = _targetToggle;
   }
}

bool ATATTimedSwitch::_TryPriorityInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   if(Super::_TryPriorityInteractPrompt(interactingCharacter, prompt))
   {
      return true;
   }

   if(IsOn())
   {
      prompt.ErrorMessage = _includeTimeInResettingMessage
         ? _resettingMessageCache.Get(FMath::CeilToInt(GetSecondsUntilReset()), [this](int seconds) { return FText::FormatNamed(_resettingErrorMessage, TEXT("Seconds"), seconds);} )
         : _resettingErrorMessage;
      return true;
   }

   return false;
}

bool ATATTimedSwitch::_TryPriorityStartInteract(ACharacter* interactingCharacter, FInteractStartResult& outResult)
{
   if(Super::_TryPriorityStartInteract(interactingCharacter, outResult))
   {
      return true;
   }

   if(IsOn())
   {
      return true;
   }

   return false;
}

void ATATTimedSwitch::ToggleForInteraction(ACharacter* interactingCharacter)
{
   // Since there could be a misprediction on a race to use multiple switches,
   // Just keep it authority-only for now
   // CONSIDER: use a rollback like ATATExclusiveSwitch::_MaybeSchedulePredictionRollbackCheck()
   if(HasAuthority())
   {
      Super::ToggleForInteraction(interactingCharacter);
   }
}

void ATATTimedSwitch::_OnStateChanged(bool isOn, bool bWasRecent)
{
   Super::_OnStateChanged(isOn, bWasRecent);

   if(HasAuthority())
   {
      if(_resolvedTarget)
      {
         _resolvedTarget->SetToggleOn(isOn == (_targetBehavior == ETATTimedSwitchBehavior::TurnOnTarget));
      }

      if(isOn)
      {
         GetWorldTimerManager().SetTimer(_resetTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::_AuthorityOnResetTimerElapsed), _resetDuration, false);
      }
      else
      {
         GetWorldTimerManager().ClearTimer(_resetTimerHandle);
      }
   }
}

void ATATTimedSwitch::_AuthorityOnResetTimerElapsed()
{
   check(HasAuthority());
   _resetTimerHandle.Invalidate();
   TurnOff();
}

float ATATTimedSwitch::_GetResetAge() const
{
   if(IsOn())
   {
      return UOSEInteractionHelpers::GetServerTimeForComparison(this) - State.ChangedServerTime;
   }

   return 0.0f;
}
