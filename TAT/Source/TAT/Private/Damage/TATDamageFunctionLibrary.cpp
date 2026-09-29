// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Damage/TATDamageFunctionLibrary.h"

// tat
#include "Damage/TATDamageSettings.h"
#include "Damage/TATDamageTypes.h"
#include "Damage/TATSimpleDamageableInterface.h"

// ose
#include "Abilities/Attributes/AttributeBaseSet.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDamageFunctionLibrary)


DEFINE_LOG_CATEGORY_STATIC(LogTATDamageFunctionLibrary, Log, All);

namespace DamageHelpers
{
   // Add a loose gameplay tag, and remove it after a duration
   // Not proper PredictionKey-based client prediction, so won't be rolled back or de-duped
   // But may be sufficient for rough rate limiting
   static void AddLooseTagForDuration(UAbilitySystemComponent* asc, FGameplayTag tag, float duration)
   {
      check(asc);
      check(duration > 0);

      UE_LOG(LogTATDamageFunctionLibrary, Verbose, TEXT("PseudoPrediction: Adding tag %s to %s for %f seconds"), *tag.ToString(), *GetNameSafe(asc->GetOwner()), duration);
      asc->AddLooseGameplayTag(tag, 1);

      auto removeCooldownTag = [weakAsc = MakeWeakObjectPtr(asc), tag]() {
         if (UAbilitySystemComponent* asc = weakAsc.Get())
         {
            UE_LOG(LogTATDamageFunctionLibrary, Verbose, TEXT("PseudoPrediction: Removing tag %s to %s"), *tag.ToString(), *GetNameSafe(asc->GetOwner()));
            asc->RemoveLooseGameplayTag(tag, 1);
         }
      };

      FTimerHandle timerHandle;
      asc->GetWorld()->GetTimerManager().SetTimer(timerHandle, MoveTemp(removeCooldownTag), duration, false);
   }

   static float GetOutgoingDamage(const FTATDamageWithType& damage, const AActor* sourceActor, const AActor* targetActor)
   {
      if (sourceActor && sourceActor != targetActor)
      {
         return UTATDamageFunctionLibrary::CalculateOutgoingDamageForSource(damage, sourceActor);
      }
      return damage.DamageAmount;
   }
}

float UTATDamageFunctionLibrary::CalculateOutgoingDamageForSource(const FTATDamageWithType& damage, const AActor* actor)
{
   float damageAmount = damage.DamageAmount;
   UAbilitySystemComponent* sourceAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor, false);
   if (sourceAsc == nullptr)
   {
      return damageAmount;
   }

   // NOTE: this used to be done via an execution in a gameplay effect, but that doesn't
   //       seem to be easier than just doing the calculation here, so I have just inlined it
   FGameplayEffectAttributeCaptureDefinition captureDef(UAttributeBaseSet::GetAttackDamageMultiplierAttribute().GetUProperty(), EGameplayEffectAttributeCaptureSource::Source, false);
   FGameplayEffectAttributeCaptureSpec captureSpec(captureDef);

   sourceAsc->CaptureAttributeForGameplayEffect(captureSpec);

   FGameplayTagContainer sourceTags;
   sourceAsc->GetOwnedGameplayTags(sourceTags);
   sourceTags.AddTag(damage.DamageType);

   FAggregatorEvaluateParameters params;
   params.SourceTags = &sourceTags;

   float attackDamageMultiplier = 0;
   if (captureSpec.AttemptCalculateAttributeMagnitude(params, attackDamageMultiplier))
   {
      damageAmount *= attackDamageMultiplier;
   }

   return damageAmount;
}

