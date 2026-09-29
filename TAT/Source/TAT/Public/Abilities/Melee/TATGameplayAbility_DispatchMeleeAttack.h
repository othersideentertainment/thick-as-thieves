// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/OSEGameplayAbility.h"

// ue
#include "GameplayTagContainer.h"

#include "TATGameplayAbility_DispatchMeleeAttack.generated.h"

class UOSESyncedAnimationDataAsset;
class UTATMeleeWeaponToolComponent;

USTRUCT()
struct FTATMeleeAttackDispatchInfo
{
   GENERATED_BODY()

public:
   // Corresponds to an FTATMeleeWeaponAttack entry of equipped TATMeleeWeaponToolComponent
   UPROPERTY(EditDefaultsOnly, meta = (Categories = "Tool.Usage"))
   FGameplayTag WeaponUsageTag;

   // Gameplay event to fire when used (should trigger an attack ability granted by equipped TATMeleeWeaponToolComponent)
   UPROPERTY(EditDefaultsOnly, meta = (Categories = "Combat.Ability.Attack"))
   FGameplayTag GameplayEvent;

   // Used for selecting pool to pull attack anim montage (except for chain attacks, where we iterate over pools sequentially)
   int32 NextSequentialAttackPoolIndex = 0;
};

/// Local-only ability that interprets triggering input as a press / hold and activates a corresponding melee weapon attack ability.
/// Runs until input is released, or combat montage ends (whichever happens first).
/// Should only be granted by an equipped UTATMeleeWeaponToolComponent!
UCLASS()
class TAT_API UTATGameplayAbility_DispatchMeleeAttack : public UOSEGameplayAbility
{
   GENERATED_BODY()

   UTATGameplayAbility_DispatchMeleeAttack();

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   // From UGameplayAbility
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
   virtual void InputReleased(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) override;

protected:
   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
   void PerformInstantTakedown();

   UFUNCTION(BlueprintImplementableEvent)
   void WaitForTakedown();

   UFUNCTION(BlueprintPure)
   const TArray<UOSESyncedAnimationDataAsset*>& GetTakedownAnimations() const;

   UFUNCTION(BlueprintPure)
   float GetNetTakedownHoldDuration() const
   {
      return _takedownHoldDuration - _takedownPointOfNoReturnHoldTimeSeconds;
   }
   
   UPROPERTY(Transient, BlueprintReadOnly)
   AActor* _takedownTarget { nullptr };
   
private:
   void _OnChargeAttackPointOfNoReturnElapsed();
   void _OnTakedownPointOfNoReturnElapsed();
   void _OnInputBufferTimeoutElapsed();
   UFUNCTION()
   void _OnCombatMontageEnd();
   UFUNCTION()
   void _OnCombatAnimationCanBeInterrupted();
   UFUNCTION()
   void _OnCombatAbilityEnd(const FAbilityEndedData& AbilityEndedData);

   void _DetermineHoldTimer();
   void _DetermineAttack(const FGameplayAbilityActorInfo* actorInfo);
   void _PerformLightAttack(const FGameplayAbilityActorInfo* actorInfo);
   void _PerformChargeAttack();

   /// Attempts to activate the attack ability (passing in corresponding anim montage), returning activation success
   bool _PerformAttack(FTATMeleeAttackDispatchInfo& attackDispatchInfo, int32 attackMontageIndex);
   bool _CanCombatBeInterrupted(const FGameplayAbilityActorInfo* actorInfo) const;
   bool _CanStartNewCombatAttack(const FGameplayAbilityActorInfo* actorInfo) const;
   void _PrepareTakedown();
   void _StartNewCombatAttack(const FGameplayAbilityActorInfo* actorInfo);

   AActor* _FindTakedownTarget() const;

   /// Returns position of currently-playing charge attack montage. If called while an unexpected montage is playing, returns 0
   float _QueryChargeAttackAnimMontagePosition() const;

   void _UnbindFromEventsAndDelegates();

   FORCEINLINE const UTATMeleeWeaponToolComponent* _GetMeleeToolComponentChecked() const
   {
      const UObject* sourceObject = GetCurrentSourceObject();
      check(sourceObject);
      const UTATMeleeWeaponToolComponent* meleeWeaponTool = Cast<UTATMeleeWeaponToolComponent>(sourceObject);
      checkf(meleeWeaponTool, TEXT("Ability failed to cast source object %s to UTATMeleeWeaponToolComponent! Did you grant this ability outside the context of an equipped tool?"), *sourceObject->GetName());
      return meleeWeaponTool;
   }

private:
   // How long input must be held to trigger a charge attack
   UPROPERTY(EditDefaultsOnly, meta = (UIMin = 0, ClampMin = 0))
   float _chargeAttackPointOfNoReturnHoldTimeSeconds = 0.5f;

   // How long input must be held to trigger a charge attack
   UPROPERTY(EditDefaultsOnly, meta = (UIMin = 0, ClampMin = 0))
   float _takedownPointOfNoReturnHoldTimeSeconds = 0.2f;

   // How long input can be buffered to chain attacks
   UPROPERTY(EditDefaultsOnly, meta = (UIMin = 0, ClampMin = 0))
   float _inputBufferTimeoutSeconds = 0.3f;

   UPROPERTY(EditDefaultsOnly)
   FTATMeleeAttackDispatchInfo _lightAttackDispatchInfo;

   UPROPERTY(EditDefaultsOnly)
   FTATMeleeAttackDispatchInfo _chargeAttackDispatchInfo;

   // Event dispatched when input is released in the middle of a charge attack (expected to handle weakened early swing)
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag _chargeAttackEarlyReleaseGameplayEvent;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer _cancelableAttackTags;

   UPROPERTY(EditDefaultsOnly, Category="Takedown")
   float _takedownHoldDuration = 3;

   // TODO: should these be here, or the tool?
   UPROPERTY(EditDefaultsOnly, Category="Takedown")
   float _takedownTraceDistance = 1000;

   UPROPERTY(EditDefaultsOnly, Category="Takedown")
   float _takedownTraceHalfAngle = 25;

   UPROPERTY(EditDefaultsOnly, Category="Takedown")
   FCollisionProfileName _takedownTraceProfile;

   UPROPERTY(EditDefaultsOnly, Category="Takedown")
   bool _shouldAttemptTakedownWithChargedAttack { true };
   
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer _abilityToCancelIfStoppingChargeAttackAndTakingDown;

   float _inputPressedTime = 0.f;
   bool _inputReleased = false;

   FTimerHandle _pointOfNoReturnTimerHandle;
   FTimerHandle _inputBufferTimerHandle;
};
