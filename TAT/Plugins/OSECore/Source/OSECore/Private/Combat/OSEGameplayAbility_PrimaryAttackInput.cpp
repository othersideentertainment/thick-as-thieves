// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/OSEGameplayAbility_PrimaryAttackInput.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Combat/CombatComponent.h"
#include "Combat/OSEGameplayAbility_MeleeAttack.h"

// ue5
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_PrimaryAttackInput)


DEFINE_LOG_CATEGORY_STATIC(LogOSEGameplayAbility_PrimaryAttackInput, Log, All);

namespace PrimaryAttackInputCVars
{
   static int AllowInputBuffering = 1;
   FAutoConsoleVariableRef CVarAllowInputBuffering(
      TEXT("OSE.Combat.AllowInputBuffering"),
      AllowInputBuffering,
      TEXT("Allow input buffering?"),
      ECVF_Default);
}

UOSEGameplayAbility_PrimaryAttackInput::UOSEGameplayAbility_PrimaryAttackInput()
{
   // instanced per actor to keep track of activation timing; local-only because this is an input-only ability that triggers networked abilities.
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UOSEGameplayAbility_PrimaryAttackInput::CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags, const FGameplayTagContainer* targetTags, FGameplayTagContainer* optionalRelevantTags) const
{
   const bool canActivateBase = Super::CanActivateAbility(handle, actorInfo, sourceTags, targetTags, optionalRelevantTags);
   return canActivateBase;
}

void UOSEGameplayAbility_PrimaryAttackInput::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);

   UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: ActivateAbility"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));

   // commit
   if (!CommitAbility(handle, actorInfo, activationInfo))
   {
      _EndAbility(true);
      return;
   }

   UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: Committed"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));

   const float now = GetWorld()->GetTimeSeconds();

   // drop our combo chains if it's been a while since we've received input
   if (_lastWorldTime != float(INDEX_NONE))
   {
      const float elapsedTime = now - _lastWorldTime;
      if (elapsedTime >= ComboDropInputTime)
      {
         _lightAttackComboIndex = 0;
         _heavyAttackComboIndex = 0;
         _lightAttackPoolIndex = 0;
         _heavyAttackPoolIndex = 0;
      }
   }
   else
   {
      _lightAttackComboIndex = 0;
      _heavyAttackComboIndex = 0;
      _lightAttackPoolIndex = 0;
      _heavyAttackPoolIndex = 0;
   }

   // random heavy attacks
   switch(HeavyAttackRandomizationType)
   {
   case EPrimaryAttackRandomizationType::RandomAny:
      {
         _heavyAttackComboIndex = FMath::RandHelper(HeavyAttackAbilities.Num());
      }
      break;
   case EPrimaryAttackRandomizationType::RandomCycleThroughPools:
      {
         _heavyAttackComboIndex = _TryFindNextPooledAbilityIndex(HeavyAttackAbilities, HeavyAttackPools, _heavyAttackPoolIndex);
         if (_heavyAttackComboIndex == INDEX_NONE)
         {
            // just always do zero so we see there's something wrong visually...?
            _heavyAttackComboIndex = 0;
         }
      }
      break;
   }

   // random light attacks
   switch (LightAttackRandomizationType)
   {
   case EPrimaryAttackRandomizationType::RandomAny:
      {
         _lightAttackComboIndex = FMath::RandHelper(LightAttackAbilities.Num());
      }
      break;
   case EPrimaryAttackRandomizationType::RandomCycleThroughPools:
      {
         _lightAttackComboIndex = _TryFindNextPooledAbilityIndex(LightAttackAbilities, LightAttackPools, _lightAttackPoolIndex);
         if (_lightAttackComboIndex == INDEX_NONE)
         {
            // just always do zero so we see there's something wrong visually...?
            _lightAttackComboIndex = 0;
         }
      }
      break;
   }

   // we can start a wind-up animation at this point, local-only animation, something that can blend into both our heavy and light attacks
   OnPlayWindup();

   // local-only ability allows us to use a simple local world-time callback
   UAbilitySystemComponent& asc = _GetAbilitySystemComponent();
   FGameplayAbilitySpec* spec = asc.FindAbilitySpecFromHandle(handle);
   if (PrimaryAttackInputCVars::AllowInputBuffering)
   {      
      // bind to the combat component so we can auto-chain attacks
      UCombatComponent& combatComponent = _GetCombatComponent();
      combatComponent.OnCombatAnimationCanBeInterruptedEvent.AddUniqueDynamic(this, &UOSEGameplayAbility_PrimaryAttackInput::_OnCombatAnimationCanBeInterrupted);

      if (spec && spec->InputPressed)
      {
         // still holding input? could be a heavy attack, start the timer and wait for the release
         GetWorld()->GetTimerManager().SetTimer(_timerHandle_OnHeavyAttackPointOfNoReturnReached, this, &UOSEGameplayAbility_PrimaryAttackInput::_OnHeavyAttackPointOfNoReturnReached, HeavyAttackPointOfNoReturnHoldTime, false);
      }
      else
      {
         // input already released?  it's a light attack starting right now
         _DoLightAttack();
      }
   }
   else
   {
      GetWorld()->GetTimerManager().SetTimer(_timerHandle_OnHeavyAttackPointOfNoReturnReached, this, &UOSEGameplayAbility_PrimaryAttackInput::_OnHeavyAttackPointOfNoReturnReached, HeavyAttackPointOfNoReturnHoldTime, false);
   }
}

