// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Melee/TATGameplayAbility_DispatchMeleeAttack.h"

// tat
#include "Combat/TATCombatComponent.h"
#include "Combat/TATCombatFunctionLibrary.h"
#include "Tools/TATMeleeWeaponToolComponent.h"
#include "Tools/TATToolFunctionLibrary.h"
#include "Combat/TATCombatSettings.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Combat/CombatComponent.h"

// ue
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_DispatchMeleeAttack)

DEFINE_LOG_CATEGORY_STATIC(LogTATGameplayAbility_DispatchMeleeAttack, Log, All);

UTATGameplayAbility_DispatchMeleeAttack::UTATGameplayAbility_DispatchMeleeAttack()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

#if WITH_EDITOR
EDataValidationResult UTATGameplayAbility_DispatchMeleeAttack::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   // Validate light / charge attack dispatch info
   auto validateAttackDispatchInfo = [&context](const FTATMeleeAttackDispatchInfo& meleeAttackDispatchInfo, const FString& memberName)
   {
      if (!meleeAttackDispatchInfo.GameplayEvent.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("UTATGameplayAbility_DispatchMeleeAttack %s has invalid GameplayEvent!"), *memberName)));
      }
      if (!meleeAttackDispatchInfo.WeaponUsageTag.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("UTATGameplayAbility_DispatchMeleeAttack %s has invalid WeaponUsageTag!"), *memberName)));
      }
   };
   validateAttackDispatchInfo(_lightAttackDispatchInfo, GET_MEMBER_NAME_STRING_CHECKED(UTATGameplayAbility_DispatchMeleeAttack, _lightAttackDispatchInfo));
   validateAttackDispatchInfo(_chargeAttackDispatchInfo, GET_MEMBER_NAME_STRING_CHECKED(UTATGameplayAbility_DispatchMeleeAttack, _chargeAttackDispatchInfo));

   if (!_chargeAttackEarlyReleaseGameplayEvent.IsValid())
   {
      context.AddError(FText::FromString(TEXT("UTATGameplayAbility_DispatchMeleeAttack has unassigned _chargeAttackEarlyReleaseGameplayEvent!")));
   }

   return (context.GetNumErrors() + context.GetNumWarnings() == 0) ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif // WITH_EDITOR

void UTATGameplayAbility_DispatchMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, ownerInfo, activationInfo, triggerEventData);

   UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("ActivateAbility called..."));

   CommitAbility(handle, ownerInfo, activationInfo);

   _DetermineHoldTimer();

   _inputPressedTime = GetWorld()->GetTimeSeconds();
   _inputReleased  = false;

   // If on Ability Activation we cannot start a new attack, then bind to some events and spawn listeners to determine when we are able to
   if (!_CanStartNewCombatAttack(CurrentActorInfo))
   {
      UCombatComponent* combatComponent = UTATCombatFunctionLibrary::GetTATCombatComponent(CurrentActorInfo->AvatarActor.Get());
      if (ensure(combatComponent))
      {
         // Bind to combat animation can be interrupted and montage end, so that we can buffer our inputs to perform attacks immediately if needed
         combatComponent->OnCombatAnimationCanBeInterruptedEvent.AddDynamic(this, &UTATGameplayAbility_DispatchMeleeAttack::_OnCombatAnimationCanBeInterrupted);
         combatComponent->OnCombatAnimationMontageEndEvent.AddDynamic(this, &UTATGameplayAbility_DispatchMeleeAttack::_OnCombatMontageEnd);
      }

      if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo_Ensured())
      {
         const UTATCombatSettings& combatSettings = UTATCombatSettings::Get();
         asc->OnAbilityEnded.AddUObject(this, &UTATGameplayAbility_DispatchMeleeAttack::_OnCombatAbilityEnd);
      }
   }
}

void UTATGameplayAbility_DispatchMeleeAttack::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
   UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("EndAbility called..."));
   // Clear timers
   GetWorld()->GetTimerManager().ClearTimer(_pointOfNoReturnTimerHandle);
   GetWorld()->GetTimerManager().ClearTimer(_inputBufferTimerHandle);
   _takedownTarget = nullptr;

   _UnbindFromEventsAndDelegates();

   Super::EndAbility(handle, actorInfo, activationInfo, bReplicateEndAbility, bWasCancelled);
}

