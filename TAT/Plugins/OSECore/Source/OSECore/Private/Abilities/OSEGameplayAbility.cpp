// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/OSEGameplayAbility.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/OSEAbilityCost.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Controller.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "BrainComponent.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility)

DEFINE_LOG_CATEGORY_STATIC(LogOSEGameplayAbility, Log, All);

// Sets default values
UOSEGameplayAbility::UOSEGameplayAbility()
   : Super()
   , ActivationRequiresLocalControl(false)
   , ActivationRequiresController(false)
   , ActivationRequiresPlayerController(false)
   , ActivationRequiresMoveInput(false)
   , ActivationRequiresLookInput(false)
   , ActivateAbilityWhenGranted(false)
   , CooldownMultiplierUpgradeValue(1.f)
{
   // Default behavior
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

   // hidden in UI by default, most abilities aren't going to have a UI definition
   AbilityInfo.HideInUI = true;
}

void UOSEGameplayAbility::SendTargetEvent(FGameplayTag eventTag, const FGameplayAbilityTargetDataHandle& targetData)
{
   UAbilitySystemComponent* abilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
   if (ensure(abilitySystemComponent))
   {
      abilitySystemComponent->ConfirmAbilityTargetData(CurrentSpecHandle, GetCurrentActivationInfo().GetActivationPredictionKey(), targetData, eventTag);
   }
}

void UOSEGameplayAbility::SendTargetCancel()
{
   UAbilitySystemComponent* abilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
   if (ensure(abilitySystemComponent))
   {
      abilitySystemComponent->CancelAbilityTargetData(CurrentSpecHandle, GetCurrentActivationInfo().GetActivationPredictionKey());
   }
}

// Called when the avatar actor is set/changes
void UOSEGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilitySpec& spec)
{
   Super::OnAvatarSet(actorInfo, spec);

   if (ActivateAbilityWhenGranted)
   {
      if (auto asc = Cast<UOSEAbilitySystemComponent>(actorInfo->AbilitySystemComponent))
      {
         // Make sure we are the ones who can initiate an ability
         if (asc->HasAuthorityToActivateAbility(spec))
         {
            if (!asc->TryActivateAbility(spec.Handle, false))
            {
               UE_LOG(LogOSEGameplayAbility, Warning, TEXT("OSEGameplayAbility %s was set to ActivateAbilityWhenGranted, but was not able to activate!"), *GetName());
            }
         }
      }
   }
}

int32 UOSEGameplayAbility::GetUpgradeValue(FGameplayTag tag, int32 fallback) const
{
   return _GetUpgradeValueFromActorInfo(CurrentActorInfo, tag, fallback);
}

void UOSEGameplayAbility::RemoveGrantedByEffectStacks(int32 stacksToRemove /*= -1*/)
{
   check(IsInstantiated()); // You should not call this on non instanced abilities.
   check(CurrentActorInfo);
   if (CurrentActorInfo)
   {
      UAbilitySystemComponent* const asc = GetAbilitySystemComponentFromActorInfo_Ensured();
      FActiveGameplayEffectHandle activeHandle = asc->FindActiveGameplayEffectHandle(GetCurrentAbilitySpecHandle());
      if (activeHandle.IsValid())
      {
         asc->RemoveActiveGameplayEffect(activeHandle, stacksToRemove);
      }
   }
}

