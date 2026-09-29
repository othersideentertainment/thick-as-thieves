// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/OSECheatManager.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECheatManager)


void UOSECheatManager::InitCheatManager()
{
   // This marks all public blueprint functions in our bp subclass as exec so we can implement custom cheats in blueprints
   for (TFieldIterator<UFunction> it(GetClass(), EFieldIteratorFlags::ExcludeSuper); it; ++it)
   {
      UFunction* function = *it;

      if (function && !function->HasAnyFunctionFlags(FUNC_Native) && function->HasAnyFunctionFlags(FUNC_Public))
      {
         function->FunctionFlags = FUNC_Exec | FUNC_BlueprintCallable;
      }
   }

   Super::InitCheatManager();
}

// Returns the ability system component for the pawn controlled by
// the player controller associated with this cheat manager. Returns
// null if there is no possessed pawn or if the pawn doesn't implement
// IAbilitySystemInterface, or doesn't have an ability system component
UAbilitySystemComponent* UOSECheatManager::GetAbilitySystemComponent() const
{
   if (APawn* const pawn = GetOuterAPlayerController()->GetPawn())
   {
      if (IAbilitySystemInterface* const asi = Cast<IAbilitySystemInterface>(pawn))
      {
         return asi->GetAbilitySystemComponent();
      }
   }

   return nullptr;
}

// Removes all active effects with the specified tag(s)
// Returns true if at least one effect was removed
bool UOSECheatManager::RemoveEffectsWithTags(const FGameplayTagContainer& tagContainer)
{
   if (UAbilitySystemComponent* const asc = GetAbilitySystemComponent())
   {
      const int32 numRemoved = asc->RemoveActiveEffectsWithTags(tagContainer);
      return numRemoved > 0;
   }

   return false;
}

// Removes all active effects with the specified tag
// Returns true if at least one effect was removed
bool UOSECheatManager::RemoveEffectsWithTag(const FGameplayTag& tag)
{
   if (!tag.IsValid())
      return false;

   return RemoveEffectsWithTags(FGameplayTagContainer(tag));
}

// Removes all active effects with the specified tag name
// Returns true if at least one effect was removed
bool UOSECheatManager::RemoveEffectsWithTagName(const FName& tagName)
{
   const bool kErrorIfNotFound = true;
   return RemoveEffectsWithTag(FGameplayTag::RequestGameplayTag(tagName, kErrorIfNotFound));
}

// Removes all active effects with the specified tag string
// Returns true if at least one effect was removed
bool UOSECheatManager::RemoveEffectsWithTagString(const FString& tagString)
{
   return RemoveEffectsWithTagName(FName(*(tagString.TrimStartAndEnd())));
}

APlayerController* UOSECheatManager::GetOwnerPlayerController() const
{
   APlayerController* pc = GetOuterAPlayerController();
   check(pc);
   return pc;
}