void UTATGameplayAbility_DispatchMeleeAttack::InputReleased(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo)
{
   Super::InputReleased(handle, actorInfo, activationInfo);

   UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("InputReleased called..."));

   GetWorld()->GetTimerManager().ClearTimer(_pointOfNoReturnTimerHandle);
   _pointOfNoReturnTimerHandle.Invalidate();

   const bool canPlayNewCombatAnimation = _CanStartNewCombatAttack(actorInfo);
   const bool isChargingMeleeAttack = UTATCombatFunctionLibrary::IsChargingMeleeAttack(actorInfo->AvatarActor.Get());
   if (canPlayNewCombatAnimation || isChargingMeleeAttack)
   {
      _DetermineAttack(actorInfo);
   }
   else
   {
      // Start a timer and wait to see if the current combat animation becomes interruptible
      UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("Combat Attack is currently ongoing, buffering input..."))
      GetWorld()->GetTimerManager().SetTimer(_inputBufferTimerHandle, this, &UTATGameplayAbility_DispatchMeleeAttack::_OnInputBufferTimeoutElapsed, _inputBufferTimeoutSeconds);
   }

   _inputReleased = true;
}

const TArray<UOSESyncedAnimationDataAsset*>& UTATGameplayAbility_DispatchMeleeAttack::GetTakedownAnimations() const
{
   return _GetMeleeToolComponentChecked()->TakedownAnimations;
}

void UTATGameplayAbility_DispatchMeleeAttack::_OnChargeAttackPointOfNoReturnElapsed()
{
   _pointOfNoReturnTimerHandle.Invalidate();
   if (_CanStartNewCombatAttack(CurrentActorInfo))
   {
      _PerformChargeAttack();
   }
}

void UTATGameplayAbility_DispatchMeleeAttack::_OnTakedownPointOfNoReturnElapsed()
{
   _pointOfNoReturnTimerHandle.Invalidate();
   if (_CanStartNewCombatAttack(CurrentActorInfo))
   {
      _PrepareTakedown();
   }
}

void UTATGameplayAbility_DispatchMeleeAttack::_OnInputBufferTimeoutElapsed()
{
   _inputBufferTimerHandle.Invalidate();
   // We failed to find an opportunity to launch an attack within the determined input buffer timeout, so just end this ability
   UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("InputBuffer timed out..."));
   constexpr bool replicateEndAbility = false;
   constexpr bool wasCancelled = false;
   EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, replicateEndAbility, wasCancelled);
}

void UTATGameplayAbility_DispatchMeleeAttack::_OnCombatMontageEnd()
{
   UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("OnCombatMontageEnd called..."));

   // The CombatMontage has ended, but the current active ability has not necessarily ended
   // Let's first check if we are able to start a new attack in this case
   if (_CanStartNewCombatAttack(CurrentActorInfo))
   {
      _StartNewCombatAttack(CurrentActorInfo);
   }
}

void UTATGameplayAbility_DispatchMeleeAttack::_OnCombatAnimationCanBeInterrupted()
{
   UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("OnCombatAnimationCanBeInterrupted called..."));

   // This is only called on abilities that have specific anim notifies to let us know we can cancel them
   // So if we hit here, we can just assume we can go straight into the next attack
   _StartNewCombatAttack(CurrentActorInfo);
}

void UTATGameplayAbility_DispatchMeleeAttack::_OnCombatAbilityEnd(const FAbilityEndedData& AbilityEndedData)
{
   UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("OnCombatAbilityEnd called..."));

   // When this is called, the current active ability has ended
   // However, the Montage the ability started may still have an uninterruptible animation ongoing
   // So first, check for that
   if (_CanStartNewCombatAttack(CurrentActorInfo))
   {
      _StartNewCombatAttack(CurrentActorInfo);
   }
}

void UTATGameplayAbility_DispatchMeleeAttack::_DetermineHoldTimer()
{
   // Set timer for charge-attack hold time
   if(_FindTakedownTarget())
   {
      GetWorld()->GetTimerManager().SetTimer(_pointOfNoReturnTimerHandle, this, &UTATGameplayAbility_DispatchMeleeAttack::_OnTakedownPointOfNoReturnElapsed, _takedownPointOfNoReturnHoldTimeSeconds);
   }
   else
   {
      GetWorld()->GetTimerManager().SetTimer(_pointOfNoReturnTimerHandle, this, &UTATGameplayAbility_DispatchMeleeAttack::_OnChargeAttackPointOfNoReturnElapsed, _chargeAttackPointOfNoReturnHoldTimeSeconds);
   }
}

