// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Damage/TATLocalDamageTrackerComponent.h"

// tat
#include "Combat/TATDefenderDamageExecCalculation.h"
#include "Damage/TATDamageFunctionLibrary.h"

// ose
#include "Abilities/Attributes/AttributeBaseSet.h"
#include "Character/OSECharacterBase.h"

// ue5
#include "GameFramework/Controller.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLocalDamageTrackerComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATLocalDamageTrackerComponent, Log, All);

namespace DamageHelpers
{
   static const FGameplayEffectExecutionDefinition* FindDamageExecution(const UGameplayEffect* effectDef)
   {
      check(effectDef);
      return effectDef->Executions.FindByPredicate([](const FGameplayEffectExecutionDefinition& execution) {
         return execution.CalculationClass == UTATDefenderDamageExecCalculation::StaticClass();
      });
   }

   static bool IsDotEffect(const FGameplayEffectSpec& spec)
   {
      if (spec.GetPeriod() == UGameplayEffect::NO_PERIOD || spec.Duration == UGameplayEffect::INSTANT_APPLICATION)
      {
         return false;
      }

      return FindDamageExecution(spec.Def) != nullptr;
   }

   static float CalculateDamageRate(const FGameplayEffectSpec& spec, UAbilitySystemComponent* asc)
   {
      check(IsDotEffect(spec));

      const FGameplayEffectExecutionDefinition* execution = FindDamageExecution(spec.Def);
      check(execution);

      auto applyScopedMods = [&spec, execution](FAggregator& aggregator, const auto& modifierPredicate) {
         for (const FGameplayEffectExecutionScopedModifierInfo& mod : execution->CalculationModifiers)
         {
            if (modifierPredicate(mod))
            {
               float modEvalValue = 0.f;
               if (mod.ModifierMagnitude.AttemptCalculateMagnitude(spec, modEvalValue))
               {
                  aggregator.AddAggregatorMod(modEvalValue, mod.ModifierOp, mod.EvaluationChannelSettings.GetEvaluationChannel(), &mod.SourceTags, &mod.TargetTags, false);
               }
            }
         }
      };

      // partial reverse-engineering of the damage execution calculation
      FAggregatorEvaluateParameters evaluationParameters;
      evaluationParameters.SourceTags = spec.CapturedTargetTags.GetAggregatedTags();
      evaluationParameters.TargetTags = spec.CapturedTargetTags.GetAggregatedTags();

      FAggregator damageAggregator;
      applyScopedMods(damageAggregator, [](const FGameplayEffectExecutionScopedModifierInfo& mod) { return mod.TransientAggregatorIdentifier == TAG_Effect_ExecutionParam_Damage;});
      float damagePerPeriod = damageAggregator.Evaluate(evaluationParameters);
      damagePerPeriod += spec.GetSetByCallerMagnitude(TAG_SetByCaller_Damage, false);

      auto captureAttribute = [&](const FGameplayAttribute& attribute, float& outMagnitude) -> bool {
         FGameplayEffectAttributeCaptureDefinition captureDef(attribute.GetUProperty(), EGameplayEffectAttributeCaptureSource::Target, false);
         FGameplayEffectAttributeCaptureSpec captureSpec(captureDef);
         asc->CaptureAttributeForGameplayEffect(captureSpec);

         FAggregator aggregator;
         if (!captureSpec.AttemptGetAttributeAggregatorSnapshot(aggregator))
         {
            return false;
         }
         applyScopedMods(aggregator, 
            [&attribute](const FGameplayEffectExecutionScopedModifierInfo& mod) {
               return mod.AggregatorType == EGameplayEffectScopedModifierAggregatorType::CapturedAttributeBacked && mod.CapturedAttribute.AttributeToCapture == attribute;
            }
         );
         outMagnitude = aggregator.Evaluate(evaluationParameters);
         return true;
      };

      // There is some simplification here, as the modifier is being sampled directly, rather than captured
      // in the spec, but that may not be present on the client
      float damageMultiplier = 0;
      if (captureAttribute(UAttributeBaseSet::GetDamageReductionMultiplierAttribute(), damageMultiplier))
      {
         damagePerPeriod *= damageMultiplier;
      }

      // account for stacks (even though we typically counter-act it)
      damagePerPeriod *= spec.GetStackCount();

      check(spec.GetPeriod() > 0);
      return damagePerPeriod / spec.Period;
   }
}


UTATLocalDamageTrackerComponent::UTATLocalDamageTrackerComponent()
{
   PrimaryComponentTick.bCanEverTick = false;

   // ...
}


