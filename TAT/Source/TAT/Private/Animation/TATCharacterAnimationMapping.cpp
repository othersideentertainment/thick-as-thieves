// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/TATCharacterAnimationMapping.h"

// ue
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterAnimationMapping)

DEFINE_LOG_CATEGORY_STATIC(LogTATCharacterAnimationMapping, Log, All)

UAnimMontage* UTATCharacterAnimationMappingAsset::LookupMontageByTag(const FGameplayTag& animationTag, bool warnOnNotFound /*= true*/) const
{
   if (UAnimMontage *const *const montage = AnimationTagToMontage.Find(animationTag))
   {
      return *montage;
   }

   if (warnOnNotFound)
   {
      UE_LOG(LogTATCharacterAnimationMapping, Warning,
         TEXT("Could not find animation for tag '%s' on character animation mapping '%s'"),
         *animationTag.ToString(),
         *GetName())
   }

   return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UTATCharacterAnimationMappingAsset::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   for (const TPair<FGameplayTag, UAnimMontage*>& tagAndMontage : AnimationTagToMontage)
   {
      FGameplayTag animationTag = tagAndMontage.Key;
      UAnimMontage* montage = tagAndMontage.Value;

      if (!animationTag.IsValid())
      {
         result = EDataValidationResult::Invalid;

         context.AddError(FText::FromString(FString::Printf(
            TEXT("Character Animation '%s' has empty tag"),
            *GetName())));
      }

      if (!montage)
      {
         result = EDataValidationResult::Invalid;

         context.AddError(FText::FromString(FString::Printf(
            TEXT("Character Animation '%s' for tag '%s' has no montage"),
            *GetName(),
            *animationTag.ToString())));
      }
   }

   return result;
}

// Partly based on USyncedAnimationCharacterMontagesAsset::ValidateForCharacter
int32 UTATCharacterAnimationMappingAsset::ValidateForCharacter(const class ACharacter* character, FDataValidationContext& context) const
{
   check(character);

   USkinnedAsset* skinnedAsset = character->GetMesh() ? character->GetMesh()->GetSkinnedAsset() : nullptr;
   USkeleton* skeleton = skinnedAsset ? skinnedAsset->GetSkeleton() : nullptr;

   int32 numErrors = 0;
   if (skeleton)
   {
      for (const TPair<FGameplayTag, UAnimMontage*>& tagAndMontage : AnimationTagToMontage)
      {
         FGameplayTag animationTag = tagAndMontage.Key;
         UAnimMontage* montage = tagAndMontage.Value;

         // NB: If montage is null, we'll error in IsDataValid, so focus on interactions with the character asset here
         if (montage && montage->GetSkeleton() != skeleton)
         {
            context.AddError(FText::FromString(FString::Printf(
               TEXT("Character Animation '%s' for character '%s' for tag '%s' uses montage '%s' that targets skeleton '%s', but the character has skeleton '%s'"),
               *GetName(),
               *character->GetName(),
               *animationTag.ToString(),
               *montage->GetName(),
               *GetNameSafe(montage->GetSkeleton()),
               *GetNameSafe(skeleton))));
            numErrors++;
         }
      }
   }

   return numErrors;
}
#endif