void UTATGameplayAbility_DispatchMeleeAttack::_DetermineAttack(const FGameplayAbilityActorInfo* actorInfo)
{
   if (UTATCombatFunctionLibrary::IsChargingMeleeAttack(actorInfo->AvatarActor.Get()))
   {
      _takedownTarget = _FindTakedownTarget();
      if (_shouldAttemptTakedownWithChargedAttack && _takedownTarget != nullptr)
      {
         UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack,
            Verbose,
            TEXT("Charge attack input released - but we have a takedown target sending takedown event"));
         if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo_Ensured())
         {
            asc->CancelAbilities(&_abilityToCancelIfStoppingChargeAttackAndTakingDown);
         }
         PerformInstantTakedown();
      }
      else
      {
         UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("Charge attack input released! Sending swing event"));
         const float animReleaseTime = _QueryChargeAttackAnimMontagePosition();

         // Activate ability to notify charge attack ability of release (passing along anim montage position at time of release)
         if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo_Ensured())
         {
            FGameplayEventData payloadData;
            payloadData.EventMagnitude = animReleaseTime;
            asc->HandleGameplayEvent(_chargeAttackEarlyReleaseGameplayEvent, &payloadData);
         }
      }
   }
   else
   {
      _PerformLightAttack(actorInfo);
   }

   constexpr bool replicateEndAbility = false;
   constexpr bool wasCancelled = false;
   EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, replicateEndAbility, wasCancelled);
}

void UTATGameplayAbility_DispatchMeleeAttack::_PerformLightAttack(const FGameplayAbilityActorInfo* actorInfo)
{
   UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("PerformLightAttack called..."));
   check(actorInfo);
   const UTATCombatComponent* combatComponent = UTATCombatFunctionLibrary::GetTATCombatComponent(actorInfo->AvatarActor.Get());
   if (!combatComponent)
   {
      UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Error, TEXT("Failed to retrieve UTATCombatComponent from owner character (%s)!")
         , actorInfo->AvatarActor.IsValid() ? *actorInfo->AvatarActor->GetName() : TEXT("NONE"));
      return;
   }

   const UTATMeleeWeaponToolComponent* meleeWeaponToolComponent = _GetMeleeToolComponentChecked();
   if (const FTATMeleeWeaponAttack* attack = meleeWeaponToolComponent->GetMeleeWeaponAttackForUsage(_lightAttackDispatchInfo.WeaponUsageTag))
   {
      int32 poolIndex = INDEX_NONE;
      if (attack->IsChainAttack)
      {
         poolIndex = combatComponent->GetNextChainAttackIndex();
      }
      else
      {
         poolIndex = _lightAttackDispatchInfo.NextSequentialAttackPoolIndex;
      }
      UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("Performing light attack (%s index = %d)...")
         , attack->IsChainAttack ? TEXT("chain attack") : TEXT("attack pool")
         , poolIndex);
      _PerformAttack(_lightAttackDispatchInfo, poolIndex);
   }
}

void UTATGameplayAbility_DispatchMeleeAttack::_PerformChargeAttack()
{
   UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Verbose, TEXT("Performing charge attack..."));

   const int32 poolIndex = _chargeAttackDispatchInfo.NextSequentialAttackPoolIndex;
   const bool success = _PerformAttack(_chargeAttackDispatchInfo, poolIndex);
   if (!success)
   {
      constexpr bool replicateEndAbility = false;
      constexpr bool wasCancelled = false;
      EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, replicateEndAbility, wasCancelled);
   }
}