void UTATLocalDamageTrackerComponent::DispatchLocalDamageFromGameplayCue(AController* controller, const FGameplayCueParameters& params)
{
   if (UTATLocalDamageTrackerComponent* damageTracker = GetLocalDamageTrackerForController(controller))
   {
      FTATLocalDamageSource source;
      source.Instigator = params.GetInstigator();
      source.Origin = params.EffectContext.GetOrigin();
      if (const FHitResult* hit = params.EffectContext.GetHitResult())
      {
         source.Normal = hit->ImpactNormal;
      }

      FTATDamageWithType damage;
      damage.DamageAmount = params.RawMagnitude;
      damage.DamageType = UTATDamageFunctionLibrary::ExtractDamageTypeFromContainer(params.AggregatedSourceTags);
      if (!damage.DamageType.IsValid())
      {
         damage.DamageType = TAG_DamageType_Physical;
         UE_LOG(LogTATLocalDamageTrackerComponent, Warning, TEXT("DispatchDamage: Could not find damage type from cue params, falling back to physical (Instigator=%s)"),
            source.Instigator ? *source.Instigator->GetActorNameOrLabel() : TEXT("None"));
      }

      UE_LOG(LogTATLocalDamageTrackerComponent, Verbose, TEXT("DispatchDamage %f %s (Instigator=%s Origin=%s Normal=%s)"),
         damage.DamageAmount, *damage.DamageType.ToString(), (source.Instigator ? *source.Instigator->GetActorNameOrLabel() : TEXT("None")), *source.Origin.ToCompactString(), *source.Normal.ToCompactString());

      damageTracker->OnInstantDamageTaken.Broadcast(damage, source);
   }
}

UTATLocalDamageTrackerComponent* UTATLocalDamageTrackerComponent::GetLocalDamageTrackerForController(AController* controller)
{
   if (controller && controller->IsLocalPlayerController())
   {
      return controller->FindComponentByClass<UTATLocalDamageTrackerComponent>();
   }
   return nullptr;
}

void UTATLocalDamageTrackerComponent::RemoveListenersFrom(const UObject* source)
{
   OnInstantDamageTaken.RemoveAll(source);
   OnDamageOverTimeChanged.RemoveAll(source);
}

float UTATLocalDamageTrackerComponent::GetMaxHealth() const
{
   if (_abilitySystemComponent)
   {
      return _abilitySystemComponent->GetNumericAttribute(UAttributeBaseSet::GetHealthMaxAttribute());
   }

   return 0.f;
}

void UTATLocalDamageTrackerComponent::BeginPlay()
{
   Super::BeginPlay();

   if (AController* controllerOwner = GetOwner<AController>())
   {
      if (controllerOwner->IsLocalController())
      {
         _OnPawnChanged(nullptr, controllerOwner->GetPawn());
         controllerOwner->OnPossessedPawnChanged.AddUniqueDynamic(this, &UTATLocalDamageTrackerComponent::_OnPawnChanged);
      }
   }
}

const FTATLocalDamageOverTimeEntry* UTATLocalDamageTrackerComponent::_FindEntryForHandle(FActiveGameplayEffectHandle effectHandle) const
{
   return _activeDotEffects.FindByKey(effectHandle);
}

FTATLocalDamageOverTimeEntry* UTATLocalDamageTrackerComponent::_FindEntryForHandle(FActiveGameplayEffectHandle effectHandle)
{
   return _activeDotEffects.FindByKey(effectHandle);
}

void UTATLocalDamageTrackerComponent::_OnPawnChanged(APawn* oldPawn, APawn* newPawn)
{
   // Cleanup old Asc
   _currentCharacter = nullptr;
   if (_abilitySystemComponent)
   {
      _abilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
      _abilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);

      for (const FActiveGameplayEffectHandle& handle : _trackedDotHandles)
      {
         if (FActiveGameplayEffectEvents* events = _abilitySystemComponent->GetActiveEffectEventSet(handle))
         {
            events->OnInhibitionChanged.RemoveAll(this);
            events->OnStackChanged.RemoveAll(this);
         }
      }

      if (_activeDotEffects.Num() > 0)
      {
         _activeDotEffects.Reset();
         OnDamageOverTimeChanged.Broadcast();
      }

      _abilitySystemComponent = nullptr;
   }

   if (AOSECharacterBase* character = Cast<AOSECharacterBase>(newPawn))
   {
      _currentCharacter = character;
      character->CallOrRegisterAbilitiesInitializedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UTATLocalDamageTrackerComponent::_OnAbilitiesInitialized));
   }
}

void UTATLocalDamageTrackerComponent::_OnAbilitiesInitialized()
{
   if (_currentCharacter == nullptr || _abilitySystemComponent != nullptr)
   {
      return;
   }

   UAbilitySystemComponent* asc = _currentCharacter->GetAbilitySystemComponent();
   if (asc == nullptr)
   {
      return;
   }

   _abilitySystemComponent = asc;
   asc->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UTATLocalDamageTrackerComponent::_OnEffectAdded);
   asc->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UTATLocalDamageTrackerComponent::_OnEffectRemoved);
   for (FActiveGameplayEffectsContainer::ConstIterator iter = asc->GetActiveGameplayEffects().CreateConstIterator(); iter; ++iter)
   {
      _OnEffectAdded(asc, iter->Spec, iter->Handle);
   }
}

