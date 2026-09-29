// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/CombatConsiderationInput.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Combat/CombatComponent.h"
#include "Combat/CombatFunctionLibrary.h"
#include "Combat/CombatSettings.h"
#include "Items/ToolRangedWeaponComponent.h"
#include "Items/ToolSetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CombatConsiderationInput)

// ue4

UCombatComponent* UCombatConsiderationInput_Base:: _GetMyCombatComponent(const FConsiderationContext& ctx) const
{
   return ctx.Character->GetCombatComponent();
}

UCombatComponent* UCombatConsiderationInput_Base::_GetTargetCombatComponent(const FConsiderationContext& ctx) const
{
   if (AOSECharacterBase* character = Cast<AOSECharacterBase>(ctx.Target.Actor))
   {
      return character->GetCombatComponent();
   }
   return nullptr;
}

float UCombatConsiderationInput_TimeSinceLastAttackInitiated::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceLastAttackInitiated);
   check(MaxSecondsSinceLastAttackInitiated >= 0.0f);

   if (UCombatComponent* combatComp = _GetMyCombatComponent(ctx))
   {
      return FMath::Clamp(combatComp->SecondsSinceLastAttackInitiated(), 0.0f, MaxSecondsSinceLastAttackInitiated) / MaxSecondsSinceLastAttackInitiated;
   }
   return 0.0f;
}

float UCombatConsiderationInput_TimeSinceTargetLastAttackInitiated::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceTargetLastAttackInitiated);
   check(MaxSecondsSinceLastAttackInitiated >= 0.0f);

   if (UCombatComponent* combatComp = _GetTargetCombatComponent(ctx))
   {
      return FMath::Clamp(combatComp->SecondsSinceLastAttackInitiated(), 0.0f, MaxSecondsSinceLastAttackInitiated) / MaxSecondsSinceLastAttackInitiated;
   }
   return 0.0f;
}

float UCombatConsiderationInput_TimeSinceCombatDisabled::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceCombatDisabled);
   check(MaxSecondsSinceCombatDisabled >= 0.0f);

   if (UCombatComponent* combatComp = _GetMyCombatComponent(ctx))
   {
      if (combatComp->HasCombatDisabledStatus())
         return FMath::Clamp(combatComp->SecondsSinceCombatDisabledStatusGained(), 0.0f, MaxSecondsSinceCombatDisabled) / MaxSecondsSinceCombatDisabled;
      return 1.0f;
   }
   return 0.0f;
}

float UCombatConsiderationInput_TimeSinceTargetCombatDisabled::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceTargetCombatDisabled);
   check(MaxSecondsSinceCombatDisabled >= 0.0f);

   if (UCombatComponent* combatComp = _GetTargetCombatComponent(ctx))
   {
      if (combatComp->HasCombatDisabledStatus())
         return FMath::Clamp(combatComp->SecondsSinceCombatDisabledStatusGained(), 0.0f, MaxSecondsSinceCombatDisabled) / MaxSecondsSinceCombatDisabled;
      return 1.0f;
   }
   return 0.0f;
}

float UCombatConsiderationInput_RangedWeaponNeedsReload::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(RangedWeaponNeedsReload);
   if (TScriptInterface<IToolSetInterface> toolSet = ctx.Character->GetToolSetInterface())
   {
      if (auto rangedTool = Cast<UToolRangedWeaponComponent>(toolSet->GetToolByCategory(ToolCategory, false)))
      {
         return rangedTool->NeedsReload() ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UCombatConsiderationInput_RangedWeaponCanReload::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(RangedWeaponCanReload);
   if (TScriptInterface<IToolSetInterface> toolSet = ctx.Character->GetToolSetInterface())
   {
      if (auto rangedTool = Cast<UToolRangedWeaponComponent>(toolSet->GetToolByCategory(ToolCategory, false)))
      {
         return rangedTool->CanReload() ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UCombatConsiderationInput_RangedWeaponHasRequiredProjectilesForActivation::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(RangedWeaponHasRequiredProjectilesForActivation);
   if (TScriptInterface<IToolSetInterface> toolSet = ctx.Character->GetToolSetInterface())
   {
      if (auto rangedTool = Cast<UToolRangedWeaponComponent>(toolSet->GetToolByCategory(ToolCategory, false)))
      {
         return rangedTool->HasRequiredProjectilesForActivation() ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UCombatConsiderationInput_TargetInRangedWeaponAttackRange::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetInRangedWeaponAttackRange);
   if (TScriptInterface<IToolSetInterface> toolSet = ctx.Character->GetToolSetInterface())
   {
      if (auto rangedTool = Cast<UToolRangedWeaponComponent>(toolSet->GetToolByCategory(ToolCategory, false)))
      {
         if (AActor* targetActor = ctx.Target.Actor.Get())
         {
            FVector targetLocation = targetActor->GetActorLocation();
            return rangedTool->TargetWithinRange(targetLocation) ? 1.0f : 0.0f;
         }
      }
   }
   return 0.0f;
}

float UCombatConsiderationInput_TargetHasValidHitPath::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetHasValidHitPath);
   const UCombatSettings& settings = UCombatSettings::Get();
   const bool hasPath = UCombatFunctionLibrary::DoesAttackerHaveHitPathToDefender(ctx.Character, ctx.Target.Actor.Get(), settings.CombatAttackerToDefenderPathTraceSphereRadius);
   return hasPath ? 1.0f : 0.0f;
}

