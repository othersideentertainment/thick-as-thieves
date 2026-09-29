// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATExclusiveSwitch.h"

// tat
#include "Interactables/TATExclusiveSwitchSet.h"
#include "Interactables/TATExplicitTransitionToggle.h"

// ue5
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATExclusiveSwitch)

ATATExclusiveSwitch::ATATExclusiveSwitch()
{
   _predictionCheckRollbackWindow = 0.75f;
}

void ATATExclusiveSwitch::BeginPlay()
{
   Super::BeginPlay();

   _cachedToggleable = Cast<UTATExplicitTransitionToggleComponent>(_targetToggleable.GetComponent(nullptr));

   if (IsValid(_cachedToggleable))
   {
      _cachedToggleable->OnStateOrTransitionChanged.AddUniqueDynamic(this, &ATATExclusiveSwitch::_OnTargetStateOrTransitionChanged);
      _cachedToggleable->OnStateOrTransitionChangedRecently.AddUniqueDynamic(this, &ATATExclusiveSwitch::_OnTargetStateOrTransitionChangedRecently);
      _OnTargetStateOrTransitionChanged(_cachedToggleable->IsOn(), _cachedToggleable->IsTransitioning());

      if (IsValid(_switchSet))
      {
         _switchSet->OnInUseChanged.AddUniqueDynamic(this, &ATATExclusiveSwitch::_OnSetInUseChanged);
         _UpdateOtherSwitchInUse();
      }

      if (HasAuthority())
      {
         SetOn(_cachedToggleable->IsOn());

         if (IsValid(_switchSet))
         {
            _switchSet->AuthorityRegisterSwitchToggle(_cachedToggleable);
         }
      }
   }
}

void ATATExclusiveSwitch::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   if (_CanCurrentlyToggle())
   {
      Super::GetInteractPrompt_Implementation(interactingCharacter, prompt);
   }
   else
   {
      const bool isTransitioning = _IsTargetProbablyTransitioning();
      prompt.ErrorMessage = isTransitioning ? AlreadyTransitioningErrorPrompt : InUseErrorPrompt;
   }
}

FInteractStartResult ATATExclusiveSwitch::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   if (!_CanCurrentlyToggle())
   {
      FInteractStartResult result;
      if (!_IsTargetProbablyTransitioning())
      {
         result.Message = InUseErrorMessage;
         BP_OnFailedToUseLocally(interactingCharacter);
      }
      return result;
   }

   return Super::StartInteract_Implementation(interactingCharacter);
}

bool ATATExclusiveSwitch::IsTargetTransitioning() const
{
   return IsValid(_cachedToggleable) && _cachedToggleable->IsTransitioning();
}

bool ATATExclusiveSwitch::IsOtherSwitchInUse() const
{
   return _isOtherSwitchInUse;
}

UTATExplicitTransitionToggleComponent* ATATExclusiveSwitch::GetTargetToggleable() const
{
   // can add handling for pre-begin-play if it comes up
   return _cachedToggleable;
}

void ATATExclusiveSwitch::ToggleForInteraction(ACharacter* interactingCharacter)
{
   Super::ToggleForInteraction(interactingCharacter);
   _MaybeSchedulePredictionRollbackCheck();

   if (HasAuthority())
   {
      if (IsValid(_cachedToggleable))
      {
         _cachedToggleable->AuthorityTransitionTo(IsOn());
      }
   }
}

bool ATATExclusiveSwitch::_CanCurrentlyToggle() const
{
   if (!IsValid(_cachedToggleable))
   {
      return false;
   }

   if (_IsTargetProbablyTransitioning())
   {
      return false;
   }

   if (!IsValid(_switchSet))
   {
      return true;
   }

   return _cachedToggleable->IsOn() != _switchSet->DefaultState || !_switchSet->IsInUse();
}