void UOSEGameplayAbility_PrimaryAttackInput::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled)
{
   Super::EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);

   UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: EndAbility"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));

   UAbilitySystemComponent& asc = _GetAbilitySystemComponent();

   // reset per-activation state
   GetWorld()->GetTimerManager().ClearTimer(_timerHandle_OnHeavyAttackPointOfNoReturnReached);
   asc.AbilityEndedCallbacks.RemoveAll(this);
   _triggeredAbility = nullptr;
   _state = State::WaitingForInputRelease;
   _chainAttackAttemptTime = float(INDEX_NONE);

   // unbind from the combat component
   if (PrimaryAttackInputCVars::AllowInputBuffering)
   {
      UCombatComponent& combatComponent = _GetCombatComponent();
      combatComponent.OnCombatAnimationCanBeInterruptedEvent.RemoveAll(this);
   }

   // mark the time of this last input on successful executions
   if (wasCancelled)
   {
      _lastWorldTime = float(INDEX_NONE);
   }
   else
   {
      _lastWorldTime = GetWorld()->GetTimeSeconds();
   }

   // support re-triggering ourselves if we're properly ending and still holding down an input button
   // Enabling this enables two interesting behaviors:
   // - triggering a light attack then holding down the button immediately after will launch us into a heavy attack
   // - triggering a heavy attack that lands on a target, which does NOT cause overextended, and then continuing to follow-up with a heavy attack
   if (PrimaryAttackInputCVars::AllowInputBuffering && AllowRetriggeringOnEndAbility)
   {
      FGameplayAbilitySpec* spec = asc.FindAbilitySpecFromHandle(handle);
      if(spec && spec->InputPressed && !wasCancelled)
      {
         UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: EndAbility re-activating ourselves due to held input button"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));

         // re-activate ourselves
         asc.TryActivateAbilityByClass(GetClass());
      }
   }

   // for external systems
   OnCombatAttackEnded.Broadcast();
}

void UOSEGameplayAbility_PrimaryAttackInput::InputPressed(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo)
{
   Super::InputPressed(handle, actorInfo, activationInfo);

   if (_state == State::WaitingForTriggeredAbilityEnd)
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: InputPressed in WaitingForTriggeredAbilityEnd state"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));

      // we triggered an attack and we're waiting for it to end.
      // if the combat component says it can be interrupted we can cancel it now and initiate another attack
      // if we cannot do that right now, we can chain the next attack when the current one is complete
      if (!_TryCancelCurrentAttackAndStartNext())
      {
         _chainAttackAttemptTime = GetWorld()->GetTimeSeconds();
      }
   }
   else
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: InputPressed in incorrect state"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));
   }
}

void UOSEGameplayAbility_PrimaryAttackInput::InputReleased(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo)
{
   Super::InputReleased(handle, actorInfo, activationInfo);

   if (_state == State::WaitingForInputRelease)
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: InputReleased in correct state"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));

      // light attack if we've released before the point of no return
      _state = State::AttackTriggered;
      _DoLightAttack();
   }
}

