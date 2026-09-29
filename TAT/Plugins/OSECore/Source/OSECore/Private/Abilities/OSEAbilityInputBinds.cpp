// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEAbilityInputBinds.h"

// ose
#include "OSECommon.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/OSECharacterBase.h"
#include "Input/OSEInputFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityInputBinds)

UInputAction* UOSEAbilityInputFunctionLibrary::GetInputActionFromAbilityType(AActor* character, EAbilityInputType inputType)
{
   if (AOSECharacterBase* oseCharacter = UOSECommon::GetCharacterFromActor(character))
   {
      if (UOSEAbilitySystemComponent* asc = oseCharacter->GetAbilitySystemComponentFromActor())
      {
         if (UEnhancedAbilityInputActionsAsset* asset = asc->GetEnhancedAbilityInputActionsAsset())
         {
            return asset->FindInputActionFromAbilityType(inputType);
         }
      }
   }
   return nullptr;
}

FKey UOSEAbilityInputFunctionLibrary::GetKeyForAbilityInput(AActor* character, EAbilityInputType inputType, EOSEInputHardwareType inputHardwareType)
{   
   if (UInputAction* action = GetInputActionFromAbilityType(character, inputType))
   {
      return UOSEInputFunctionLibrary::GetKeyForInputAction(character, action, inputHardwareType);
   }
   return FKey();
}

FText UOSEAbilityInputFunctionLibrary::GetKeyTextForAbilityInput(AActor* character, EAbilityInputType inputType, EOSEInputHardwareType inputHardwareType, bool longDisplayName)
{
   FKey key = GetKeyForAbilityInput(character, inputType, inputHardwareType);
   return UOSEInputFunctionLibrary::GetDisplayNameForKey(key, longDisplayName);
}

bool FOSEAbilityBindRequirements::IsMet(const UOSEAbilitySystemComponent* asc) const
{
   check(asc);
   return !RequiredUpgradeTag.IsValid() || asc->GetUpgradeValue(RequiredUpgradeTag, 0) >= RequiredUpgradeLevel;
}

//--------------------------------------------------------------------------------------------------
// UEnhancedAbilityInputActionsAsset
//--------------------------------------------------------------------------------------------------

UEnhancedAbilityInputActionsAsset::UEnhancedAbilityInputActionsAsset()
{
   for (int idx = 0; idx < int(EAbilityInputType::None); ++idx)
   {
      AbilityInputToEnhancedInputActionMapping.Add(EAbilityInputType(idx), nullptr);
   }
}

bool UEnhancedAbilityInputActionsAsset::FindAbilityInputFromInputAction(const UInputAction* searchInputAction, EAbilityInputType& inputType) const
{
   if (searchInputAction)
   {
      for (auto it = AbilityInputToEnhancedInputActionMapping.CreateConstIterator(); it; ++it)
      {
         UInputAction* inputAction = it.Value();
         if (searchInputAction == inputAction)
         {
            inputType = it.Key();
            return true;
         }

      }
   }
   return false;
}

UInputAction* UEnhancedAbilityInputActionsAsset::FindInputActionFromAbilityType(EAbilityInputType inputType) const
{
   if (auto* result = AbilityInputToEnhancedInputActionMapping.Find(inputType))
   {
      return *result;
   }
   return nullptr;
}

