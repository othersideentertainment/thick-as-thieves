// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/TATCombatFunctionLibrary.h"

// tat
#include "Abilities/SyncedAnimation/TATGameplayAbility_SyncedSelect.h"
#include "Abilities/TATGameplayAbilityTargetData_Dodge.h"
#include "Combat/TATCombatComponent.h"
#include "Combat/TATCombatSettings.h"
#include "Developer/TATProjectSettings.h"

// ose
#include "Abilities/OSEGameplayAbility_SyncedAnimationPlayer.h"
#include "Abilities/TargetActors/ConeTargetHelpers.h"
#include "Character/OSECharacterBase.h"
#include "Combat/CombatFunctionLibrary.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCombatFunctionLibrary)
DEFINE_LOG_CATEGORY_STATIC(LogTATCombatFunctionLibrary, Log, All);

TAutoConsoleVariable<int32> CVarTATStealthTakedownsDebugDrawConeTrace(
   TEXT("TAT.StealthTakedowns.DebugDrawConeTrace"),
   0,
   TEXT("(default 0) Whether or not to draw debug visuals for the cone trace for stealth takedowns")
);

namespace TATCombatUtl
{
   bool ActorHasTag(const AActor* actor, const FGameplayTag& tagCheck)
   {
      if (auto tagInterface = Cast<IGameplayTagAssetInterface>(actor))
      {
         return tagInterface->HasMatchingGameplayTag(tagCheck);
      }
      return false;
   }

   bool IsDefenderDefendingAgainstTag(const AActor* attacker, const AActor* defender, const FGameplayTag& tagCheck)
   {
      if (!attacker || !defender)
         return false;

      // first check is that the defender has the attacker in front of them
      if (UTATCombatFunctionLibrary::IsDefenderInPositionToBlockAttacker(attacker, defender))
      {
         // second check is that the defender is actually blocking
         return ActorHasTag(defender, tagCheck);
      }

      return false;
   }
}

bool UTATCombatFunctionLibrary::IsDefenderShielded(const AActor* defender)
{
   const UTATCombatSettings& settings = UTATCombatSettings::Get();
   return TATCombatUtl::ActorHasTag(defender, settings.IsShieldedTag);
}

bool UTATCombatFunctionLibrary::IsDefenderParrying(const AActor* attacker, const AActor* defender)
{
   const UTATCombatSettings& settings = UTATCombatSettings::Get();
   return TATCombatUtl::IsDefenderDefendingAgainstTag(attacker, defender, settings.IsParryingTag);
}

bool UTATCombatFunctionLibrary::IsDefenderInPositionToBlockAttacker(const AActor* attacker, const AActor* defender)
{
   // intentionally calling this backwards, we want to see if the attacker is in front of the defender.  TODO: maybe different rename?
   return UCombatFunctionLibrary::IsDefenderInFrontOfAttacker(defender, attacker);
}

bool UTATCombatFunctionLibrary::IsDefenderBlocking(const AActor* attacker, const AActor* defender)
{
   const UTATCombatSettings& settings = UTATCombatSettings::Get();
   return TATCombatUtl::IsDefenderDefendingAgainstTag(attacker, defender, settings.IsBlockingTag);
}

bool UTATCombatFunctionLibrary::IsDefenderVulnerableToCounterByAttacker(const AActor* attacker, const AActor* defender)
{
   const UTATCombatSettings& settings = UTATCombatSettings::Get();
   if (UAbilitySystemComponent* defenderAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(defender))
   {
      if (UAbilitySystemComponent* attackerAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(attacker))
      {
         TArray<FActiveGameplayEffectHandle> activeVulnToCounterGEs = defenderAsc->GetActiveEffectsWithAllTags(settings.IsVulnerableToCounterTag.GetSingleTagContainer());
         for (FActiveGameplayEffectHandle activeVuln : activeVulnToCounterGEs)
         {
            // If the defender asc has a vulnerable GE that was instigated by the attacker asc, then this is a counter
            const UGameplayEffect* vulnGE = defenderAsc->GetGameplayEffectCDO(activeVuln);
            if (defenderAsc->GetGameplayEffectCount(vulnGE->GetClass(), attackerAsc) > 0)
            {
               return true;
            }
         }
      }
   }

   return false;
}