void UOSEGameplayAbility_PrimaryAttackInput::_OnHeavyAttackPointOfNoReturnReached()
{
   // clear timer
   GetWorld()->GetTimerManager().ClearTimer(_timerHandle_OnHeavyAttackPointOfNoReturnReached);

   // ignore this callback if we've moved along the state machine; we've already decided we're not doing a heavy attack
   if (_state == State::WaitingForInputRelease)
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: _OnHeavyAttackPointOfNoReturnReached"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));

      OnHeavyAttackPointOfNoReturnReached();

      // we are doing a heavy attack and there is no canceling it out at this point
      _state = State::AttackTriggered;
      _DoHeavyAttack();

      // for external systems
      OnHeavyAttackPointOfNoReturnReachedEvent.Broadcast();
   }
}

void UOSEGameplayAbility_PrimaryAttackInput::_DoLightAttack()
{
   _DoAttack(LightAttackAbilities, _lightAttackComboIndex, _heavyAttackComboIndex, false);
}

void UOSEGameplayAbility_PrimaryAttackInput::_DoHeavyAttack()
{
   if (HeavyAttackAbilities.Num() > 0)
      _DoAttack(HeavyAttackAbilities, _heavyAttackComboIndex, _lightAttackComboIndex, true);
   else
      _DoAttack(LightAttackAbilities, _lightAttackComboIndex, _heavyAttackComboIndex, false);
}

void UOSEGameplayAbility_PrimaryAttackInput::_DoAttack(const TArray<TSubclassOf<UOSEGameplayAbility_MeleeAttack>>& abilities, int& index, int& otherIndex, bool isHeavyAttack)
{
   UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: _DoAttack"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));

   UAbilitySystemComponent& asc = _GetAbilitySystemComponent();

   // trigger our attack
   check(abilities.IsValidIndex(index));

   TSubclassOf<UOSEGameplayAbility_MeleeAttack> abilityClass = abilities[index];
   if (!abilityClass)
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Error, TEXT("Failed to find ability class!"));
      _EndAbility(true);
      return;
   }

   // activate; this generates a prediction key
   if (!asc.TryActivateAbilityByClass(abilityClass, true))
   {
      // failed to activate!
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Error, TEXT("Failed to activate ability class %s!"), *abilityClass->GetName());
      _EndAbility(true);
      return;
   }

   // ensure we can actually find the spec so we can listen for it's completion
   FGameplayAbilitySpec* spec = asc.FindAbilitySpecFromClass(abilityClass);
   if (!spec)
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Error, TEXT("Failed to trigger ability class %s!"), *abilityClass->GetName());
      _EndAbility(true);
      return;
   }

   // with a spec we can wait for it to complete
   _triggeredAbility = Cast<UOSEGameplayAbility_MeleeAttack>(spec->Ability);
   if (!_triggeredAbility)
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Error, TEXT("Failed to determine ability from spec!"));
      _EndAbility(true);
      return;
   }

   if (isHeavyAttack)
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: OnHeavyAttackTriggered"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));
      OnHeavyAttackTriggered(_triggeredAbility);
   }
   else
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: OnLightAttackTriggered"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));
      OnLightAttackTriggered(_triggeredAbility);
   }

   _state = State::WaitingForTriggeredAbilityEnd;
   asc.AbilityEndedCallbacks.AddUObject(this, &UOSEGameplayAbility_PrimaryAttackInput::_OnAbilityEnd);

   // drop the other attack type combo (light drops heavy and vice-versa)
   otherIndex = 0;

   // increment this attack type combo for the next attack
   index++;
   if (index >= abilities.Num())
      index = 0;
}

void UOSEGameplayAbility_PrimaryAttackInput::_EndAbility(bool wasCanceled)
{
   bool replicateEndAbility = false;
   EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), replicateEndAbility, wasCanceled);
}

void UOSEGameplayAbility_PrimaryAttackInput::_OnAbilityEnd(UGameplayAbility* ability)
{
   UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: _OnAbilityEnd %s"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()), ability ? *ability->GetName() : TEXT("nullptr"));

   if (_state == State::WaitingForTriggeredAbilityEnd)
   {
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: _OnAbilityEnd in correct state"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));

      // only bound once we know our triggered ability
      check(_triggeredAbility);

      // we can end once our triggered ability ends
      // TODO: This doesn't happen predictively does it...
      if (ability && (_triggeredAbility->GetClass() == ability->GetClass()))
      {
         // additionally removing this callback here so our own end ability doesn't call this back (for logging/sanity sake)
         UAbilitySystemComponent& asc = _GetAbilitySystemComponent();
         asc.AbilityEndedCallbacks.RemoveAll(this);

         _EndAbility(false);
      }
   }
}