void UTATDamageFunctionLibrary::DealDamage(
   AActor* sourceActor,
   AActor* targetActor,
   FTATDamageWithType damage,
   const FVector& origin,
   const FHitResult& hitResult,
   const FGameplayTag damageContext)
{
   if (damage.DamageAmount <= 0 || targetActor == nullptr || !targetActor->HasAuthority())
   {
      return;
   }

   if (IsComponentInvalidDamageTarget(hitResult.GetComponent()))
   {
      return;
   }

   FGameplayTag damageType = damage.DamageType;
   if (!damageType.IsValid())
   {
      // fall back to physical by default
      damageType = TAG_DamageType_Physical;
      UE_LOG(LogTATDamageFunctionLibrary, Warning, TEXT("DealDamage: no damage type provided, falling back to physical (Target=%s Source=%s)"), *targetActor->GetActorNameOrLabel(), sourceActor ? *sourceActor->GetActorNameOrLabel() : TEXT("None"));
   }

   // If it is a simple damageable do that instead
   if (targetActor && targetActor->Implements<UTATSimpleDamageableInterface>())
   {
      // assumption: An actor will never implement both at once
      check(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(targetActor, false) == nullptr);
      if (!ITATSimpleDamageableInterface::Execute_CanHandleDamage(targetActor))
      {
         return;
      }

      const float simpleDamageAmount = DamageHelpers::GetOutgoingDamage(damage, sourceActor, targetActor);
      ITATSimpleDamageableInterface::Execute_AuthorityHandleDamage(targetActor, FTATDamageWithType(simpleDamageAmount, damageType), FTATSimpleDamageSource(sourceActor, origin, hitResult));
      return;
   }

   UAbilitySystemComponent* targetAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(targetActor, false);
   if (targetAsc == nullptr || !targetAsc->IsOwnerActorAuthoritative())
   {
      return;
   }
   
   // ASSUMPTION: The damage effect will be loaded by the subsystem, so the load is a no-op
   const UTATDamageSettings& damageSettings = UTATDamageSettings::Get();
   TSubclassOf<UGameplayEffect> effectClass = damageSettings.DamageEffectByType.FindRef(damageType).LoadSynchronous();
   if (effectClass == nullptr)
   {
      UE_LOG(LogTATDamageFunctionLibrary, Error, TEXT("DealDamage: could not find damage gameplay effect for type %s"), *damageType.ToString());
      return;
   }

   // calculate outgoing damage (optional)
   const float damageAmount = DamageHelpers::GetOutgoingDamage(damage, sourceActor, targetActor);

   // populate effect context
   FGameplayEffectContextHandle context = targetAsc->MakeEffectContext();
   if (sourceActor)
   {
      // NOTE: The old BPFL logic also used the source actor for both. I could see
      //       potentially wanting to distinguish between the two but I did not
      //       have any immediate use-cases, so it didn't seem worth adding to the external
      //       api for it.
      context.AddInstigator(sourceActor, sourceActor);
   }
   if (origin != FVector::ZeroVector)
   {
      context.AddOrigin(origin);
   }
   if (hitResult.bBlockingHit)
   {
      context.AddHitResult(hitResult);
   }

   UGameplayEffect* effectCdo = effectClass->GetDefaultObject<UGameplayEffect>();
   check(effectCdo->GetAssetTags().HasTag(damageType));
   constexpr float level = 0;
   FGameplayEffectSpec spec(effectCdo, context, level);
   spec.SetSetByCallerMagnitude(TAG_SetByCaller_Damage, damageAmount);
   if (damageContext.IsValid())
   {
      spec.AddDynamicAssetTag(damageContext);
   }
   targetAsc->ApplyGameplayEffectSpecToSelf(spec);
   UE_LOG(LogTATDamageFunctionLibrary, Verbose, TEXT("DealDamage: Amount=%f Type=%s Target=%s"), damageAmount, *damageType.ToString(), *targetActor->GetActorNameOrLabel());
}

FTATDamageWithType UTATDamageFunctionLibrary::Conv_TATScalableDamageWithTypeToTATDamageWithType(const FTATScalableDamageWithType& scalableDamage)
{
   return FTATDamageWithType(scalableDamage);
}

