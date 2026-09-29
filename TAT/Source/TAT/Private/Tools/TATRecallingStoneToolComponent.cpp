// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATRecallingStoneToolComponent.h"

// tat
#include "Tools/WorldActors/TATToolWorldActor_RecallingStone.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATRecallingStoneToolComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATRecallingStoneToolComponent, Log, All)

#if WITH_EDITOR
EDataValidationResult UTATRecallingStoneToolComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (!GetClass()->HasAnyClassFlags(CLASS_Abstract))
   {
      if (!SecondaryFireAvailableEffect)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Recalling Stone Tool '%s' does not define a valid SecondaryFireAvailableEffect"), *GetName())));
         result = EDataValidationResult::Invalid;
      }
   }

   return result;
}
#endif

bool UTATRecallingStoneToolComponent::OnRemoveFromToolSet_Implementation()
{
   if (GetOwner() && GetOwner()->HasAuthority())
   {
      AuthorityRemoveSpawnedTeleportStone();
   }

   return Super::OnRemoveFromToolSet_Implementation();
}

ATATToolWorldActor_RecallingStone* UTATRecallingStoneToolComponent::GetSpawnedTeleportStone() const
{
   // Validated against above
   if (!ensure(SecondaryFireAvailableEffect))
   {
      return nullptr;
   }

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      // Find the active effect that the recalling stone gave us, if it's present
      TArray<FActiveGameplayEffectHandle> gameplayEffectHandles = UOSEAbilityFunctionLibrary::GetActiveEffectsByClass(asc, SecondaryFireAvailableEffect);

      // Should not happen: we should only be able to deploy one recalling stone at a time, and that should be the only thing giving us this effect
      if (gameplayEffectHandles.Num() > 1)
      {
         UE_LOG(LogTATRecallingStoneToolComponent, Error,
            TEXT("Owner '%s' of recalling stone tool has multiple instances of secondary fire effect '%s'."),
            *GetOwner()->GetName(),
            *SecondaryFireAvailableEffect->GetFullName());
      }

      if (gameplayEffectHandles.Num() > 0)
      {
         if (const FActiveGameplayEffect* activeEffect = asc->GetActiveGameplayEffect(gameplayEffectHandles[0]))
         {
            // If we have an effect with the right class, get the effect causer:
            // that will be the reference to the recalling stone actor we placed in the world earlier
            AActor* effectCauser = activeEffect->Spec.GetEffectContext().GetEffectCauser();
            if (auto* recallingStone = Cast<ATATToolWorldActor_RecallingStone>(effectCauser))
            {
               return recallingStone;
            }
            else
            {
               UE_LOG(LogTATRecallingStoneToolComponent, Error,
                  TEXT("TATRecallingStoneToolComponent component found secondary fire effect, but it was mising the correct effect causer ('%s')"),
                  *GetNameSafe(effectCauser));
            }
         }
      }
   }

   return nullptr;
}

void UTATRecallingStoneToolComponent::AuthoritySetSpawnedTeleportStone(ATATToolWorldActor_RecallingStone* teleportStone)
{
   // Validated against
   if (ensure(SecondaryFireAvailableEffect))
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         if (teleportStone)
         {
            // Apply the secondary fire effect so the tool can switch to the teleport-ready state,
            // and add the teleport stone as the effect causer so we can refer back to it. This will replicate,
            // so the client will also have a reference to it
            // N.B. This assumes we can only have one recalling stone per-player at a time, which is the current design,
            // but this will need to change if the design allows more stones in play for a player
            FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
            effectContext.AddInstigator(GetOwner(), teleportStone);
            asc->ApplyGameplayEffectToSelf(SecondaryFireAvailableEffect.GetDefaultObject(), 0.0f, effectContext);
         }
         else
         {
            asc->RemoveActiveGameplayEffectBySourceEffect(SecondaryFireAvailableEffect, asc);
         }
      }
   }
}

void UTATRecallingStoneToolComponent::AuthorityRemoveSpawnedTeleportStone()
{
   check(GetOwner()->HasAuthority());

   // Remove any world actor we spawned in
   if (ATATToolWorldActor_RecallingStone* spawnedStone = GetSpawnedTeleportStone())
   {
      spawnedStone->Destroy();
   }

   // Clear out the effect
   AuthoritySetSpawnedTeleportStone(nullptr);
}
