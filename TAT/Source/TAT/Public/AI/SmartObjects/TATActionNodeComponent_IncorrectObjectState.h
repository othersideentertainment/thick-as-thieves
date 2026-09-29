// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/SmartObjects/TATActionNodeComponent.h"

// ue
#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"

#include "TATActionNodeComponent_IncorrectObjectState.generated.h"

class UAIPerceptionStimuliSourceComponent;
class UTATBreakableComponent;
class UTATNavLinkOwnerComponent;

// Smart object component that tracks "incorrect" states on the owning actor and manages
// several optional, linked components based on the "correct"/"incorrect" state.
UCLASS()
class TAT_API UTATActionNodeComponent_IncorrectObjectState : public UTATActionNodeComponent
{
   GENERATED_BODY()

public:
   virtual void InitializeComponent() override;
   // from UActorComponent
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   void SetInitialState(int32 correct, int32 current);
   void SetCurrentState(int32 state);
   bool IsStateCorrect(const bool allowIgnoringOfState = true) const;

   // Bind to a breakable component so that the "broken" state can impact the "incorrect"
   // object state. If the actor is broken, we can consider it not in an "incorrect" state
   // as there is nothing an AI could do to correct its state.
   void AssignBreakableComponent(UTATBreakableComponent* breakable);

   // Bind to a nav-link owning actor so that any AI are currently planning to traverse
   // will affect the "incorrect" object state. Open doors can be considered an "incorrect"
   // state for doors but AI need to open/close them in order to traverse through. To prevent
   // the door being considered in an "incorrect" state when opened to traverse, we ignore
   // the "incorrect" state if any AI is traversing.
   void AssignNavLinkOwnerComponent(UTATNavLinkOwnerComponent* navLinkOwner);

   // If assigned, registration of the sight sense will be automatically enabled/disabled
   // depending on the correct/incorrect state of this component.
   // If the actor should be visible for more cases than just being in an incorrect state,
   // the owning actor should handle this functionality themselves.
   void AssignStimuliSourceComponent(UAIPerceptionStimuliSourceComponent* stimuliSource);

   // A passthrough for ITATSmartObjectTagInterface::GetGameplayTagCountContainer().
   // Will have the incorrect state tag added when necessary.
   FGameplayTagCountContainer& GetIncorrectStateTags() { return _gameplayTagCountContainer; }

   // If set to false, disable this smart object component for use (regardless of state).
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool IsEnabled = true;

   // If true, will not unregister the stimuli source component when the owning actor
   // is broken (which would otherwise, as IsInCorrectState() would return true).
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool ShouldBeVisibleWhenBroken = true;

private:
   UFUNCTION()
   void _OnIsBrokenChanged(bool isBroken);

   UFUNCTION()
   void _OnReservingNavAgentsChanged(int32 reservingAgentCount);

   // Defines conditions by which the component's current state is ignored.
   // (for example, if the actor is broken, there's nothing AI can do to "correct" it)
   bool _ShouldIgnoreIncorrectState() const;
   bool _IsEnabled() const { return IsEnabled; }

   void _OnStateChanged();

   // Local cache of the incorrect state status. Useful to compare when individual
   // pieces influencing the state change but may not actually change the overall state.
   bool _IsStateCorrect = true;

   // State is cached as an integer to encapsulate usage of simple bools (as 0 and 1)
   // and enumerations when tracking states.
   int32 _correctState = 0;
   int32 _currentState = 0;

   FGameplayTagCountContainer _gameplayTagCountContainer;

   TWeakObjectPtr<UAbilitySystemComponent> _abilitySystemComponent = nullptr;
   bool _registeredWithSense = false;
   
   TWeakObjectPtr<UTATBreakableComponent> _breakableComponent = nullptr;
   bool _isBroken = false;

   TWeakObjectPtr<UTATNavLinkOwnerComponent> _navLinkOwnerComponent = nullptr;
   bool _hasReservingNavAgents = false;

   TWeakObjectPtr<UAIPerceptionStimuliSourceComponent> _stimuliSourceComponent = nullptr;
	
};