void UOSEGameplayAbility_PrimaryAttackInput::_OnCombatAnimationCanBeInterrupted()
{
   ensure(PrimaryAttackInputCVars::AllowInputBuffering);
   if (_triggeredAbility && _chainAttackAttemptTime != float(INDEX_NONE))
   {
      const float now = GetWorld()->GetTimeSeconds();
      const float chainInputSecondsAgo = now - _chainAttackAttemptTime;
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: _OnCombatAnimationCanBeInterrupted %.04f seconds ago"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()), chainInputSecondsAgo);

      if (chainInputSecondsAgo < _triggeredAbility->GetChainAttackInputBufferWindow())
      {
         _TryCancelCurrentAttackAndStartNext();
      }

      // consumed
      _chainAttackAttemptTime = float(INDEX_NONE);
   }

   // for external systems
   OnCombatAnimationCanBeInterrupted.Broadcast();
}

bool UOSEGameplayAbility_PrimaryAttackInput::_TryCancelCurrentAttackAndStartNext()
{
   UCombatComponent& combatComponent = _GetCombatComponent();
   if (_triggeredAbility && combatComponent.CanCombatAnimationBeInterrupted())
   {
      UAbilitySystemComponent& asc = _GetAbilitySystemComponent();

      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: _TryCancelTriggeredAbilityAndReActivateSelf interrupting %s"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()), *_triggeredAbility->GetName());

      // cancel
      asc.CancelAbility(_triggeredAbility);

      // re-activate ourselves
      asc.TryActivateAbilityByClass(GetClass());

      // handled!
      return true;
   }
   else
   {
      // we pressed the button too early to interrupt, the animation is still playing and not ready to be interrupted yet
      UE_LOG(LogOSEGameplayAbility_PrimaryAttackInput, Verbose, TEXT("%s: _TryCancelTriggeredAbilityAndReActivateSelf cannot interrupt right now!"), *AActor::GetDebugName(GetActorInfo().AvatarActor.Get()));
   }

   // can't handle it!
   return false;
}

int UOSEGameplayAbility_PrimaryAttackInput::_TryFindNextPooledAbilityIndex(const TArray<TSubclassOf<UOSEGameplayAbility_MeleeAttack>>& allAbilities, const TArray<FPrimaryAttackPool>& pools, int& index)
{
   if (!pools.IsValidIndex(index))
      return INDEX_NONE;

   const FPrimaryAttackPool& pool = pools[index];
   const int numPossibleAbilities = pool.AttackAbilities.Num();
   if (numPossibleAbilities == 0)
      return INDEX_NONE;

   TSubclassOf<UOSEGameplayAbility_MeleeAttack> attackAbility = pool.AttackAbilities[FMath::RandHelper(numPossibleAbilities)];
   if (!attackAbility.Get())
      return INDEX_NONE;

   // increment for next time
   index++;
   if (!pools.IsValidIndex(index))
   {
      index = 0;
   }

   // need to work in the space of the big abilities array because that's the space the rest of the class operates in
   return allAbilities.Find(attackAbility);
}

AOSECharacterBase& UOSEGameplayAbility_PrimaryAttackInput::_GetCharacter() const
{
   const FGameplayAbilityActorInfo* actorInfo = GetCurrentActorInfo();
   check(actorInfo);
   return *CastChecked<AOSECharacterBase>(actorInfo->AvatarActor.Get());
}

UAbilitySystemComponent& UOSEGameplayAbility_PrimaryAttackInput::_GetAbilitySystemComponent()
{
   const FGameplayAbilityActorInfo* actorInfo = GetCurrentActorInfo();
   check(actorInfo);
   UAbilitySystemComponent* asc = actorInfo->AbilitySystemComponent.Get();
   check(asc);
   return *asc;
}

UCombatComponent& UOSEGameplayAbility_PrimaryAttackInput::_GetCombatComponent() const
{
   AOSECharacterBase& character = _GetCharacter();
   UCombatComponent* combatComp = character.GetCombatComponent();
   check(combatComp);
   return *combatComp;
}

