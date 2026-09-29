// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#include "Interactables/Electrical/TATPowerSource.h"


// tat
#include "Breakables/TATBreakableComponent.h"
#include "GameFramework/TATEndgameActionComponent.h"
#include "Graphics/TATHighlightStateMgrComponent.h"
#include "Interactables/Electrical/TATPowerSourceVisComponent.h"
#include "Interactables/TATSecurityLockdownComponent.h"
#include "AI/SmartObjects/TATActionNodeComponent_IncorrectObjectState.h"
#include "AI/Perception/TATPerceptionFunctionLibrary.h"

// ue
#include "Components/StaticMeshComponent.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"

// wwise
#include "AkGameplayStatics.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPowerSource)

DEFINE_LOG_CATEGORY(LogTATPowerSource)

ATATPowerSource::ATATPowerSource()
{
   bReplicates = true;
   NetDormancy = DORM_Initial;

   _rootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
   _rootComponent->bEditableWhenInherited = true;
   _rootComponent->Mobility = EComponentMobility::Static;

   RootComponent = _rootComponent;
   
#if WITH_EDITORONLY_DATA
   _visComponent = CreateEditorOnlyDefaultSubobject<UTATPowerSourceVisComponent>(TEXT("PowerSourceVisComponent"));
#endif // WITH_EDITORONLY_DATA
   
   _securityLockdownComponent = CreateDefaultSubobject<UTATSecurityLockdownComponent>(TEXT("SecurityLockdown"));
   
   _endgameActionComponent = CreateDefaultSubobject<UTATEndgameActionComponent>(TEXT("EndgameAction"));
   _endgameActionComponent->SetDefaultAction(ETATDefaultEndgameAction::TurnOn);

   _incorrectStateActionNodeComponent = CreateDefaultSubobject<UTATActionNodeComponent_IncorrectObjectState>(TEXT("IncorrectStateActionNode"));
   _incorrectStateActionNodeComponent->SetupAttachment(_rootComponent);
   _incorrectStateActionNodeComponent->Mobility = EComponentMobility::Static;

   _perceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSourceComponent"));
}

void ATATPowerSource::AuthoritySetSwitchedOn(bool switchedOn, bool actAsIfAlwaysSet)
{
   check(HasAuthority());
   _SetSwitchedOn(switchedOn, !actAsIfAlwaysSet);
}

bool ATATPowerSource::IsPowered() const
{
   return _isPowered;
}

bool ATATPowerSource::AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const
{
   check(_incorrectStateActionNodeComponent != nullptr);
   return _incorrectStateActionNodeComponent->IsStateCorrect(allowIgnoringOfState);
}

FGameplayTagCountContainer& ATATPowerSource::GetGameplayTagCountContainer()
{
   check(_incorrectStateActionNodeComponent != nullptr);
   return _incorrectStateActionNodeComponent->GetIncorrectStateTags();
}

void ATATPowerSource::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATPowerSource, _state);
   DOREPLIFETIME(ATATPowerSource, _autoResetWorldTime);
}

bool ATATPowerSource::IsInteractable_Implementation(ACharacter* interactingChracter) const
{
   return !_breakableComponent->IsBroken();
}

void ATATPowerSource::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   prompt.PressAction = _state.bIsOn ? _turnOffPrompt : _turnOnPrompt;
}

FInteractStartResult ATATPowerSource::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   Toggle();
   return FInteractStartResult();
}

void ATATPowerSource::ShowHighlight_Implementation(const bool showHighlight)
{
   static const FName kPowerSystemName = TEXT("Power");
   UTATHighlightStateMgrComponent::HighlightMeshesWithTag(this, kPowerSystemName, showHighlight);
}

bool ATATPowerSource::CanBeSeenFrom(const FVector& observerLocation,
   FVector& outSeenLocation,
   int32& numberOfLoSChecksPerformed,
   float& outSightStrength,
   const AActor* ignoreActor,
   const bool* wasVisible,
   int32* userData) const
{
   static constexpr bool kNonColliding = false;
   static constexpr bool kIncludeFromChildActors = false;
   const FBox actorBounds = GetComponentsBoundingBox(kNonColliding, kIncludeFromChildActors);
   return UTATPerceptionFunctionLibrary::CanActorBoundingBoxBeSeenFromLocation(
      this,
      actorBounds,
      observerLocation,
      outSeenLocation,
      numberOfLoSChecksPerformed,
      outSightStrength,
      ignoreActor
   );
}

