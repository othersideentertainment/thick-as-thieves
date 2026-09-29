// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATMeleeWeaponToolComponent.h"

// tat
#include "Developer/TATToolSettings.h"
#include "Indicators/TATThiefVisionSubsystem.h"

// ue
#include "Animation/AnimMontage.h"
#include "GameFramework/Pawn.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMeleeWeaponToolComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATMeleeWeaponToolComponent, Log, All)

UTATMeleeWeaponToolComponent::UTATMeleeWeaponToolComponent()
{
   // Melee weapons by default don't need ammo
   AmmoType = ETATToolAmmoType::Infinite;
}

#if WITH_EDITOR
EDataValidationResult UTATMeleeWeaponToolComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   for (const TPair<FGameplayTag, FTATMeleeWeaponAttack>& meleeAttack : MeleeAttacksPerUsage)
   {
      if (!meleeAttack.Value.DealDamageEventTag.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Melee tool %s has has usage '%s' with an empty DealDamageEventTag")
            , *GetName()
            , *meleeAttack.Key.ToString())));
         result = EDataValidationResult::Invalid;
      }
   }

   return result;
}
#endif

bool UTATMeleeWeaponToolComponent::GetMeleeWeaponAttackForUsage(FGameplayTag usageTag, FTATMeleeWeaponAttack& weaponAttack) const
{
   if (!usageTag.IsValid())
   {
      UE_LOG(LogTATMeleeWeaponToolComponent, Error,
         TEXT("GetMeleeWeaponAttackForUsage given empty usage tag on tool '%s'"),
         *GetName());
      return false;
   }

   if (const FTATMeleeWeaponAttack* foundWeaponAttack = MeleeAttacksPerUsage.Find(usageTag))
   {
      weaponAttack = *foundWeaponAttack;
      return true;
   }
   else
   {
      UE_LOG(LogTATMeleeWeaponToolComponent, Error,
         TEXT("Could not find melee weapon attack associated with usage tag '%s' on tool '%s': check MeleeAttacksPerUsage"),
         *usageTag.ToString(),
         *GetName());
      return false;
   }
}

const FTATMeleeWeaponAttack* UTATMeleeWeaponToolComponent::GetMeleeWeaponAttackForUsage(FGameplayTag usageTag) const
{
   return MeleeAttacksPerUsage.Find(usageTag);
}

int32 UTATMeleeWeaponToolComponent::GetMaxChainAttackLength() const
{
   if (const FTATMeleeWeaponAttack* attack = _GetChainMeleeWeaponAttack())
   {
      return attack->SequentialAttackPools.Num();
   }

   return 0;
}

const FTATMeleeWeaponAttack* UTATMeleeWeaponToolComponent::_GetChainMeleeWeaponAttack() const
{
   for (const auto& it : MeleeAttacksPerUsage)
   {
      const FTATMeleeWeaponAttack& attack = it.Value;
      if (attack.IsChainAttack)
      {
         return &attack;
      }
   }

   return nullptr;
}
