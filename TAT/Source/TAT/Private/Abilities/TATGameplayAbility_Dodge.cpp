// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATGameplayAbility_Dodge.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_Dodge)
DEFINE_LOG_CATEGORY_STATIC(LogTATGameplayAbility_Dodge, Log, All);

#if WITH_EDITOR
EDataValidationResult UTATGameplayAbility_Dodge::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   // Ensure each entry has an assigned montage + unit-length direction vector
   for (const FTATDodgeDirectionalAnimEntry& dodgeAnimEntry : _dodgeDirectionalAnimEntries)
   {
      if (!dodgeAnimEntry.DodgeMontage)
      {
         context.AddError(FText::FromString(TEXT("FTATDodgeDirectionalAnimEntry found with unassigned AnimMontage!")));
      }
      if (!dodgeAnimEntry.DodgeDirection.IsUnit())
      {
         context.AddError(FText::FromString(TEXT("FTATDodgeDirectionalAnimEntry found with non-unit vector!")));
      }
   }

   return context.GetIssues().IsEmpty() ? result : EDataValidationResult::Invalid;
}
#endif // WITH_EDITOR

UTATGameplayAbility_Dodge::UTATGameplayAbility_Dodge()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

UAnimMontage* UTATGameplayAbility_Dodge::GetDodgeAnimMontage(const FVector& relativeDodgeDirection) const
{
   // Select entry with vector most closely aligned with (relative) dodge direction
   const FTATDodgeDirectionalAnimEntry* closestFit = nullptr;
   float closestDot = FLT_MIN;
   for (const FTATDodgeDirectionalAnimEntry& dodgeAnimEntry : _dodgeDirectionalAnimEntries)
   {
      // Copy vector and normalize to ensure length doesn't skew results
      FVector entryVector = dodgeAnimEntry.DodgeDirection;
      entryVector.Normalize();
      const float dot = FVector::DotProduct(relativeDodgeDirection, entryVector);
      if (dot > closestDot)
      {
         closestFit = &dodgeAnimEntry;
         closestDot = dot;
      }
   }

   if (closestFit)
   {
      UE_CLOG(closestFit->DodgeMontage == nullptr, LogTATGameplayAbility_Dodge, Error, TEXT("GetDodgeAnimMontage() | selected FTATDodgeDirectionalAnimEntry in direction %s with unassigned AnimMontage!")
         , *closestFit->DodgeDirection.ToString());
      return closestFit->DodgeMontage;
   }

   UE_LOG(LogTATGameplayAbility_Dodge, Error, TEXT("GetDodgeAnimMontage() | no FTATDodgeDirectionalAnimEntry present to match with!"));
   return nullptr;
}
