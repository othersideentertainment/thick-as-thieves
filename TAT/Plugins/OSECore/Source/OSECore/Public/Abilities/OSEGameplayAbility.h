// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEAbilityInfo.h"
#include "Abilities/UpgradeQueryInterface.h"

// ue
#include "Abilities/GameplayAbility.h"

#include "OSEGameplayAbility.generated.h"

class UOSEAbilityCost;

UENUM(BlueprintType)
enum class AILockType : uint8
{
   DoNotLock,
   ForDuration,
};

/** Notification delegate definition for when the gameplay ability ends */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSEOnGameplayAbilityEnded, bool, wasCancelled);

/// Derived base gameplay ability class
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility : public UGameplayAbility, public IUpgradeQueryInterface
{
   GENERATED_BODY()
   
public:
   // Sets default values
   UOSEGameplayAbility();

   /// Sends a targeting data event to this ability, will activate any WaitTargetEvent tasks
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   virtual void SendTargetEvent(FGameplayTag eventTag, const FGameplayAbilityTargetDataHandle& targetData);

   /// Sends a targeting data cancel to this ability, will cancel any WaitTargetEvent tasks
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   virtual void SendTargetCancel();

   /// Returns true if this ability can be activated right now. Has no side effects
   virtual bool CanActivateAbility(
      const FGameplayAbilitySpecHandle handle,
      const FGameplayAbilityActorInfo* actorInfo,
      const FGameplayTagContainer* sourceTags = nullptr,
      const FGameplayTagContainer* targetTags = nullptr,
      OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;

   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;

   virtual bool CommitAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, OUT FGameplayTagContainer * optionalRelevantTags = nullptr) override;
   virtual bool CommitAbilityCooldown(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const bool forceCooldown, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) override;
   virtual bool CommitAbilityCost(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) override;

   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled) override;

   bool TryGetCooldownMultiplier(float& outMultiplier) const;
   bool TryGetCooldownMultiplier(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, float& outMultiplier) const;

   // Attempts to apply ChildEffectClass, returning true if applied (and false if not applied, or already applied previously)
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   bool CommitChildEffect();

   /// Called when the avatar actor is set/changes
   virtual void OnAvatarSet(const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilitySpec& spec) override;

   // UpgradeQueryInterface
   virtual int32 GetUpgradeValue(FGameplayTag tag, int32 fallback = 0) const final;

   // Removes stacks of the GameplayEffect that granted this ability. Can only be called on instanced abilities.
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   void RemoveGrantedByEffectStacks(int32 stacksToRemove = -1);

   /// GameplayEvent with this tag is sent to the Owner if this Ability fails to activate.
   /// InstigatorTags of event will contain reason(s) for failure.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE")
   FGameplayTag AbilityFailedEventTag;

   // Class of effect to maintain on owner while this Abilty operates. Added on Commit, removed on End.
   UPROPERTY(EditDefaultsOnly, Category = "Ability|OSE")
   TSubclassOf<UGameplayEffect> ChildEffectClass;

   UFUNCTION(BlueprintPure)
   const FOSEAbilityInfo& GetAbilityInfo() const { return AbilityInfo; }

   const FGameplayTagContainer& GetActivationBlockedTags() const { return ActivationBlockedTags; }

   /// Only works on the local client, so it's only useful to local abilities
   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   bool IsLocalInputPressed();

   /// Convenience method to sample a scalable float at the ability's level
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta=(HideSelfPin="true", DisplayName = "GetValueAtAbilityLevel", CompactNodeTitle = "GetValueAtAbilityLevel"))
   float GetScalableFloatValueAtAbilityLevel(const FScalableFloat& scalableFloat) const;

   UPROPERTY(BlueprintAssignable, Category = "Ability|OSE")
   FOSEOnGameplayAbilityEnded OnAbilityEnded;

   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   bool IsAbilityActive() const;

   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   FGameplayAbilityTargetingLocationInfo MakeTargetLocationInfoFromTransform(const FTransform& transform) const;

protected:

   // Handle to GameplayEffect added as specified by ChildEffectClass, for later cleanup.
   FActiveGameplayEffectHandle _childEffectHandle;

   /// True if ability activation requires the owner to have local control
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE")
   bool ActivationRequiresLocalControl;

   /// True if ability activation requires the owner to have a controller
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE")
   bool ActivationRequiresController;

   /// True if ability activation requires the owner to have a _player_ controller.
   /// Doesn't necessarily have to be locally controlled.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE")
   bool ActivationRequiresPlayerController;

   /// True if ability activation requires the controller to accept move input.
   /// Useful for locally controlled, movement-related input abilities (such as jumping)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE")
   bool ActivationRequiresMoveInput;

   /// True if ability activation requires the controller to accept look input.
   /// Useful for locally controlled, aiming-related input abilities
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE")
   bool ActivationRequiresLookInput;

   /// True if ability should be auto-activated
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE")
   bool ActivateAbilityWhenGranted;

   // Fill in this info if we want to be able to query info about this ability.
   // Mark Set HideInUI = false if it should show up in the "Ability Bar" in the HUD
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE")
   FOSEAbilityInfo AbilityInfo;

   /// The upgrade tag that will be used as the level for CooldownMultiplierUpgradeValue
   UPROPERTY(EditDefaultsOnly, Category = "Cooldowns|Upgrade", meta = (Categories = "Upgrade"))
   FGameplayTag CooldownMultiplierUpgradeTag;

   /// Only valid if CooldownMultiplierUpgradeTag is set (unless we want it regardless)
   UPROPERTY(EditDefaultsOnly, Category = "Cooldowns|Upgrade")
   FScalableFloat CooldownMultiplierUpgradeValue;

   // TODO: Move to sparse class data (along with AbilityInfo, VO, etc)?
   UPROPERTY(EditDefaultsOnly, Instanced, Category = Costs)
   TArray<UOSEAbilityCost*> AdditionalCosts;

   /** True if the owning actor is locally controlled, true in single player */
   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|OSE", DisplayName = "IsLocallyControlled", meta = (ScriptName = "IsLocallyControlled"))
   bool K2_IsLocallyControlled() const { return IsLocallyControlled(); }

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", DisplayName = "DoesAbilitySatisfyTagRequirements", meta = (ScriptName = "DoesAbilitySatisfyTagRequirements"))
   bool K2_DoesAbilitySatisfyTagRequirements() const;

   UPROPERTY(EditDefaultsOnly, Category = "Ability|OSE|AI")
   AILockType AILock = AILockType::DoNotLock;

protected:
   virtual bool CheckCost(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;
   virtual void ApplyCost(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) const override;

   // If ChildEffectHandle is not yet valid, apply ChildEffectClass now
   void _TryApplyChildEffect(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo);

   int32 _GetUpgradeValueFromActorInfo(const FGameplayAbilityActorInfo* actorInfo, FGameplayTag tag, int32 fallback = 0) const;

   void _SetAILock(bool locked);

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR
};

