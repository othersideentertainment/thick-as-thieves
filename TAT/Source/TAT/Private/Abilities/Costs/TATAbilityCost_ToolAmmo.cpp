// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Costs/TATAbilityCost_ToolAmmo.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Tools/TATToolComponent.h"

// ose
#include "Items/ToolSetInterface.h"

// ue5
#include "Abilities/GameplayAbility.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAbilityCost_ToolAmmo)

DEFINE_LOG_CATEGORY_STATIC(LogTATAbilityCost_ToolAmmo, Log, All)

bool UTATAbilityCost_ToolAmmo::CanAfford(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, FGameplayTagContainer* optionalRelevantTags) const
{
   if (UTATToolComponent* toolComponent = _GetAssociatedTool(ability, handle, actorInfo))
   {
      if (toolComponent->HasEnoughAmmoToUse(AmmoCost))
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

void UTATAbilityCost_ToolAmmo::ApplyCost(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo) const
{
   // Only trigger the cost on authority, and unless we're only checking the cost w/o applying it
   if (!CheckOnly && ability->HasAuthority(&activationInfo))
   {
      if (UTATToolComponent* toolComponent = _GetAssociatedTool(ability, handle, actorInfo))
      {
         bool hadEnoughToUse = toolComponent->AuthorityUseAmmo(AmmoCost);

         if (!hadEnoughToUse)
         {
            UE_LOG(LogTATAbilityCost_ToolAmmo, Warning,
               TEXT("Ability '%s' tried to apply ammo cost on tool '%s', but it didn't have enough ammo. Possibly a timing issue"),
               *GetNameSafe(ability), *GetNameSafe(toolComponent));
         }
      }
   }
}

UTATToolComponent* UTATAbilityCost_ToolAmmo::_GetAssociatedTool(const UGameplayAbility* ability, const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo) const
{
   if (ToolCostType == ETATToolCostType::SourceTool)
   {
      if (UTATToolComponent* currentTool = Cast<UTATToolComponent>(ability->GetSourceObject(handle, actorInfo)))
      {
         return currentTool;
      }
      else
      {
         UE_LOG(LogTATAbilityCost_ToolAmmo, Error,
            TEXT("Ability '%s' tried to check/apply ammo cost, but it was not associated with a tool"),
            *GetNameSafe(ability));
         return nullptr;
      }
   }
   else if (ToolCostType == ETATToolCostType::SpecificToolCategory)
   {
      if (TScriptInterface<IToolSetInterface> toolsetInterface = IToolSetInterface::GetToolSetFromActor(actorInfo->AvatarActor.Get()))
      {
         return Cast<UTATToolComponent>(toolsetInterface->GetToolByCategory(ToolCategoryToCostAmmo, true));
      }
      else
      {
         // Don't log an error here: since it's not tied to our current tool we may have situations where we don't
         // have the tool present
         return nullptr;
      }
   }
   else
   {
      checkNoEntry();
      return nullptr;
   }
}