FTATDamageWithType UTATDamageFunctionLibrary::EvaluateDamageAtLevel(const FTATScalableDamageWithType& scalableDamage, float level)
{
    return FTATDamageWithType(scalableDamage.DamageAmount.GetValueAtLevel(level), scalableDamage.DamageType);
}

FGameplayTag UTATDamageFunctionLibrary::ExtractDamageTypeFromContainer(const FGameplayTagContainer& tagContainer)
{
   // Find the first gameplay tag whose direct parent is DamageType
   // Assumption: DamageType tags are currently not nested (being more strict for the heck of it)
   for (const FGameplayTag& tag : tagContainer)
   {
      if (tag.RequestDirectParent() == TAG_DamageType)
      {
         return tag;
      }
   }

   return FGameplayTag();
}

FGameplayTag UTATDamageFunctionLibrary::ExtractDamageContextFromContainer(const FGameplayTagContainer& tagContainer)
{
   for (const FGameplayTag& tag : tagContainer)
   {
      if (tag.RequestDirectParent() == TAG_DamageContext)
      {
         return tag;
      }
   }

   return FGameplayTag();
}

bool UTATDamageFunctionLibrary::IsComponentInvalidDamageTarget(const UActorComponent* component)
{
   static const FName kNoDamageTag("NoDamage");
   return component != nullptr && component->ComponentHasTag(kNoDamageTag);
}

bool UTATDamageFunctionLibrary::IsActorPossiblyDamageable(const AActor* actor)
{
   // Can add check for future trivial-damage-response interface, or other preconditions here
   const UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor, false);
   if (asc != nullptr)
   {
      return true;
   }

   if (actor && actor->Implements<UTATSimpleDamageableInterface>())
   {
      return ITATSimpleDamageableInterface::Execute_CanHandleDamage(actor);
   }

   return false;
}

bool UTATDamageFunctionLibrary::TryApplyEnvironmentalDamageCooldown(AActor* target, TSubclassOf<UGameplayEffect> cooldownEffect, bool allowPseudoPrediction /*= true*/)
{
   if (target == nullptr)
   {
      return false;
   }

   if (!ensure(cooldownEffect))
   {
      return false;
   }

   UAbilitySystemComponent* targetAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(target, false);
   if (targetAsc == nullptr || !(targetAsc->IsOwnerActorAuthoritative() || allowPseudoPrediction))
   {
      return false;
   }

   const UGameplayEffect* cooldownCdo = cooldownEffect.GetDefaultObject();
   check(cooldownCdo);
   const FGameplayTagContainer& cooldownTags = cooldownCdo->GetGrantedTags();

   if (cooldownTags.Num() == 0)
   {
      UE_LOG(LogTATDamageFunctionLibrary, Error, TEXT("TryApplyEnvironmentalDamageCooldown: Cooldown effect %s has no tags"), *cooldownEffect->GetName());
      return false;
   }

   if (targetAsc->HasAnyMatchingGameplayTags(cooldownTags))
   {
      // already in cooldown
      return false;
   }

   FGameplayEffectContextHandle context = targetAsc->MakeEffectContext();
   FGameplayEffectSpec spec(cooldownCdo, context);
   if (targetAsc->IsOwnerActorAuthoritative())
   {
      targetAsc->ApplyGameplayEffectSpecToSelf(spec);
   }
   else
   {
      check(allowPseudoPrediction);

      float duration = 0;
      if (!spec.AttemptCalculateDurationFromDef(duration) || duration <= 0)
      {
         UE_LOG(LogTATDamageFunctionLibrary, Error, TEXT("TryApplyEnvironmentalDamageCooldown: Cooldown effect %s has no duration"), *cooldownEffect->GetName());
         return false;
      }

      // fall back to "fake" client prediction that does not attempt to explicitly synchronize with the server
      const FGameplayTag cooldownTag = cooldownTags.First();
      DamageHelpers::AddLooseTagForDuration(targetAsc, cooldownTag, duration);
   }

   return true;
}