bool UTATGameplayAbility_DispatchMeleeAttack::_PerformAttack(FTATMeleeAttackDispatchInfo& attackDispatchInfo, int32 poolIndex)
{
   if (!attackDispatchInfo.WeaponUsageTag.IsValid() || !attackDispatchInfo.GameplayEvent.IsValid())
   {
      UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Error, TEXT("_PerformAttack() called with invalid FTATMeleeAttackDispatchInfo!"));
      return false;
   }
   
   // Find attack with provided usage tag
   const UTATMeleeWeaponToolComponent* meleeWeaponToolComponent = _GetMeleeToolComponentChecked();
   const FTATMeleeWeaponAttack* attack = meleeWeaponToolComponent->GetMeleeWeaponAttackForUsage(attackDispatchInfo.WeaponUsageTag);
   if (!attack)
   {
      UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Error, TEXT("_PerformAttack() | could not find attack on tool %s with UsageTag = %s")
         , *meleeWeaponToolComponent->GetName()
         , *attackDispatchInfo.WeaponUsageTag.ToString());
      return false;
   }

   if (!attack->SequentialAttackPools.IsValidIndex(poolIndex))
   {
      UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Error, TEXT("_PerformAttack() called with invalid poolIndex %d (SequentialAttackPools size = %d)! Selecting a random montage!"), poolIndex, attack->SequentialAttackPools.Num());
      poolIndex = FMath::RandHelper(attack->SequentialAttackPools.Num());
   }

   // Deliberately skip usage of nextPoolIndex - montage index is either arbitrary or tied to chain attack index
   const UAnimMontage* animMontage = UTATToolFunctionLibrary::ChooseRandomMontageForAttack(*attack, poolIndex, attackDispatchInfo.NextSequentialAttackPoolIndex);
   if (!animMontage)
   {
      UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Error, TEXT("Failed to select AnimMontage for attack using index %d!"), poolIndex);
      return false;
   }

   if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo_Ensured())
   {
      if(!_cancelableAttackTags.IsEmpty())
      {
         asc->CancelAbilities(&_cancelableAttackTags);
      }

      FGameplayEventData payloadData;
      if (attack->IsChainAttack)
      {
         payloadData.EventMagnitude = poolIndex;
      }
      payloadData.OptionalObject = animMontage;
      payloadData.InstigatorTags.AddTag(attackDispatchInfo.WeaponUsageTag);

      FScopedPredictionWindow newScopedWindow(asc, true);
      const int32 activationCount = asc->HandleGameplayEvent(attackDispatchInfo.GameplayEvent, &payloadData);
      if (activationCount == 0)
      {
         UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Display, TEXT("Didn't activate any abilities for attack (probably stamina)"), activationCount);
         return false;
      }
   }

   return true;
}

bool UTATGameplayAbility_DispatchMeleeAttack::_CanCombatBeInterrupted(const FGameplayAbilityActorInfo* actorInfo) const
{
   const AOSECharacterBase* character = Cast<AOSECharacterBase>(actorInfo->AvatarActor.Get());
   if (character == nullptr)
   {
      return false;
   }

   const UCombatComponent* combatComponent = character->GetCombatComponent();
   check(combatComponent);

   return !combatComponent->HasUninterruptibleCombatAnimation();
}

bool UTATGameplayAbility_DispatchMeleeAttack::_CanStartNewCombatAttack(const FGameplayAbilityActorInfo* actorInfo) const
{
   // First, we check if there are any ongoing animations that can be cancelled
   UCombatComponent* combatComponent = UTATCombatFunctionLibrary::GetTATCombatComponent(CurrentActorInfo->AvatarActor.Get());
   if (ensure(combatComponent))
   {
      if (combatComponent->CanCombatAnimationBeInterrupted())
      {
         // If we're currently in a combat animation that has entered it's cancelable frames, we're free to start a new attack!
         return true;
      }
   }

   // The above will return false if there is no ongoing animation to be cancelled
   // However in those cases we still need to check if there is currently an ongoing combat attack ability that has yet to start an animation
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo_Ensured())
   {
      const UTATCombatSettings& combatSettings = UTATCombatSettings::Get();
      if (asc->HasMatchingGameplayTag(combatSettings.IsPerformingMeleeAttackTag))
      {
         // If there is no interruptible animation, but there is still an ongoing combat attack, we cannot start a new attack
         return false;
      }
   }

   // No interruptible animation and no ongoing combat attack means we're free to start a new attack!
   return true;
}

void UTATGameplayAbility_DispatchMeleeAttack::_PrepareTakedown()
{
   _takedownTarget = _FindTakedownTarget();

   // Start winding up takedown if target still in range
   if(_takedownTarget)
   {
      WaitForTakedown();
   }
   else
   {
      // Otherwise end without dispatching anything (to avoid accidentally breaking stealth)
      constexpr bool replicateEndAbility = false;
      constexpr bool wasCancelled = false;
      EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, replicateEndAbility, wasCancelled);
   }
}

