// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/SmartObjects/TATActionNodeComponent_IncorrectObjectState.h"

// tat
#include "AI/Navigation/TATNavLinkOwnerComponent.h"
#include "AI/Perception/TATAISense_Sight.h"
#include "AI/SmartObjects/TATSmartObjectObjectTags.h"
#include "Breakables/TATBreakableComponent.h"
#include "Breakables/TATBreakableTags.h"

// ue
#include "AbilitySystemComponent.h"
#include "SmartObjectSubsystem.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActionNodeComponent_IncorrectObjectState)

void UTATActionNodeComponent_IncorrectObjectState::InitializeComponent()
{
   Super::InitializeComponent();
   UAbilitySystemComponent* propAsc = GetOwner()->FindComponentByClass<UAbilitySystemComponent>();
   _abilitySystemComponent = propAsc;
}

void UTATActionNodeComponent_IncorrectObjectState::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UTATBreakableComponent* component = _breakableComponent.Get())
   {
      component->OnBrokenChanged.RemoveDynamic(this, &UTATActionNodeComponent_IncorrectObjectState::_OnIsBrokenChanged);
   }

   if (UTATNavLinkOwnerComponent* component = _navLinkOwnerComponent.Get())
   {
      component->OnNavLinkReservationChanged.RemoveAll(this);
   }

   Super::EndPlay(endPlayReason);
}

void UTATActionNodeComponent_IncorrectObjectState::SetInitialState(int32 correct, int32 current)
{
   _correctState = correct;
   SetCurrentState(current);
}

void UTATActionNodeComponent_IncorrectObjectState::SetCurrentState(int32 state)
{
   _currentState = state;
   _OnStateChanged();
}

bool UTATActionNodeComponent_IncorrectObjectState::IsStateCorrect(const bool allowIgnoringOfState) const
{
   return (_ShouldIgnoreIncorrectState() && allowIgnoringOfState) || _currentState == _correctState;
}

bool UTATActionNodeComponent_IncorrectObjectState::_ShouldIgnoreIncorrectState() const
{
   return !IsEnabled || _hasReservingNavAgents;
}

void UTATActionNodeComponent_IncorrectObjectState::AssignBreakableComponent(UTATBreakableComponent* breakable)
{
   if (UTATBreakableComponent* component = _breakableComponent.Get())
   {
      component->OnBrokenChanged.RemoveDynamic(this, &UTATActionNodeComponent_IncorrectObjectState::_OnIsBrokenChanged);
      _isBroken = false;
   }

   _breakableComponent = breakable;

   if (UTATBreakableComponent* component = _breakableComponent.Get())
   {
      component->OnBrokenChanged.AddUniqueDynamic(this, &UTATActionNodeComponent_IncorrectObjectState::_OnIsBrokenChanged);
      _OnIsBrokenChanged(component->IsBroken());
   }
}

void UTATActionNodeComponent_IncorrectObjectState::AssignNavLinkOwnerComponent(UTATNavLinkOwnerComponent* navLinkOwner)
{
   if (UTATNavLinkOwnerComponent* component = _navLinkOwnerComponent.Get())
   {
      component->OnNavLinkReservationChanged.RemoveAll(this);
      _hasReservingNavAgents = false;
   }

   _navLinkOwnerComponent = navLinkOwner;

   if (UTATNavLinkOwnerComponent* component = _navLinkOwnerComponent.Get())
   {
      component->OnNavLinkReservationChanged.AddUObject(this, &UTATActionNodeComponent_IncorrectObjectState::_OnReservingNavAgentsChanged);
      _OnReservingNavAgentsChanged(component->GetNumberOfAgentsReservingNavLink());
   }
}

void UTATActionNodeComponent_IncorrectObjectState::AssignStimuliSourceComponent(UAIPerceptionStimuliSourceComponent* stimuliSource)
{
   _stimuliSourceComponent = stimuliSource;
}

void UTATActionNodeComponent_IncorrectObjectState::_OnIsBrokenChanged(bool isBroken)
{
   if (_isBroken != isBroken)
   {
      _isBroken = isBroken;
      _OnStateChanged();
   }
}

void UTATActionNodeComponent_IncorrectObjectState::_OnReservingNavAgentsChanged(int32 reservingAgentCount)
{
   const bool hasReservingAgents = reservingAgentCount > 0;
   if (_hasReservingNavAgents != hasReservingAgents)
   {
      _hasReservingNavAgents = hasReservingAgents;
      _OnStateChanged();
   }
}

void UTATActionNodeComponent_IncorrectObjectState::_OnStateChanged()
{
   if (!IsEnabled)
   {
      return;
   }
   if(IsBeingDestroyed())
   {
      // we're in the process of tearing down, ignore state changes.
      return;
   }
   const bool isInCorrectState = IsStateCorrect();
   _IsStateCorrect = isInCorrectState;

   if (GetOwner()->HasAuthority() == false)
   {
      return;
   }
   
   USmartObjectSubsystem* smartObjectSubsystem = GetWorld()->GetSubsystem<USmartObjectSubsystem>();
   if(smartObjectSubsystem == nullptr)
   {
      // We're likely tearing down if this is null.
      return;
   }
   if(_isBroken)
   {
      if(_abilitySystemComponent.IsValid())
      {
         _abilitySystemComponent->AddLooseGameplayTag(TAG_Status_Broken);
         _abilitySystemComponent->RemoveLooseGameplayTag(TAG_SmartObjects_Type_ResolveIncorrectState);
      }
      else
      {
         _gameplayTagCountContainer.SetTagCount(TAG_Status_Broken,1);
         _gameplayTagCountContainer.SetTagCount(TAG_SmartObjects_Type_ResolveIncorrectState,1);
      }
   }
   else
   {
      if(_IsStateCorrect)
      {
         if(_abilitySystemComponent.IsValid())
         {
            _abilitySystemComponent->RemoveLooseGameplayTag(TAG_SmartObjects_Type_ResolveIncorrectState);
         }
         else
         {
            _gameplayTagCountContainer.SetTagCount(TAG_SmartObjects_Type_ResolveIncorrectState,0);
         }
         smartObjectSubsystem->RemoveTagFromInstance(GetRegisteredHandle(), TAG_SmartObjects_Type_ResolveIncorrectState);
      }
      else
      {
         if(_abilitySystemComponent.IsValid())
         {
            _abilitySystemComponent->AddLooseGameplayTag(TAG_SmartObjects_Type_ResolveIncorrectState);
         }
         else
         {
            _gameplayTagCountContainer.SetTagCount(TAG_SmartObjects_Type_ResolveIncorrectState,1);
         }
         smartObjectSubsystem->AddTagToInstance(GetRegisteredHandle(), TAG_SmartObjects_Type_ResolveIncorrectState);
      }
   }
  
   // If the object is in the correct state, no reason that AI needs to see it.
   // Only make it visible if it is in an incorrect state that needs fixing.
   if (UAIPerceptionStimuliSourceComponent* component = _stimuliSourceComponent.Get())
   {
      if (_IsStateCorrect && _isBroken == false)
      {
         if(_registeredWithSense)
         {
            _registeredWithSense = false;
            component->UnregisterFromSense(UTATAISense_Sight::StaticClass());
         }
      }
      else
      {
         if(_registeredWithSense == false)
         {
            _registeredWithSense = true;
            component->RegisterForSense(UTATAISense_Sight::StaticClass());
         }
      }
   }
}