void ATATPowerSource::Toggle()
{
   _SetSwitchedOn(!IsSwitchedOn());
}

void ATATPowerSource::BeginPlay()
{
   Super::BeginPlay();

   SyncTimelines(_state);

   _breakableComponent->OnBrokenChanged.AddUniqueDynamic(this, &ATATPowerSource::_OnIsBrokenChanged);   
   _securityLockdownComponent->OnSecurityLockdownStateChanged.AddUniqueDynamic(this, &ThisClass::_OnSecurityLockdownStateChanged);

   _TryBindToParentPowerSource();
   
   _RefreshIsPowered();
   
   if (_incorrectStateActionNodeComponent != nullptr)
   {
      const int32 currentState = IsSwitchedOn() ? 1 : 0;
      if (_isPowered == false)
      {
         _incorrectStateActionNodeComponent->IsEnabled = false;
      }
      else
      {
         _incorrectStateActionNodeComponent->SetInitialState(currentState, currentState);
      }
   }

   if (_toggleableByFairy)
   {
      Tags.AddUnique(FName("ToggleableByFairy"));
   }
}

void ATATPowerSource::EndPlay(const EEndPlayReason::Type reason)
{
   _breakableComponent->OnBrokenChanged.RemoveAll(this);
   _TryUnbindFromParentPowerSource();

   Super::EndPlay(reason);
}

void ATATPowerSource::PostRegisterAllComponents()
{
   Super::PostRegisterAllComponents();

#if WITH_EDITOR
   // If we were created in the editor just now by duplicating an existing power source with assigned parent, 
   // refresh that parent vis component's cached children so it draws lines back to us
   _TrySetParentVisComponentDirty();
#endif // WITH_EDITOR
}

void ATATPowerSource::PostInitializeComponents()
{
   Super::PostInitializeComponents();
   if (HasAuthority() && _incorrectStateActionNodeComponent != nullptr)
   {
      // Incorrect state component will pay attention to whether this actor is broken or not.
      // If broken, the object will be marked as not in an incorrect state as it's a state 
      // the AI can't do anything about
      _incorrectStateActionNodeComponent->AssignBreakableComponent(_breakableComponent);

      // Incorrect state component will enable/disable the stimuli source component based on
      // whether the object is in a correct/incorrect state. If in incorrect state, it will be 
      // visible. If in a correct state, it will not be visible.
      _incorrectStateActionNodeComponent->AssignStimuliSourceComponent(_perceptionStimuliSource);
   }
}

#if WITH_EDITOR
void ATATPowerSource::CheckForErrors()
{
   Super::CheckForErrors();
   
   // Validate parent power source to avoid circular chains
   if (_parentPowerSource.IsValid())
   {
      const ATATPowerSource* outSelfReferencingPowerSource = nullptr;
      if (_PowerSourceChainHasCircularDependencyToSelf(outSelfReferencingPowerSource))
      {
         check(outSelfReferencingPowerSource);
         FText error;
         if (outSelfReferencingPowerSource == this)
         {
            error = FText::Format(INVTEXT("[{0}] _parentPowerSource references self!"), FText::FromString(GetActorLabel()));
         }
         else
         {
            error = FText::Format(INVTEXT("[{0}] Circular power source dependency detected! (ancestor {1} has this as a _parentPowerSource)!")
               , FText::FromString(GetActorLabel())
               , FText::FromString(outSelfReferencingPowerSource->GetActorLabel())
            );
         }
         FMessageLog("MapCheck")
            .Error()
            ->AddToken(FUObjectToken::Create(this, FText::FromString(GetActorNameOrLabel())))
            ->AddToken(FTextToken::Create(error));
      }
   }
}

