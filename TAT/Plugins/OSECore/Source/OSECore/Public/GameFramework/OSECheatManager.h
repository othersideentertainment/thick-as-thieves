// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "OSECoreCheats.h"
#include "OSECheatManager.generated.h"


// Verify that the cheat macros are properly defined
#if !defined(OSE_CHEATS_ENABLED)
#   define OSE_CHEATS_ENABLED (0)
#   error OSE_CHEATS_ENABLED must be defined
#endif // OSE_CHEATS_ENABLED

class APlayerController;

UCLASS()
class OSECORE_API UOSECheatManager : public UCheatManager
{
   GENERATED_BODY()
   
protected:

   // from UCheatManager
   virtual void InitCheatManager();

   /// @brief Returns the ability system component for the pawn controlled by
   /// the player controller associated with this cheat manager. Returns
   /// null if there is no possessed pawn or if the pawn doesn't implement
   /// IAbilitySystemInterface, or doesn't have an ability system component
   class UAbilitySystemComponent* GetAbilitySystemComponent() const;

   /// @brief Removes all active effects with the specified tag(s)
   /// @param TagContainer The tag(s) to check against when removing effects
   /// @return Returns true if at least one effect was removed
   virtual bool RemoveEffectsWithTags(const struct FGameplayTagContainer& tagContainer);

   /// @brief Removes all active effects with the specified tag
   /// @param Tag The tag to check against when removing effects
   /// @return Returns true if at least one effect was removed
   virtual bool RemoveEffectsWithTag(const struct FGameplayTag& tag);

   /// @brief Removes all active effects with the specified tag name
   /// @param Tag The tag to check against when removing effects
   /// @return Returns true if at least one effect was removed
   virtual bool RemoveEffectsWithTagName(const FName& tagName);

   /// @brief Removes all active effects with the specified tag string
   /// @param Tag The tag to check against when removing effects
   /// @return Returns true if at least one effect was removed
   virtual bool RemoveEffectsWithTagString(const FString& tagString);

   UFUNCTION(BlueprintPure)
   APlayerController* GetOwnerPlayerController() const;
};