#if WITH_EDITOR
void ATATExclusiveSwitch::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      if (_targetToggleable.GetComponent(nullptr) == nullptr)
      {
         FFormatNamedArguments arguments;
         arguments.Add(TEXT("ActorName"), FText::FromString(GetPathName()));
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} : Exclusive switch does not have target")), arguments)));
      }

      if (!IsValid(_switchSet))
      {
         FFormatNamedArguments arguments;
         arguments.Add(TEXT("ActorName"), FText::FromString(GetPathName()));
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} : Exclusive switch is not in a switch set (please ask if you want to support this, but this catches more errors)")), arguments)));
      }
   }

   auto checkTextInStringTable = [this](const FText& text, const TCHAR* fieldName)
   {
      if (!text.IsFromStringTable() && !text.IsEmpty())
      {
         FFormatNamedArguments arguments;
         arguments.Add(TEXT("ActorName"), FText::FromString(GetPathName()));
         arguments.Add(TEXT("FieldName"), FText::FromString(fieldName));
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} : Text {FieldName} is not in a string table")), arguments)));
      }
   };

   checkTextInStringTable(AlreadyTransitioningErrorPrompt, TEXT("AlreadyTransitioningErrorPrompt"));
   checkTextInStringTable(InUseErrorPrompt, TEXT("InUseErrorPrompt"));
   checkTextInStringTable(InUseErrorMessage, TEXT("InUseErrorMessage"));
}
#endif

void ATATExclusiveSwitch::_UpdateOtherSwitchInUse()
{
   if (!IsValid(_cachedToggleable) || !IsValid(_switchSet)) return;

   const bool otherInUse = _switchSet->IsInUse() && _switchSet->GetInUseToggle() != _cachedToggleable;
   if (otherInUse != _isOtherSwitchInUse)
   {
      _isOtherSwitchInUse = otherInUse;
      BP_OnOtherSwitchInUseChanged(_isOtherSwitchInUse);
   }
}

void ATATExclusiveSwitch::_OnSetInUseChanged(bool inUse)
{
   _UpdateOtherSwitchInUse();
}

void ATATExclusiveSwitch::_MaybeSchedulePredictionRollbackCheck()
{
   if(HasAuthority() || _predictionCheckRollbackWindow <= 0) return;

   // A client can predict the state of the switch, but not the target of the switch
   // it will then check for a rollback after some delay
   _checkRollbackTimestamp = State.ChangedServerTime;
   GetWorldTimerManager().SetTimer(_checkRollbackTimerHandle, this, &ATATExclusiveSwitch::_CheckForPredictionRollback, _predictionCheckRollbackWindow);
}

void ATATExclusiveSwitch::_CheckForPredictionRollback()
{
   // If the changed server time has not been altered in the window (and this not acked),
   // then mirror the state of the target toggleable (assumes the target is relevant,
   // but so does the transition check [revisit if needed to explicitly replicate])
   if (_checkRollbackTimestamp == State.ChangedServerTime && IsValid(_cachedToggleable))
   {
      SetOn(_cachedToggleable->IsOn());
   }
}

bool ATATExclusiveSwitch::_IsTargetProbablyTransitioning() const
{
   // If the switch has been change recently, assume the target is probably still transitioning
   // Aim is to fill gap before the target toggles state is replicated back, since that is not predicted
   // NOTE: Assumes target transition is probably longer than this threshold (Make this dynamic if that changes?)
   return IsTargetTransitioning() || (!HasAuthority() && !UOSEInteractionHelpers::IsOld(this, State.ChangedServerTime, _predictionCheckRollbackWindow));
}

void ATATExclusiveSwitch::_OnTargetStateOrTransitionChanged(bool isOn, bool isTransitioning)
{
   // just in case something else controls this toggle, mirror it
   if (HasAuthority())
   {
      SetOn(isOn);
   }

   BP_OnTargetStateOrTransitionChanged(isOn, isTransitioning);
}

void ATATExclusiveSwitch::_OnTargetStateOrTransitionChangedRecently(bool isOn, bool isTransitioning)
{
   BP_OnTargetStateOrTransitionChangedRecently(isOn, isTransitioning);
}

