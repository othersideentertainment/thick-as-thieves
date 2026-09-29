// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "Abilities/OSEAbilityInputBinds.h"
#include "OSEAbilitiesGameplayEffectComponent.generated.h"


// originally a subclass of FOSEAbilityBindInfo, but I couldn't get the properties to show up in the right order
// should be able to switch back if desired in that is figured out
USTRUCT()
struct FOSEGameplayEffectAbilityGrantConfig
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (InlineEditConditionToggle))
   bool bBindToInput = false;

   /// The input enumeration to map to the activation of the ability
   UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (EditCondition = "bBindToInput"))
   EAbilityInputType InputCommand = EAbilityInputType::None;

   /// The actual ability class
   UPROPERTY(EditDefaultsOnly, Category = "Ability")
   TSubclassOf<class UOSEGameplayAbility> AbilityClass;

   /// The upgrade tag required to grant the ability (or none)
   UPROPERTY(EditDefaultsOnly, Category = "Ability")
   FOSEAbilityBindRequirements Requirements;

   //! New properties start here

   UPROPERTY(EditDefaultsOnly, Category = "Ability", DisplayName = "Level", meta=(UIMin=0.0))
   FScalableFloat LevelScalableFloat = FScalableFloat{ 1.0f };
   
   UPROPERTY(EditDefaultsOnly, Category = "Ability")
   EGameplayEffectGrantedAbilityRemovePolicy RemovalPolicy = EGameplayEffectGrantedAbilityRemovePolicy::CancelAbilityImmediately;
};

// Grants additional Gameplay Abilities to the Target of a Gameplay Effect while active
//
// Fork of UAbilitiesGameplayEffectComponent with OSE input bindings and support for requirements
// Copied as of 5.4
UCLASS(DisplayName="[OSE] Grant Gameplay Abilities")
class OSECORE_API UOSEAbilitiesGameplayEffectComponent : public UGameplayEffectComponent
{
   GENERATED_BODY()

public:
   UOSEAbilitiesGameplayEffectComponent();

   // Register for the appropriate events when we're applied
   virtual bool OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const override;

#if WITH_EDITOR
   // Warn on misconfigured Gameplay Effect
   virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
   // This allows us to be notified when the owning GameplayEffect has had its inhibition changed (which can happen on the initial application).
   void OnInhibitionChanged(FActiveGameplayEffectHandle ActiveGEHandle, bool bIsInhibited) const;
   void GrantAbilities(FActiveGameplayEffectHandle ActiveGEHandle) const;
   void RemoveAbilities(FActiveGameplayEffectHandle ActiveGEHandle) const;

private:
   // We must undo all effects when removed
   void OnActiveGameplayEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo) const;

protected:
   // Abilities to Grant to the Target while this Gameplay Effect is active
   UPROPERTY(EditDefaultsOnly, Category = GrantAbilities, meta=(TitleProperty="{AbilityClass} ({InputCommand})"))
   TArray<FOSEGameplayEffectAbilityGrantConfig>	_grantAbilityConfigs;
};
