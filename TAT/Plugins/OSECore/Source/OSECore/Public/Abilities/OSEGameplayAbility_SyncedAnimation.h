// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "OSESyncedAnimations.h"
#include "Abilities/OSEGameplayAbility.h"

// ue4
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTargetDataFilter.h"

#include "OSEGameplayAbility_SyncedAnimation.generated.h"

class UOSESyncedAnimationDataAsset;

UENUM(BlueprintType)
enum class ESyncedAnimationTargetRequirement : uint8
{
   None,
   Any,
   Ally,
   Enemy,
};

UENUM(BlueprintType)
enum class ESyncedAnimationPhase : uint8
{
   Activate, // a stub to do any upfront work
   CheckInitialConstraints, // check our initial constraints, like if an ability requires us to be sprinting
   InitialTarget, // get our initial target, in some abilities (player help up / enemy takedown) this will be our only target
   CheckInitialTargetConstraints, // ensure our target meets our synced animation constraints
   PlaySourceAnimation, // play the animation on ourselves
   FinalTarget, // optionally re-acquire a final target when our source animation hits the spot where we should do a target check
   PlayTargetAnimation, // also apply damage in the case of combat
   Ended,

   MAX           UMETA(Hidden)
};

// Base class for all synced animation abilities
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_SyncedAnimation : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:
   UOSEGameplayAbility_SyncedAnimation();

   // data about our synced animation
   UFUNCTION(BlueprintPure)
   UOSESyncedAnimationDataAsset* GetSyncedAnimationDataAsset() const { return SyncedAnimationDataAsset; }

   // does our source and target actor meet constraints to trigger this synced animation ability?
   UFUNCTION(BlueprintPure)
   bool MeetsConstraints(AActor* sourceActor, AActor* targetActor) const;

   // phase
   UFUNCTION(BlueprintPure)
   ESyncedAnimationPhase GetCurrentPhase() const { return _currentPhase; }
   UFUNCTION(BlueprintCallable)
   void EndPhase();

   // from UGameplayAbility
   virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags = nullptr, const FGameplayTagContainer* targetTags = nullptr, OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled) override;

protected:
   // custom events for each phase
   UFUNCTION(BlueprintNativeEvent)
   void OnPhaseActivate();
   UFUNCTION(BlueprintNativeEvent)
   void OnPhaseCheckInitialConstraints();
   UFUNCTION(BlueprintNativeEvent)
   void OnPhaseInitialTarget();
   UFUNCTION(BlueprintNativeEvent)
   void OnPhaseCheckInitialTargetConstraints();
   UFUNCTION(BlueprintNativeEvent)
   void OnPhasePlaySourceAnimation();
   UFUNCTION(BlueprintNativeEvent)
   void OnPhaseFinalTarget();
   UFUNCTION(BlueprintNativeEvent)
   void OnPhasePlayTargetAnimation();
   UFUNCTION(BlueprintNativeEvent)
   void OnPhaseEnded();

private:
   FString _GetDebugName() const;
   bool _RequiresTarget() const { return TargetRequirement != ESyncedAnimationTargetRequirement::None; }
   void _SetPhase(ESyncedAnimationPhase newPhase);

protected:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Synced Animation")
   UOSESyncedAnimationDataAsset* SyncedAnimationDataAsset = nullptr;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Synced Animation")
   ESyncedAnimationTargetRequirement TargetRequirement = ESyncedAnimationTargetRequirement::Any;

private:
   ESyncedAnimationPhase _currentPhase = ESyncedAnimationPhase::Activate;
   FGameplayAbilityActivationInfo _activationInfo;
};
