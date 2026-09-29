// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/CombatFunctionLibrary.h"

// ose
#include "Combat/CombatSettings.h"

// ue4
#include "DrawDebugHelpers.h"
#include "GameplayTagAssetInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CombatFunctionLibrary)

namespace CombatUtlCVars
{
   static int DebugDrawHitPath = 0;
   FAutoConsoleVariableRef CVarDebugDrawHitPath(
      TEXT("OSE.Combat.DebugDrawHitPath"),
      DebugDrawHitPath,
      TEXT("Draw combat hit path checks"),
      ECVF_Default);
};

namespace CombatUtl
{
   bool ActorHasTag(const AActor* actor, const FGameplayTag& tagCheck)
   {
      if (auto tagInterface = Cast<IGameplayTagAssetInterface>(actor))
      {
         return tagInterface->HasMatchingGameplayTag(tagCheck);
      }
      return false;
   }

   bool CompareDotProducts(float dot, float tolerance)
   {
      if (tolerance == float(INDEX_NONE))
      {
         return dot >= 0.0f;
      }
      else
      {
         return dot >= 0.0f && dot >= (1.0f - tolerance);
      }
   }
}

bool UCombatFunctionLibrary::IsDefenderInFrontOfAttacker(const AActor* attacker, const AActor* defender, float dotTolerance /* = float(INDEX_NONE) */)
{
   if (!attacker || !defender)
      return false;
   
   // using look rot instead of actor rot to determine fwd

   FVector attackerLoc;
   FRotator attackerRot;
   attacker->GetActorEyesViewPoint(attackerLoc, attackerRot);
   FVector attackerFwd = attackerRot.Vector();

   FVector defenderLoc;
   FRotator defenderRot;
   defender->GetActorEyesViewPoint(defenderLoc, defenderRot);
   FVector defenderFwd = (defenderLoc - attackerLoc);
   defenderFwd.Normalize();

   float dot = attackerFwd | defenderFwd;
   return CombatUtl::CompareDotProducts(dot, dotTolerance);
}

bool UCombatFunctionLibrary::IsLocationInFrontOfAttacker(const AActor* attacker, const FVector& location, float dotTolerance /*= -1.0f*/)
{
   if (!attacker)
      return false;

   // using look rot instead of actor rot to determine fwd

   FVector attackerLoc;
   FRotator attackerRot;
   attacker->GetActorEyesViewPoint(attackerLoc, attackerRot);
   FVector attackerFwd = attackerRot.Vector().GetSafeNormal2D();
   FVector defenderFwd = (location - attackerLoc).GetSafeNormal2D();

   const float dot = attackerFwd | defenderFwd;
   return CombatUtl::CompareDotProducts(dot, dotTolerance);
}

static FVector GetDefenderTracePositionPerLogic(const AActor* defender, EOSECombatDefenderTraceLogic traceLogic)
{
   switch (traceLogic)
   {
   case EOSECombatDefenderTraceLogic::TraceToCenter: {
      // Not just using `defender->GetActorLocation()` here, as illusions have their origin at their feet
      // If we keep running into issues because of illusion's not having their origin at their center, then it may be worth revisiting that, but for now there is still utility in terms of ease of placement
      FVector defenderOrigin, defenderExtent;
      defender->GetActorBounds(true, defenderOrigin, defenderExtent);

      return defenderOrigin;
   }

   case EOSECombatDefenderTraceLogic::TraceToEyes: {
      FVector defenderEyesLocation;
      FRotator unusedRotation;
      defender->GetActorEyesViewPoint(defenderEyesLocation, unusedRotation);
      return defenderEyesLocation;
   }

   default:
      checkNoEntry();
      return FVector::ZeroVector;
   }
}

bool UCombatFunctionLibrary::DoesAttackerHaveHitPathToDefender(const AActor* attacker, const AActor* defender, float radius, EOSECombatDefenderTraceLogic traceLogic /*= EOSECombatDefenderTraceLogic::TraceToCenter*/)
{
   if (!attacker || !defender)
      return false;

   const FVector defenderTraceLocation = GetDefenderTracePositionPerLogic(defender, traceLogic);
   return DoesAttackerHaveHitPathToDefenderLocation(attacker, defender, defenderTraceLocation, radius);
}

bool UCombatFunctionLibrary::DoesAttackerHaveHitPathToDefenderLocation(const AActor* attacker, const AActor* defender, const FVector& location, float radius)
{
   UWorld* world = attacker->GetWorld();
   check(world);

   const UCombatSettings& settings = UCombatSettings::Get();

   FVector attackerViewLocation;
   FRotator attackerViewRotation;
   attacker->GetActorEyesViewPoint(attackerViewLocation, attackerViewRotation);

   static const bool kTraceComplex = false;
   FCollisionQueryParams params(SCENE_QUERY_STAT(UCombatFunctionLibrary_DoesAttackerHaveHitPathToDefender), kTraceComplex);
   params.bReturnPhysicalMaterial = false; // don't need this
   params.bIgnoreTouches = true; // no overlaps, only blocks
   params.AddIgnoredActor(attacker);

   FVector startPoint = attackerViewLocation;
   FVector endPoint = location;

   FHitResult outHit;
   const TArray<AActor*> objectsToIgnore = { const_cast<AActor*>(attacker) };
   EDrawDebugTrace::Type debugDrawType = EDrawDebugTrace::None;
   const bool ignoreSelf = true;
   const bool traceComplex = false;
   if (CombatUtlCVars::DebugDrawHitPath)
   {
      debugDrawType = EDrawDebugTrace::ForDuration;
   }
   UKismetSystemLibrary::SphereTraceSingleByProfile(attacker, startPoint, endPoint, radius, settings.CombatHitPathTraceProfile.Name, traceComplex, objectsToIgnore, debugDrawType, outHit, ignoreSelf, FColor::Red, FColor::Green, 2.0f);

   const bool isVisible = !outHit.bBlockingHit || (outHit.bBlockingHit && outHit.GetActor() == defender);
   return isVisible;
}

bool UCombatFunctionLibrary::IsHeavyAttackReadied(const AActor* actor)
{
   const UCombatSettings& settings = UCombatSettings::Get();
   return CombatUtl::ActorHasTag(actor, settings.IsHeavyAttackReadiedTag);
}