void UTATLocalDamageTrackerComponent::_OnEffectAdded(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle effectHandle)
{
   check(asc);
   if (!DamageHelpers::IsDotEffect(spec))
   {
      return;
   }

   _trackedDotHandles.Add(effectHandle);

   FActiveGameplayEffectEvents* events = asc->GetActiveEffectEventSet(effectHandle);
   check(events);
   events->OnInhibitionChanged.AddUObject(this, &UTATLocalDamageTrackerComponent::_OnEffectInhibitionChanged);
   events->OnStackChanged.AddUObject(this, &UTATLocalDamageTrackerComponent::_OnEffectStackChanged);

   const FActiveGameplayEffect* activeEffect = asc->GetActiveGameplayEffect(effectHandle);
   check(activeEffect);
   if (!activeEffect->bIsInhibited)
   {
      _AddActiveDotEffect(spec, effectHandle);
   }
}

void UTATLocalDamageTrackerComponent::_OnEffectInhibitionChanged(FActiveGameplayEffectHandle effectHandle, bool isInhibited)
{
   if (isInhibited)
   {
      _TryRemoveActiveDotEffect(effectHandle);
      return;
   }
   else
   {
      // The inhibition event may fire right after the effect is added, so defend against double adds
      // (but still want to add in the on-add, so late binds stay in the same code-path)
      const FActiveGameplayEffect* activeEffect = _abilitySystemComponent->GetActiveGameplayEffect(effectHandle);
      if (ensure(activeEffect) && _FindEntryForHandle(effectHandle) == nullptr)
      {
         _AddActiveDotEffect(activeEffect->Spec, effectHandle);
      }
   }
}

void UTATLocalDamageTrackerComponent::_OnEffectStackChanged(FActiveGameplayEffectHandle effectHandle, int32 newStackCount, int32 oldStackCount)
{
   const FActiveGameplayEffect* activeEffect = _abilitySystemComponent->GetActiveGameplayEffect(effectHandle);
   FTATLocalDamageOverTimeEntry* entry = _FindEntryForHandle(effectHandle);
   if (entry && ensure(activeEffect))
   {
      const float newDamagePerSecond = DamageHelpers::CalculateDamageRate(activeEffect->Spec, _abilitySystemComponent);
      if (newDamagePerSecond != entry->DamagePerSecond)
      {
         entry->DamagePerSecond = newDamagePerSecond;
         UE_LOG(LogTATLocalDamageTrackerComponent, Verbose, TEXT("UpdateStacks DoT effect (%s) Damage -> %f"), *effectHandle.ToString(), newDamagePerSecond);
         OnDamageOverTimeChanged.Broadcast();
      }
      else
      {
         UE_LOG(LogTATLocalDamageTrackerComponent, Verbose, TEXT("UpdateStacks DoT effect (%s) Damage remained %f"), *effectHandle.ToString(), newDamagePerSecond);
      }
   }
}

void UTATLocalDamageTrackerComponent::_TryRemoveActiveDotEffect(FActiveGameplayEffectHandle effectHandle)
{
   const int32 removedCount = _activeDotEffects.RemoveAll([effectHandle](const FTATLocalDamageOverTimeEntry& entry) { return entry.SourceEffect == effectHandle; });
   if (removedCount > 0)
   {
      UE_LOG(LogTATLocalDamageTrackerComponent, Verbose, TEXT("Removed DoT effect (%s)"), *effectHandle.ToString());
      OnDamageOverTimeChanged.Broadcast();
   }
}

void UTATLocalDamageTrackerComponent::_AddActiveDotEffect(const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle effectHandle)
{
   check(_FindEntryForHandle(effectHandle) == nullptr);
   FTATLocalDamageOverTimeEntry& entry = _activeDotEffects.Emplace_GetRef();
   entry.SourceEffect = effectHandle;
   entry.DamagePerSecond = DamageHelpers::CalculateDamageRate(spec, _abilitySystemComponent);

   // Use tags on the effect def, as the combined tags on the spec are not initialized on clients
   entry.DamageType = UTATDamageFunctionLibrary::ExtractDamageTypeFromContainer(spec.Def->GetAssetTags());
   if (!entry.DamageType.IsValid())
   {
      entry.DamageType = TAG_DamageType_Physical;
      UE_LOG(LogTATLocalDamageTrackerComponent, Warning, TEXT("DoT: Could not find damage type from source tags, falling back to physical (%s)"), *spec.ToSimpleString());
   }

   UE_LOG(LogTATLocalDamageTrackerComponent, Verbose, TEXT("Added DoT effect (%s) DamagePerSecond: %f Type: %s"), *effectHandle.ToString(), entry.DamagePerSecond, *entry.DamageType.ToString());
   OnDamageOverTimeChanged.Broadcast();
}

void UTATLocalDamageTrackerComponent::_OnEffectRemoved(const FActiveGameplayEffect& activeEffect)
{
   // NB: no need to remove listeners here, as the event itself will be destroyed synchronously
   // try to remove without checking if it was DoT
   _TryRemoveActiveDotEffect(activeEffect.Handle);
   _trackedDotHandles.RemoveSingleSwap(activeEffect.Handle);
}