void UTATGameplayAbility_DispatchMeleeAttack::_StartNewCombatAttack(const FGameplayAbilityActorInfo* actorInfo)
{
   // If we have reached here it is because we know we are able to start a new attack...
   // either because there is no current attack ongoing anymore, or the current attack is interruptible
   // Since we have already confirmed a new combat attack is allowed to start...
   // we should cancel all the delegates and listeners that are waiting to see when it is appropriate to start a new attack
   // That way we're not running this code more than once.
   _UnbindFromEventsAndDelegates();

   // If we have already released the input when we hit here, then we have buffered it, so time to follow through
   if (_inputReleased)
   {
      GetWorld()->GetTimerManager().ClearTimer(_inputBufferTimerHandle);
      _inputBufferTimerHandle.Invalidate();
      _DetermineAttack(CurrentActorInfo);
   }
   // If this is no longer valid, then the hold time has elapsed while we were waiting to be allowed to start a new attack
   else if (!_pointOfNoReturnTimerHandle.IsValid())
   {
      if (_FindTakedownTarget())
      {
         _PrepareTakedown();
      }
      else
      {
         _PerformChargeAttack();
      }
   }

   // If the input is still held down and the _pointOfNoReturnTimerHandle is still valid...
   // then do nothing here and just wait for the timer to elapse and let the input release event handle it
}

AActor* UTATGameplayAbility_DispatchMeleeAttack::_FindTakedownTarget() const
{
   const UTATMeleeWeaponToolComponent* meleeWeaponTool = _GetMeleeToolComponentChecked();
   if(!meleeWeaponTool->CanBeUsedForStealthTakedowns)
   {
      return nullptr;
   }

   AActor* avatar = GetAvatarActorFromActorInfo();
   return UTATCombatFunctionLibrary::FindStealthTakedownTarget(avatar, meleeWeaponTool->TakedownAnimations, _takedownTraceDistance, _takedownTraceHalfAngle, _takedownTraceProfile);
}

float UTATGameplayAbility_DispatchMeleeAttack::_QueryChargeAttackAnimMontagePosition() const
{
   const ACharacter* character = Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get());
   if (!character)
   {
      UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Error, TEXT("_QueryChargeAttackAnimationHoldTime() failed to find owning character for player (%s)")
         , *GetNameSafe(CurrentActorInfo->OwnerActor.Get()));
      return 0.f;
   }

   // Get currently-playing montage
   const USkeletalMeshComponent* skeletalMesh = character->GetMesh();
   check(skeletalMesh);
   const UAnimInstance* animInstance = skeletalMesh->GetAnimInstance();
   check(animInstance);
   const UAnimMontage* montage = animInstance->GetCurrentActiveMontage();
   if (!montage)
   {
      UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Error, TEXT("_QueryChargeAttackAnimMontagePosition() | No montage playing"));
      return 0.f;
   }

   // Make sure montage currently playing is one of the defined charge attack ones
   const UTATMeleeWeaponToolComponent* meleeTool = _GetMeleeToolComponentChecked();
   const FTATMeleeWeaponAttack* chargeAttack = meleeTool->GetMeleeWeaponAttackForUsage(_chargeAttackDispatchInfo.WeaponUsageTag);
   if (chargeAttack)
   {
      const bool isAttackMontage = chargeAttack->SequentialAttackPools.ContainsByPredicate([&](const FTATMeleeWeaponAttackAnimationPool& attackAnimPool)
         { return attackAnimPool.Animations.Contains(montage); });
      if (!isAttackMontage)
      {
         UE_LOG(LogTATGameplayAbility_DispatchMeleeAttack, Error, TEXT("_QueryChargeAttackAnimMontagePosition() | Unexpected montage %s playing (not found in %s attack animation pool)")
            , *montage->GetName()
            , *_chargeAttackDispatchInfo.WeaponUsageTag.ToString());
         return 0.f;
      }
   }

   return animInstance->Montage_GetPosition(montage);
}

void UTATGameplayAbility_DispatchMeleeAttack::_UnbindFromEventsAndDelegates()
{
   // Unbind from combat component events
   UCombatComponent* combatComponent = UTATCombatFunctionLibrary::GetTATCombatComponent(CurrentActorInfo->AvatarActor.Get());
   if (ensure(combatComponent))
   {
      combatComponent->OnCombatAnimationCanBeInterruptedEvent.RemoveAll(this);
      combatComponent->OnCombatAnimationMontageEndEvent.RemoveAll(this);
   }

   if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo_Ensured())
   {
      asc->OnAbilityEnded.RemoveAll(this);
   }
}
