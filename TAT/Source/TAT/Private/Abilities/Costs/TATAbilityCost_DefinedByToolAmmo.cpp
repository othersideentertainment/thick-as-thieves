// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Costs/TATAbilityCost_DefinedByToolAmmo.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Tools/TATToolComponent.h"

// ue5
#include "Abilities/GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAbilityCost_DefinedByToolAmmo)

DEFINE_LOG_CATEGORY_STATIC(LogTATAbilityCost_DefinedByToolAmmo, Log, All)

bool UTATAbilityCost_DefinedByToolAmmo::CanAfford(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle,
                                                  const FGameplayAbilityActorInfo* actorInfo, FGameplayTagContainer* optionalRelevantTags) const
{
   if (UTATToolComponent* toolComponent = _GetAssociatedTool(ability, handle, actorInfo))
   {
      if (toolComponent->HasEnoughAmmoToUse(toolComponent->GetToolCostByUsageType(ToolUsageTag)))
      {
         return true;
      }
   }

   const FGameplayTag& failureTag = UTATProjectSettings::Get().AmmoCostActivationFailureTag;
   if (optionalRelevantTags && failureTag.IsValid())
   {
      optionalRelevantTags->AddTag(failureTag);
   }

   return false;
}

void UTATAbilityCost_DefinedByToolAmmo::ApplyCost(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle,
   const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) const
{
   // Only trigger the cost on authority, and unless we're only checking the cost w/o applying it
   if (!CheckOnly && ability->HasAuthority(&activationInfo))
   {
      if (UTATToolComponent* toolComponent = _GetAssociatedTool(ability, handle, actorInfo))
      {
         bool hadEnoughToUse = toolComponent->AuthorityUseAmmo(toolComponent->GetToolCostByUsageType(ToolUsageTag));

         if (!hadEnoughToUse)
         {
            UE_LOG(LogTATAbilityCost_DefinedByToolAmmo, Warning,
               TEXT("Ability '%s' tried to apply ammo cost on tool '%s', but it didn't have enough ammo. Possibly a timing issue"),
               *GetNameSafe(ability), *GetNameSafe(toolComponent));
         }
      }
   }
}

UTATToolComponent* UTATAbilityCost_DefinedByToolAmmo::_GetAssociatedTool(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle,
   const FGameplayAbilityActorInfo* actorInfo) const
{
   if (UTATToolComponent* currentTool = Cast<UTATToolComponent>(ability->GetSourceObject(handle, actorInfo)))
   {
      return currentTool;
   }
   else
   {
      UE_LOG(LogTATAbilityCost_DefinedByToolAmmo, Error,
         TEXT("Ability '%s' tried to check/apply ammo cost, but it was not associated with a tool"),
         *GetNameSafe(ability));
      return nullptr;
   }
}