bool UOSEGameplayAbility::IsLocalInputPressed()
{
   if (UOSEAbilitySystemComponent* asc = Cast<UOSEAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
   {
      if (FGameplayAbilitySpec* spec = GetCurrentAbilitySpec())
      {
         if (spec->InputID >= 0)
         {
            return asc->IsLocalInputPressed(static_cast<EAbilityInputType>(spec->InputID));
         }
      }
   }
   return false;
}

float UOSEGameplayAbility::GetScalableFloatValueAtAbilityLevel(const FScalableFloat& scalableFloat) const
{
   return scalableFloat.GetValueAtLevel(GetAbilityLevel());
}

bool UOSEGameplayAbility::IsAbilityActive() const
{
   return IsActive();
}

FGameplayAbilityTargetingLocationInfo UOSEGameplayAbility::MakeTargetLocationInfoFromTransform(const FTransform& transform) const
{
   FGameplayAbilityTargetingLocationInfo returnLocation;
   returnLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
   returnLocation.LiteralTransform = transform;
   returnLocation.SourceActor = GetActorInfo().AvatarActor.Get();
   returnLocation.SourceAbility = const_cast<UOSEGameplayAbility*>(this);
   return returnLocation;
}

// Look for a controller or pawn in the owner chain for the specified actor
static const AController* _FindControllerByActor(const AActor* testActor)
{
   while (testActor)
   {
      if (const AController* castController = Cast<AController>(testActor))
         return castController;

      if (const APawn* pawn = Cast<APawn>(testActor))
         return Cast<AController>(pawn->GetController());

      testActor = testActor->GetOwner();
   }

   return nullptr;
}

static const AController* _FindController(const FGameplayAbilityActorInfo* actorInfo)
{
   check(actorInfo != nullptr);
   check(actorInfo->OwnerActor.Get() != nullptr);

   // This may be null
   const APlayerController* playerController = actorInfo->PlayerController.Get();
   if (playerController != nullptr)
      return playerController;

   // Look for a controller in the OWNER chain
   if (const AController* foundController = _FindControllerByActor(actorInfo->OwnerActor.Get()))
   {
      return foundController;
   }

   // Look for a controller in the AVATAR chain
   if (const AController* foundController = _FindControllerByActor(actorInfo->AvatarActor.Get()))
   {
      return foundController;
   }

   // Not found
   return nullptr;
}

// Returns true if this ability can be activated right now. Has no side effects
bool UOSEGameplayAbility::CanActivateAbility(
   const FGameplayAbilitySpecHandle handle,
   const FGameplayAbilityActorInfo* actorInfo,
   const FGameplayTagContainer* sourceTags /* = nullptr */,
   const FGameplayTagContainer* targetTags /* = nullptr */,
   OUT FGameplayTagContainer* optionalRelevantTags /* = nullptr */) const
{
   // Call base class
   if (!Super::CanActivateAbility(handle, actorInfo, sourceTags, targetTags, optionalRelevantTags))
      return false;

   if (ActivationRequiresLocalControl)
   {
      if (!actorInfo->OwnerActor.Get()->HasLocalNetOwner())
         return false;
   }

   // This may be null
   const AController* const foundController = _FindController(actorInfo);

   if (ActivationRequiresController)
   {
      if (foundController == nullptr)
         return false;
   }

   if (ActivationRequiresPlayerController)
   {
      if (Cast<APlayerController>(foundController) == nullptr)
         return false;
   }

   if (ActivationRequiresMoveInput || ActivationRequiresLookInput)
   {
      // Requires a controller to check these values
      if (foundController == nullptr)
         return false;

      if (ActivationRequiresMoveInput && foundController->IsMoveInputIgnored())
         return false;

      if (ActivationRequiresLookInput && foundController->IsLookInputIgnored())
         return false;
   }

   return true;
}

bool UOSEGameplayAbility::CommitAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, OUT FGameplayTagContainer* optionalRelevantTags)
{
   if (Super::CommitAbility(handle, actorInfo, activationInfo)) 
   {
      _TryApplyChildEffect(handle, actorInfo, activationInfo);
      return true;
   }
   return false;
}

bool UOSEGameplayAbility::CommitAbilityCooldown(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const bool forceCooldown, OUT FGameplayTagContainer* optionalRelevantTags)
{
   if (Super::CommitAbilityCooldown(handle, actorInfo, activationInfo, forceCooldown))
   {
      return true;
   }
   return false;
}

bool UOSEGameplayAbility::CommitAbilityCost(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, OUT FGameplayTagContainer* optionalRelevantTags)
{
   if (Super::CommitAbilityCost(handle, actorInfo, activationInfo))
   {
      return true;
   }
   return false;
}

bool UOSEGameplayAbility::K2_DoesAbilitySatisfyTagRequirements() const
{
   if (const UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo())
   {
      return DoesAbilitySatisfyTagRequirements(*asc);
   }
   return false;
}

