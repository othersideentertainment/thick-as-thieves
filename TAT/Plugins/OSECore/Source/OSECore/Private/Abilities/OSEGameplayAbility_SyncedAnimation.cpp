// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEGameplayAbility_SyncedAnimation.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Abilities/OSEGameplayAbility_SyncedAnimationPlayer.h"

// ue4
#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_SyncedAnimation)

DEFINE_LOG_CATEGORY_STATIC(LogGameplayAbilitySyncedAnimation, Log, All);

UOSEGameplayAbility_SyncedAnimation::UOSEGameplayAbility_SyncedAnimation()
   : Super()
{
}

bool UOSEGameplayAbility_SyncedAnimation::MeetsConstraints(AActor* sourceActor, AActor* targetActor) const
{
   // simple one: if we require a target but don't have one we don't meet the constraints
   if (_RequiresTarget() && !targetActor)
      return false;

   FSyncedAnimationSearchParameters searchParams;
   searchParams.Source = sourceActor;
   searchParams.Target = targetActor;
   const bool meetsConstraints = SyncedAnimationDataAsset ? SyncedAnimationDataAsset->SyncedAnimationData.MeetsConstraints(searchParams, _RequiresTarget()) : false;
   return meetsConstraints;
}

void UOSEGameplayAbility_SyncedAnimation::EndPhase()
{
   // ability should be active to move along phases
   ensure(IsActive());

   // go to the next phase, if there is one
   const int nextPhaseIdx = int(_currentPhase) + 1;
   if (nextPhaseIdx < int(ESyncedAnimationPhase::MAX))
   {
      ESyncedAnimationPhase nextPhase = ESyncedAnimationPhase(nextPhaseIdx);
      _SetPhase(nextPhase);
   }
}

bool UOSEGameplayAbility_SyncedAnimation::CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags, const FGameplayTagContainer* targetTags, FGameplayTagContainer* optionalRelevantTags) const
{
   const bool canActivateBase = Super::CanActivateAbility(handle, actorInfo, sourceTags, targetTags, optionalRelevantTags);
   return canActivateBase;
}

void UOSEGameplayAbility_SyncedAnimation::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   // cache this off for use in utl functions between activate/end
   _activationInfo = activationInfo;

   // super will call ActivateAbility() on our blueprint classes, which may kill us
   Super::ActivateAbility(handle, ownerInfo, activationInfo, triggerEventData);

   if (IsActive())
   {
      // should never activate outside of the init phase
      check(_currentPhase == ESyncedAnimationPhase::Activate);
      OnPhaseActivate();
   }
}

void UOSEGameplayAbility_SyncedAnimation::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled)
{
   Super::EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);

   // reset back to activate for our next activation
   _currentPhase = ESyncedAnimationPhase::Activate;
   UE_LOG(LogGameplayAbilitySyncedAnimation, Verbose, TEXT("%s reset phase to %s"), *_GetDebugName(), *UEnum::GetValueAsString(_currentPhase));

   // clear it
   _activationInfo = FGameplayAbilityActivationInfo();
}

void UOSEGameplayAbility_SyncedAnimation::OnPhaseActivate_Implementation()
{
   // base impl just moves us along to the next phase
   EndPhase();
}

void UOSEGameplayAbility_SyncedAnimation::OnPhaseCheckInitialConstraints_Implementation()
{
   // base impl just moves us along to the next phase
   EndPhase();
}

void UOSEGameplayAbility_SyncedAnimation::OnPhaseInitialTarget_Implementation()
{
   // base impl just moves us along to the next phase
   EndPhase();
}

void UOSEGameplayAbility_SyncedAnimation::OnPhaseCheckInitialTargetConstraints_Implementation()
{
   // base impl just moves us along to the next phase
   EndPhase();
}

void UOSEGameplayAbility_SyncedAnimation::OnPhasePlaySourceAnimation_Implementation()
{
   // base impl just moves us along to the next phase
   EndPhase();
}

void UOSEGameplayAbility_SyncedAnimation::OnPhaseFinalTarget_Implementation()
{
   // base impl just moves us along to the next phase
   EndPhase();
}

void UOSEGameplayAbility_SyncedAnimation::OnPhasePlayTargetAnimation_Implementation()
{
   // base impl just moves us along to the next phase
   EndPhase();
}

void UOSEGameplayAbility_SyncedAnimation::OnPhaseEnded_Implementation()
{
   // base impl just moves us along to the next phase
   EndPhase();
}

FString UOSEGameplayAbility_SyncedAnimation::_GetDebugName() const
{
   FString debugName;
   if (SyncedAnimationDataAsset)
   {
      debugName = SyncedAnimationDataAsset->SyncedAnimationData.GetDebugName();
   }
   else
   {
      debugName = TEXT("NONE");
   }
   return FString::Printf(TEXT("(%s) %s"), HasAuthority(&_activationInfo) ? TEXT("SERVER") : TEXT("CLIENT"), *debugName);
}

void UOSEGameplayAbility_SyncedAnimation::_SetPhase(ESyncedAnimationPhase newPhase)
{
   ESyncedAnimationPhase oldPhase = _currentPhase;
   if (oldPhase != newPhase)
   {
      _currentPhase = newPhase;
      UE_LOG(LogGameplayAbilitySyncedAnimation, Verbose, TEXT("%s is going to phase %s"), *_GetDebugName(), *UEnum::GetValueAsString(_currentPhase));
      
      // a custom event per phase to make blueprint authoring less gross
      switch (_currentPhase)
      {
      case ESyncedAnimationPhase::Activate:
         OnPhaseActivate();
         break;
      case ESyncedAnimationPhase::CheckInitialConstraints:
         OnPhaseCheckInitialConstraints();
         break;
      case ESyncedAnimationPhase::InitialTarget:
         OnPhaseInitialTarget();
         break;
      case ESyncedAnimationPhase::CheckInitialTargetConstraints:
         OnPhaseCheckInitialTargetConstraints();
         break;
      case ESyncedAnimationPhase::PlaySourceAnimation:
         OnPhasePlaySourceAnimation();
         break;
      case ESyncedAnimationPhase::FinalTarget:
         OnPhaseFinalTarget();
         break;
      case ESyncedAnimationPhase::PlayTargetAnimation:
         OnPhasePlayTargetAnimation();
         break;
      case ESyncedAnimationPhase::Ended:
         OnPhaseEnded();
         break;
      }
   }
   else
   {
      UE_LOG(LogGameplayAbilitySyncedAnimation, Error, TEXT("%s is trying to going to phase %s but we're already in it!"), *GetName(), *UEnum::GetValueAsString(_currentPhase));
   }
}

