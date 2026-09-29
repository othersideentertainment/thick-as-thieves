// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/InteractMontageMappingAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractMontageMappingAsset)

UAnimMontage* UInteractMontageMappingAsset::FindInstantInteractMontage(FGameplayTag animationTypeTag) const
{
   while (animationTypeTag.IsValid())
   {
      if (TObjectPtr<UAnimMontage> const* found = _instantInteractMontages.Find(animationTypeTag))
      {
         return *found;
      }

      animationTypeTag = animationTypeTag.RequestDirectParent();
   }

   return _fallbackInstantInteractMontage;
}

TSoftObjectPtr<UAnimMontage> UInteractMontageMappingAsset::FindHoldInteractMontage(FGameplayTag animationTypeTag) const
{
   while (animationTypeTag.IsValid())
   {
      if (TSoftObjectPtr<UAnimMontage> const* found = _holdInteractMontages.Find(animationTypeTag))
      {
         return *found;
      }

      animationTypeTag = animationTypeTag.RequestDirectParent();
   }

   return nullptr;
}

