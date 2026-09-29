// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEGameplayAbility.h"

#include "ToolAbility.generated.h"

class UToolComponent;

//--------------------------------------------------------------------------------------------------
/// Component that represents an ability granted by an tool.
/// Can only be activated if the granted source object is a tool that has been added
/// to tool set and is currently equipped.
//--------------------------------------------------------------------------------------------------

UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UToolAbility : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:

   // Sets default values
   UToolAbility();

   /// Returns true if this ability can be activated right now. Has no side effects
   virtual bool CanActivateAbility(
      const FGameplayAbilitySpecHandle handle,
      const FGameplayAbilityActorInfo* actorInfo,
      const FGameplayTagContainer* sourceTags = nullptr,
      const FGameplayTagContainer* targetTags = nullptr,
      OUT FGameplayTagContainer* optionalRelevantTags = nullptr) const override;

   /// Returns the associated tool interface
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   virtual TScriptInterface<class IToolInterface> GetToolInterface() const;

   /// Returns true if the tool is ready
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   virtual bool IsToolReady() const;

   /// Returns true if the tool is equipped
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   virtual bool IsToolEquipped() const;

   // Returns the tool responsible for granting an ability, or nullptr if ability is not tool-associated. Should only be called with instanced abilities.
   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   static UToolComponent* GetSourceToolForAbility(const UGameplayAbility* ability);

   //allow activation of the ability while the tool is not equipped
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE")
   bool AllowUnequippedActivation;
};