bool UOSEGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, OUT FGameplayTagContainer* optionalRelevantTags) const
{
   if (!Super::CheckCost(handle, actorInfo, optionalRelevantTags))
   {
      return false;
   }

   if (!actorInfo)
   {
      return false;
   }

   for (UOSEAbilityCost* cost : AdditionalCosts)
   {
      if (cost && !cost->CanAfford(this, handle, actorInfo, optionalRelevantTags))
      {
         return false;
      }
   }

   return true;
}

void UOSEGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) const
{
   Super::ApplyCost(handle, actorInfo, activationInfo);

   for (UOSEAbilityCost* cost : AdditionalCosts)
   {
      if (cost)
      {
         cost->ApplyCost(this, handle, actorInfo, activationInfo);
      }
   }
}

void UOSEGameplayAbility::_TryApplyChildEffect(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo)
{
   if (ChildEffectClass && !_childEffectHandle.IsValid())
   {
      const UGameplayEffect* gameplayEffect = ChildEffectClass->GetDefaultObject<UGameplayEffect>();
      _childEffectHandle = ApplyGameplayEffectToOwner(handle, actorInfo, activationInfo, gameplayEffect, 1, 1);
   }
}

bool UOSEGameplayAbility::TryGetCooldownMultiplier(float& outMultiplier) const
{
   return TryGetCooldownMultiplier(CurrentSpecHandle, CurrentActorInfo, outMultiplier);
}

bool UOSEGameplayAbility::TryGetCooldownMultiplier(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, float& outMultiplier) const
{
   if (!CooldownMultiplierUpgradeTag.IsValid())
   {
      return false;
   }

   const int upgradeLevel = _GetUpgradeValueFromActorInfo(actorInfo, CooldownMultiplierUpgradeTag);
   outMultiplier = CooldownMultiplierUpgradeValue.GetValueAtLevel(upgradeLevel);
   return true;
}

bool UOSEGameplayAbility::CommitChildEffect()
{
   if (!ChildEffectClass)
   {
      return true;
   }

   const FActiveGameplayEffectHandle prevChildEffectHandle = _childEffectHandle;
   _TryApplyChildEffect(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
   return prevChildEffectHandle != _childEffectHandle && _childEffectHandle.IsValid();
}

int32 UOSEGameplayAbility::_GetUpgradeValueFromActorInfo(const FGameplayAbilityActorInfo* actorInfo, FGameplayTag tag, int32 fallback /*= 0*/) const
{
   if (!ensure(actorInfo))
   {
      return fallback;
   }

   if (auto asc = Cast<UOSEAbilitySystemComponent>(actorInfo->AbilitySystemComponent))
   {
      return asc->GetUpgradeValue(tag, fallback);
   }
   return fallback;
}

void UOSEGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   if (AILock == AILockType::ForDuration)
   {
      _SetAILock(true);
   }
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);

}


void UOSEGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled)
{


   if (_childEffectHandle.IsValid())
   {
      // Since we're in EndAbility, we already know we have authority
      actorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(_childEffectHandle, 1);
      _childEffectHandle = FActiveGameplayEffectHandle(); // Invalidate current handle
   }

   Super::EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);

   if (AILock == AILockType::ForDuration)
   {
      _SetAILock(false);
   }
   // Execute our delegate and unbind it, as we are no longer active and listeners can re-register when we become active again.
   OnAbilityEnded.Broadcast(wasCancelled);
   OnAbilityEnded.Clear();
}

void UOSEGameplayAbility::_SetAILock(bool locked)
{
   const AActor* actor = GetOwningActorFromActorInfo();
   if (actor == nullptr)
   {
      return;
   }
   // This may be null
   const AAIController* aiController = Cast<AAIController>(_FindControllerByActor(actor));
   if (aiController != nullptr)
   {
      if (locked)
      {
         aiController->BrainComponent->LockResource(EAIRequestPriority::HardScript);
      }
      else
      {
         aiController->BrainComponent->ForceUnlockResource();
      }
   }
}

#if WITH_EDITOR
EDataValidationResult UOSEGameplayAbility::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);
   return result;
}
#endif // WITH_EDITOR