void ATATPowerSource::PreEditChange(FProperty* propertyThatWillChange)
{
   if (propertyThatWillChange && propertyThatWillChange->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, _parentPowerSource))
   {
      // Mark previously-assigned parent dirty so vis-compoennt-cached children are refreshed upon selection
      _TrySetParentVisComponentDirty();
   }

   Super::PreEditChange(propertyThatWillChange);
}

void ATATPowerSource::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   if (propertyChangedEvent.Property && propertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, _parentPowerSource))
   {
      // Mark (newly-assigned) parent dirty for re-evaluation of children upon vis component selection
      _TrySetParentVisComponentDirty();
   }
}
#endif // WITH_EDITOR

float ATATPowerSource::GetNormalizedTimeRemainingOnAutoReset() const
{
   const float maxConfigTime = _configTimeUntilAutoReset;
   const float timeRemaining = _autoResetWorldTime - GetWorld()->GetTimeSeconds();
   return FMath::GetMappedRangeValueClamped(FVector2D(maxConfigTime, 0.f), FVector2D(0,1), timeRemaining);
}

void ATATPowerSource::_OnRep_State(const FOSEToggleState& previousState)
{
   SyncTimelines(_state);
   if (previousState.bIsOn != _state.bIsOn)
   {
      K2_OnPowerSwitched.Broadcast();
      OnToggled(_state.bIsOn);
      if (!_state.IsOld(this, 0.75f))
      {
         OnRecentlyToggled(_state.bIsOn);
      }

      _RefreshIsPowered();
   }
}

void ATATPowerSource::_OnIsBrokenChanged(bool isBroken)
{
   _RefreshIsPowered();
}

void ATATPowerSource::_OnParentPowerSourceIsPoweredChanged(bool isPowered)
{
   check(_parentPowerSource.IsValid());
   UE_LOG(LogTATPowerSource, Verbose, TEXT("[%s] had power %s parent source (%s)")
      , *GetName()
      , isPowered ? TEXT("restored") : TEXT("cut")
      , *_parentPowerSource->GetName());

   _RefreshIsPowered();
}

void ATATPowerSource::_OnIsPoweredChanged()
{
   const bool powered = IsPowered();
   OnPowerStateChanged.Broadcast(powered);
   K2_OnPowerStateChanged.Broadcast();
   OnPoweredChanged(powered);
}

#if WITH_EDITOR
bool ATATPowerSource::_PowerSourceChainHasCircularDependencyToSelf(const ATATPowerSource*& outSelfReferencingPowerSource) const
{
   const ATATPowerSource* parentPowerSource = GetParentPowerSource();
   if (parentPowerSource == this)
   {
      outSelfReferencingPowerSource = this;
      return true;
   }

   TArray<const ATATPowerSource*> powerSourceChain({this});
   while (parentPowerSource != nullptr)
   {
      // Detect circular dependency
      if (powerSourceChain.Contains(parentPowerSource))
      {
         // If the bad reference is to us, return true with the culprit
         if (parentPowerSource == this)
         {
            outSelfReferencingPowerSource = powerSourceChain.Last();
            return true;
         }

         // Otherwise return false to avoid log spam from each power source in the chain
         break;
      }

      powerSourceChain.Add(parentPowerSource);
      parentPowerSource = parentPowerSource->GetParentPowerSource();
   }

   outSelfReferencingPowerSource = nullptr;
   return false;
}

void ATATPowerSource::_TrySetParentVisComponentDirty()
{
#if WITH_EDITORONLY_DATA
   if (const ATATPowerSource* parent = _parentPowerSource.Get())
   {
      parent->_visComponent->IsDirty = true;
   }
#endif // WITH_EDITORONLY_DATA
}
#endif // WITH_EDITOR

bool ATATPowerSource::_HasUnpoweredParentPowerSource() const
{
   if (const ATATPowerSource* parentPowerSource = _parentPowerSource.Get())
   {
      return !parentPowerSource->IsPowered();
   }
   return false;
}

