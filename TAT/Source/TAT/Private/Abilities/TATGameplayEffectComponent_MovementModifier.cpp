// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATGameplayEffectComponent_MovementModifier.h"

// tat
#include "Character/TATCharacterMovement.h"

// ose
#include "Player/OSEPlayerState.h"

// ue
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayEffectComponent_MovementModifier)


namespace MovementModHelpers
{
   ETATCharacterMovementModifierOp Convert(ETATGameplayEffectComponent_MovementModifierOp op)
   {
      switch (op)
      {
      case ETATGameplayEffectComponent_MovementModifierOp::ClampMax:
         return ETATCharacterMovementModifierOp::ClampMax;
      default:
         checkNoEntry();
         break;
      }
      return ETATCharacterMovementModifierOp::ClampMax;
   }

   ETATCharacterMovementModifierStat Convert(ETATGameplayEffectComponent_MovementModifierStat stat)
   {
      switch (stat)
      {
      case ETATGameplayEffectComponent_MovementModifierStat::Friction:
         return ETATCharacterMovementModifierStat::Friction;
      case ETATGameplayEffectComponent_MovementModifierStat::BrakingDeceleration:
         return ETATCharacterMovementModifierStat::BrakingDeceleration;
      default:
         break;
      }
      return ETATCharacterMovementModifierStat::None;
   }
}

UTATGameplayEffectComponent_MovementModifier::UTATGameplayEffectComponent_MovementModifier()
{
#if WITH_EDITORONLY_DATA
   EditorFriendlyName = TEXT("[TAT] Movement Modifier (Experimental)");
#endif
}

bool UTATGameplayEffectComponent_MovementModifier::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& activeGEContainer, FActiveGameplayEffect& activeGE) const
{
   if (ACharacter* character = _GetOwnerCharacter(activeGEContainer.Owner))
   {
      if (UTATCharacterMovement* cmc = Cast<UTATCharacterMovement>(character->GetMovementComponent()))
      {
         check(activeGE.Spec.Def);
         // We can just add all the modifiers with the same tag so they'll all get removed at the same time when we call RemoveMovementModifier(tag) later
         const FName tag = FName(activeGE.Spec.Def->GetName());
         for (const FTATGameplayEffectComponent_MovementModifierItem& mod : Modifiers)
         {
            cmc->AddMovementModifier(tag, MovementModHelpers::Convert(mod.Stat), MovementModHelpers::Convert(mod.Operator), mod.Value);
         }

         // Only check for removal if we successfully applied modifiers
         activeGE.EventSet.OnEffectRemoved.AddUObject(this, &UTATGameplayEffectComponent_MovementModifier::_OnActiveGameplayEffectRemoved, &activeGEContainer);
      }
   }

   return true;
}

void UTATGameplayEffectComponent_MovementModifier::_OnActiveGameplayEffectRemoved(const FGameplayEffectRemovalInfo& removalInfo, FActiveGameplayEffectsContainer* activeGEContainer) const
{
   if (!ensure(activeGEContainer))
   {
      return;
   }

   if (!ensure(removalInfo.ActiveEffect))
   {
      return;
   }

   const FGameplayEffectSpec& geSpec = removalInfo.ActiveEffect->Spec;
   if (!ensure(geSpec.Def))
   {
      return;
   }

   if (ACharacter* character = _GetOwnerCharacter(activeGEContainer->Owner))
   {
      if (UTATCharacterMovement* cmc = Cast<UTATCharacterMovement>(character->GetMovementComponent()))
      {
         const FName tag = FName(geSpec.Def->GetName());
         cmc->RemoveMovementModifier(tag);
      }
   }
}

// static
ACharacter* UTATGameplayEffectComponent_MovementModifier::_GetOwnerCharacter(const UAbilitySystemComponent* asc)
{
   if (asc == nullptr)
   {
      return nullptr;
   }

   ACharacter* character = Cast<ACharacter>(asc->GetOwner());
   if (character != nullptr)
   {
      return character;
   }

   if (AOSEPlayerState* playerState = Cast<AOSEPlayerState>(asc->GetOwner()))
   {
      return Cast<ACharacter>(playerState->GetPawn());
   }

   return nullptr;
}