bool UTATCombatFunctionLibrary::IsBlocking(const AActor* actor)
{
   const UTATCombatSettings& settings = UTATCombatSettings::Get();
   return TATCombatUtl::ActorHasTag(actor, settings.IsBlockingTag);
}

bool UTATCombatFunctionLibrary::IsPerformingChargeAttack(const AActor* actor)
{
   const UTATCombatSettings& settings = UTATCombatSettings::Get();
   return TATCombatUtl::ActorHasTag(actor, settings.IsPerformingChargeAttackTag);
}

bool UTATCombatFunctionLibrary::IsChargingMeleeAttack(const AActor* actor)
{
   const UTATCombatSettings& settings = UTATCombatSettings::Get();
   return TATCombatUtl::ActorHasTag(actor, settings.IsChargingMeleeAttackTag);
}

bool UTATCombatFunctionLibrary::IsPerformingShove(const AActor* actor)
{
   const UTATCombatSettings& settings = UTATCombatSettings::Get();
   return TATCombatUtl::ActorHasTag(actor, settings.IsPerformingShoveTag);
}

bool UTATCombatFunctionLibrary::CanBeSneakAttackedFromPosition(const AActor* defender, FVector attackerPosition)
{
   // The base functionality we're starting with is to only check if we are facing the actor attacking us
   // To do that, see if the angle b/w our forward vector, and the vector from us to them, is more than half
   // the "facing angle" arc width defined in the settings
   const float cosineHalfMaxAngleDegrees = UTATProjectSettings::Get().GetCosineSneakAttackFacingHalfAngle();

   const FVector defenderToAttackerDirection = (attackerPosition - defender->GetActorLocation()).GetSafeNormal2D();
   const FVector defenderForwardDireaction = defender->GetActorForwardVector();

   const float attackingDotProduct = FVector::DotProduct(defenderToAttackerDirection, defenderForwardDireaction);

   return attackingDotProduct < -cosineHalfMaxAngleDegrees;
}

AActor* UTATCombatFunctionLibrary::FindStealthTakedownTarget(const AActor* attacker, const TArray<UOSESyncedAnimationDataAsset*>& takedownSyncedAnimations, float traceDistance, float traceConeHalfAngle, FCollisionProfileName traceProfile)
{
   if (!ensure(attacker))
   {
      return nullptr;
   }

   ConeTargetHelpers::FConeTraceParams params;
   params.SourceActor = attacker;
   params.MaxRange = traceDistance;
   params.HalfAngle = traceConeHalfAngle;
   params.LineOfSightProfile = traceProfile;
   params.ObjectQueryParams = FCollisionObjectQueryParams(ECC_Pawn);
   params.DrawDebug = (CVarTATStealthTakedownsDebugDrawConeTrace.GetValueOnGameThread() != 0);
   auto filter = [&](const AActor* possibleTarget)
   {
      return UTATCombatFunctionLibrary::CanPerformStealthTakedownAgainst(attacker, possibleTarget, takedownSyncedAnimations);
   };

   ConeTargetHelpers::FConeTraceResult result = ConeTargetHelpers::DoConeTrace(params, filter);
   return result.FoundActor;
}

bool UTATCombatFunctionLibrary::CanPerformStealthTakedownAgainst(const AActor* attacker, const AActor* possibleTarget, const TArray<UOSESyncedAnimationDataAsset*>& takedownSyncedAnimations)
{
   if (!IsValidTakedownDefender(possibleTarget))
   {
      return false;
   }

   const UOSESyncedAnimationDataAsset* possibleTakedownAnim = UTATGameplayAbility_SyncedSelect::FindMatchingAnimation(takedownSyncedAnimations, attacker, possibleTarget);
   return possibleTakedownAnim != nullptr;
}

