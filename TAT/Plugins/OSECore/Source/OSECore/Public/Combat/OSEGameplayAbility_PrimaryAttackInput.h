// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

// ue4

#include "OSEGameplayAbility_PrimaryAttackInput.generated.h"

class AOSECharacterBase;
class UCombatComponent;
class UOSEGameplayAbility_MeleeAttack;

// TODO: It's possible that this impl is too TAT-specific and should live at the project level

UENUM(BlueprintType)
enum class EPrimaryAttackRandomizationType : uint8
{
   NotRandom,
   RandomAny,
   RandomCycleThroughPools,
};

USTRUCT(BlueprintType)
struct OSECORE_API FPrimaryAttackPool
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
   TArray<TSubclassOf<UOSEGameplayAbility_MeleeAttack>> AttackAbilities;
};

UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_PrimaryAttackInput : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:
   UOSEGameplayAbility_PrimaryAttackInput();

   /// Shared behaviors
   
   // This is the max time between inputs allowed before we drop the current combo
   UPROPERTY(EditDefaultsOnly, Category = "Combat");
   float ComboDropInputTime = 0.5f;

   // Enabling this enables two interesting behaviors:
   // - triggering a light attack then holding down the button immediately after will launch us into a heavy attack
   // - triggering a heavy attack that lands on a target, which does NOT cause overextended, and then continuing to follow-up with a heavy attack
   UPROPERTY(EditDefaultsOnly, Category = "Combat");
   bool AllowRetriggeringOnEndAbility = false;

   /// Heavy Attacks
   
   // time from left click till we start readying a heavy attack
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Heavy Attack");
   float HeavyAttackPointOfNoReturnHoldTime = 0.35f;

   // heavy attack abilities to cycle through
   UPROPERTY(EditDefaultsOnly, Category = "Combat|Heavy Attack");
   TArray<TSubclassOf<UOSEGameplayAbility_MeleeAttack>> HeavyAttackAbilities;

   // NOTE: still requires abilities to also be populated in the HeavyAttackAbilities array
   UPROPERTY(EditDefaultsOnly, Category = "Combat|Heavy Attack", meta = (EditCondition = "HeavyAttackRandomizationType == EPrimaryAttackRandomizationType::RandomCycleThroughPools"));
   TArray<FPrimaryAttackPool> HeavyAttackPools;

   UPROPERTY(EditDefaultsOnly, Category = "Combat|Heavy Attack");
   EPrimaryAttackRandomizationType HeavyAttackRandomizationType = EPrimaryAttackRandomizationType::NotRandom;

   /// Light Attacks
   
   // light attack abilities to cycle through
   UPROPERTY(EditDefaultsOnly, Category = "Combat|Light Attack");
   TArray<TSubclassOf<UOSEGameplayAbility_MeleeAttack>> LightAttackAbilities;

   // NOTE: still requires abilities to also be populated in the LightAttackAbilities array
   UPROPERTY(EditDefaultsOnly, Category = "Combat|Light Attack", meta = (EditCondition = "LightAttackRandomizationType == EPrimaryAttackRandomizationType::RandomCycleThroughPools"));
   TArray<FPrimaryAttackPool> LightAttackPools;

   UPROPERTY(EditDefaultsOnly, Category = "Combat|Light Attack");
   EPrimaryAttackRandomizationType LightAttackRandomizationType = EPrimaryAttackRandomizationType::NotRandom;

   // from UGameplayAbility
   virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags = nullptr, const FGameplayTagContainer* targetTags = nullptr, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled) override;
   virtual void InputPressed(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) override;
   virtual void InputReleased(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) override;

protected:
   // subclasses could impl
   virtual void _OnHeavyAttackPointOfNoReturnReached();
   virtual void _DoLightAttack();
   virtual void _DoHeavyAttack();
   virtual void _DoAttack(const TArray<TSubclassOf<UOSEGameplayAbility_MeleeAttack>>& abilities, int& index, int& otherIndex, bool isHeavyAttack);
   virtual void _EndAbility(bool wasCanceled);

   UFUNCTION()
   void _OnAbilityEnd(UGameplayAbility* ability);
   UFUNCTION()
   void _OnCombatAnimationCanBeInterrupted();

   bool _TryCancelCurrentAttackAndStartNext();
   int _TryFindNextPooledAbilityIndex(const TArray<TSubclassOf<UOSEGameplayAbility_MeleeAttack>>& allAbilities, const TArray<FPrimaryAttackPool>& pools, int& index);

   UFUNCTION(BlueprintNativeEvent, Category = "Combat")
   void OnPlayWindup();
   void OnPlayWindup_Implementation() { };
   UFUNCTION(BlueprintNativeEvent, Category = "Combat")
   void OnHeavyAttackPointOfNoReturnReached();
   void OnHeavyAttackPointOfNoReturnReached_Implementation() { };
   UFUNCTION(BlueprintNativeEvent, Category = "Combat")
   void OnLightAttackTriggered(UGameplayAbility* ability);
   void OnLightAttackTriggered_Implementation(UGameplayAbility* ability) { };
   UFUNCTION(BlueprintNativeEvent, Category = "Combat")
   void OnHeavyAttackTriggered(UGameplayAbility* ability);
   void OnHeavyAttackTriggered_Implementation(UGameplayAbility* ability) { };

   // blueprint bindings for important attack timing events
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeavyAttackPointOfNoReturnReached);
   UPROPERTY(BlueprintAssignable, Category = "Combat")
   FOnHeavyAttackPointOfNoReturnReached OnHeavyAttackPointOfNoReturnReachedEvent;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatAnimationCanBeInterrupted);
   UPROPERTY(BlueprintAssignable, Category = "Combat")
   FOnCombatAnimationCanBeInterrupted OnCombatAnimationCanBeInterrupted;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatAttackEnded);
   UPROPERTY(BlueprintAssignable, Category = "Combat")
   FOnCombatAttackEnded OnCombatAttackEnded;

   // utl
   AOSECharacterBase& _GetCharacter() const;
   UAbilitySystemComponent& _GetAbilitySystemComponent();
   UCombatComponent& _GetCombatComponent() const;

private:
   enum class State
   {
      WaitingForInputRelease,
      AttackTriggered,
      WaitingForTriggeredAbilityEnd,
   };

   // per-activation state
   FTimerHandle _timerHandle_OnHeavyAttackPointOfNoReturnReached;
   bool _doHeavyAttack = false;
   UOSEGameplayAbility_MeleeAttack* _triggeredAbility = nullptr;
   State _state = State::WaitingForInputRelease;
   float _chainAttackAttemptTime = float(INDEX_NONE);

   // per-actor state
   float _lastWorldTime = float(INDEX_NONE);
   int _lightAttackComboIndex = 0;
   int _heavyAttackComboIndex = 0;
   int _lightAttackPoolIndex = 0;
   int _heavyAttackPoolIndex = 0;
};