void ATATPowerSource::_TryBindToParentPowerSource()
{
   if (ATATPowerSource* parentPowerSource = GetParentPowerSource())
   {
      // Editor-only circular dependency checking (skip otherwise for perf)
#if WITH_EDITOR
      const ATATPowerSource* outSelfReferencingPowerSource = nullptr;
      if (_PowerSourceChainHasCircularDependencyToSelf(outSelfReferencingPowerSource))
      {
         UE_LOG(LogTATPowerSource, Error, TEXT("[%s] Circular power source dependency detected! Ignoring parent power source to prevent recursive callbacks"), *GetName());
         _parentPowerSource.Reset();
         return;
      }
#endif
      
       parentPowerSource->OnPowerStateChanged.AddUObject(this, &ATATPowerSource::_OnParentPowerSourceIsPoweredChanged);
   }
}
void ATATPowerSource::_TryUnbindFromParentPowerSource()
{
   if(ATATPowerSource* parentPowerSource = GetParentPowerSource())
   {
      parentPowerSource->OnPowerStateChanged.RemoveAll(this);
   }
}

void ATATPowerSource::_OnSecurityLockdownStateChanged(bool bLockedDown)
{
   if (IsBroken())
   {
      return;
   }
   if (HasAuthority() && bLockedDown)
   {
      AuthoritySetSwitchedOn(true, true);
   }
}

void ATATPowerSource::OnRep_AutoResetWorldTimeChanged()
{
   _HandleAutoResetWorldTimeChanged();
}

void ATATPowerSource::_HandleAutoResetWorldTimeChanged_Implementation()
{
}

void ATATPowerSource::_AuthorityOnAutoResetTimerTriggered()
{
   if(_timeRemainingUntilAutoReset-- <= 0)
   {
      AuthoritySetSwitchedOn(true);
      return;
   }
   MulticastTriggerAudibleTickFromResetTimer();
}

void ATATPowerSource::MulticastTriggerAudibleTickFromResetTimer_Implementation()
{
   // Note this is a multicast and is purely cosmetic, so I don't care if this is culled under heavy network conditions
   _HandleAudibleTickFromResetTimer();
}

void ATATPowerSource::_HandleAudibleTickFromResetTimer_Implementation()
{
   UAkGameplayStatics::PostEventAtLocation(_audibleTickFromResetTimerEvent, GetActorLocation(), GetActorRotation(), this);
}

void ATATPowerSource::_SetSwitchedOn(bool switchedOn, bool writeChangedTime)
{
   if (IsSwitchedOn() != switchedOn)
   {
      FlushNetDormancy();
      const FOSEToggleState oldState = _state;
      _state.bIsOn = !_state.bIsOn;
      if (writeChangedTime)
      {
         _state.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);
      }
      if(HasAuthority())
      {
         if(_shouldAutoReset)
         {
            if(switchedOn)
            {
               _autoResetWorldTime = INDEX_NONE;
               _HandleAutoResetWorldTimeChanged();
               GetWorld()->GetTimerManager().ClearTimer(_autoResetHandle);
            }
            else
            {
               _timeRemainingUntilAutoReset = _configTimeUntilAutoReset;
               _autoResetWorldTime = GetWorld()->GetTimeSeconds() + _configTimeUntilAutoReset;
               _HandleAutoResetWorldTimeChanged();
               GetWorld()->GetTimerManager().SetTimer(_autoResetHandle, FTimerDelegate::CreateUObject(this, &ThisClass::_AuthorityOnAutoResetTimerTriggered), 1.f, true);
            }
         }
         if (_incorrectStateActionNodeComponent && _incorrectStateActionNodeComponent->IsEnabled == false && switchedOn)
         {
            _incorrectStateActionNodeComponent->IsEnabled = true;
            _incorrectStateActionNodeComponent->SetInitialState(switchedOn, switchedOn);
         }
         _incorrectStateActionNodeComponent->SetCurrentState(switchedOn ? 1 : 0);
      }
      _OnRep_State(oldState);
   }
}

void ATATPowerSource::_RefreshIsPowered()
{
   const bool wasPowered = _isPowered;
   _isPowered = IsSwitchedOn() && !IsBroken() && !_HasUnpoweredParentPowerSource();
   if (_isPowered != wasPowered)
   {
      _OnIsPoweredChanged();
   }
}