bool UTATCombatFunctionLibrary::PerformStealthTakedown(const AActor* attacker, const AActor* defender, FGameplayTag takedownEventTag, const TArray<UOSESyncedAnimationDataAsset*>& takedownSyncedAnimations)
{
   if (!ensure(attacker) || !ensure(defender))
   {
      return false;
   }

   if (!IsValidTakedownDefender(defender))
   {
      return false;
   }

   const UOSESyncedAnimationDataAsset* stealthTakedownAnim = UTATGameplayAbility_SyncedSelect::FindMatchingAnimation(takedownSyncedAnimations, attacker, defender);

   if (!stealthTakedownAnim)
   {
      return false;
   }

   FGameplayEventData eventData;
   eventData.Instigator = attacker;
   eventData.Target = defender;
   eventData.OptionalObject = stealthTakedownAnim;

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(attacker))
   {
      FScopedPredictionWindow newScopedWindow(asc, true);
      asc->HandleGameplayEvent(takedownEventTag, &eventData);
      return true;
   }
   else
   {
      return false;
   }
}

bool UTATCombatFunctionLibrary::IsValidTakedownDefender(const AActor* defender)
{
   if (auto defenderTags = Cast<IGameplayTagAssetInterface>(defender))
   {
      return !defenderTags->HasAnyMatchingGameplayTags(UTATProjectSettings::Get().InvalidTakedownTargetTags);
   }

   return false;
}

void UTATCombatFunctionLibrary::GetDodgeInfoFromTargetData(const APawn* dodgingPawn, const FGameplayAbilityTargetDataHandle& targetDataHandle, bool& success, FVector& dodgeEndLocation, FVector& dodgeDirection, bool& isBackstep)
{
   success = false;
   dodgeEndLocation = FVector::ZeroVector;
   isBackstep = false;

   if (!dodgingPawn)
   {
      UE_LOG(LogTATCombatFunctionLibrary, Error, TEXT("GetDodgeInfoFromTargetData() called with invalid dodgingPawn!"));
      return;
   }

   const int32 index = 0;
   const FGameplayAbilityTargetData* targetData = targetDataHandle.Get(index);
   if (!targetData)
   {
      UE_LOG(LogTATCombatFunctionLibrary, Error, TEXT("GetDodgeInfoFromTargetData() called with invalid target data!"));
      return;
   }

   if(targetData->GetScriptStruct() != FTATGameplayAbilityTargetData_Dodge::StaticStruct())
   {
      UE_LOG(LogTATCombatFunctionLibrary, Error, TEXT("GetDodgeInfoFromTargetData() called with non-FTATGameplayAbilityTargetData_Dodge target data!"));
      return;
   }
   const UTATCombatSettings& combatSettings = UTATCombatSettings::Get();

   // Should be safe after verifying the above, cast and pull out the data
   const FTATGameplayAbilityTargetData_Dodge* dodgeTargetData = static_cast<const FTATGameplayAbilityTargetData_Dodge*>(targetData);
   check(dodgeTargetData);

   success = true;
   dodgeEndLocation = dodgeTargetData->GetEndPoint();
   dodgeDirection = dodgeTargetData->DodgeDirection;
   isBackstep = dodgeTargetData->IsBackstep;
}

UTATCombatComponent* UTATCombatFunctionLibrary::GetTATCombatComponent(const AActor* actor)
{
   if (const AOSECharacterBase* character = Cast<AOSECharacterBase>(actor))
   {
      return Cast<UTATCombatComponent>(character->GetCombatComponent());
   }
   UE_LOG(LogTATCombatFunctionLibrary, Error, TEXT("GetCombatComponent() called with invalid character!"));
   return nullptr;
}
