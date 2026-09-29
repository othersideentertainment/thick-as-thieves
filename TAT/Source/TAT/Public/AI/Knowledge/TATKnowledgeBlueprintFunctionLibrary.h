// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATKnowledgeBlueprintFunctionLibrary.generated.h"

class ATATCharacterAIBase;
class UTATKnowledgeComponent;
class UUtilityAIComponent;
struct FTATSharedTarget;

UCLASS()
class TAT_API UTATKnowledgeBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

   UFUNCTION(BlueprintCallable, Category = "TAT|Knowledge|Shared Targets")
   static bool GetSharedTargetActor(const FTATSharedTarget& sharedTarget, AActor*& outActor);

   UFUNCTION(BlueprintCallable, Category = "TAT|Knowledge|Shared Targets")
   static bool GetSharedTargetLocation(const FTATSharedTarget& sharedTarget, FVector& outLocation);

   UFUNCTION(BlueprintCallable, Category = "TAT|Knowledge|Shared Targets")
   static const FGameplayTag& GetSharedTargetTypeTag(const FTATSharedTarget& sharedTarget);

   UFUNCTION(BlueprintCallable, Category = "TAT|Knowledge|Shared Targets")
   static const FGameplayTagContainer& GetSharedTargetContextTags(const FTATSharedTarget& sharedTarget);

   UFUNCTION(BlueprintCallable, Category = "TAT|Knowledge|Shared Targets")
   static ATATCharacterAIBase* GetSharedTargetInstigator(const FTATSharedTarget& sharedTarget);

   /// Generate a target that an AI intends to share with an ally.
   /// @param knowledgeComponent The knowledge component of the AI that is remembering a target to share with an ally.
   /// @param utilityAIComponent The utility AI component of the remembering AI, where the target will be pulled from.
   /// @param sharedTargetType Used as an indicator, when this target is shared with an ally, how the receiver should respond.
   /// @param partnerClass The AI class which we desired to inform of the target we're remembering (i.e. a civilian wants to
   ///                     report a suspicious player to a guard - the guard would be the 'partner').
   UFUNCTION(BlueprintCallable, Category = "TAT|Knowledge|Shared Targets")
   static bool RememberTargetToShare(
      UTATKnowledgeComponent* knowledgeComponent, 
      const UUtilityAIComponent* utilityAIComponent, 
      UPARAM(meta = (Categories = "AI.Knowledge.SharedTarget")) FGameplayTag sharedTargetType,
      TSoftClassPtr<ATATCharacterAIBase> partnerClass);

   /// Generate a target that an AI intends to share with an ally.
   /// @param knowledgeComponent The knowledge component of the AI that is remembering a target to share with an ally.
   /// @param utilityAIComponent The utility AI component of the remembering AI, where the target will be pulled from.
   /// @param sharedTargetType Used as an indicator, when this target is shared with an ally, how the receiver should respond.
   /// @param partnerClass The AI class which we desired to inform of the target we're remembering (i.e. a civilian wants to
   ///                     report a suspicious player to a guard - the guard would be the 'partner').
   UFUNCTION(BlueprintCallable, Category = "TAT|Knowledge|Shared Targets")
   static bool RememberExplicitTargetToShare(
      UTATKnowledgeComponent* knowledgeComponent, 
      AActor* target, 
      UPARAM(meta = (Categories = "AI.Knowledge.SharedTarget")) FGameplayTag sharedTargetType,
      TSoftClassPtr<ATATCharacterAIBase> partnerClass);
   
   /// Generate a target that an AI intends to share with an ally.
   /// @param knowledgeComponent The knowledge component of the AI that is remembering a target to share with an ally.
   /// @param location The location to be remembered.
   /// @param sharedTargetType Used as an indicator, when this target is shared with an ally, how the receiver should respond.
   /// @param partnerClass The AI class which we desired to inform of the target we're remembering (i.e. a civilian wants to
   ///                     report a suspicious player to a guard - the guard would be the 'partner').
   UFUNCTION(BlueprintCallable, Category = "TAT|Knowledge|Shared Targets")
   static bool RememberLocationToShare(
      UTATKnowledgeComponent* knowledgeComponent,
      const FVector location,
      UPARAM(meta = (Categories = "AI.Knowledge.SharedTarget")) FGameplayTag sharedTargetType,
      TSoftClassPtr<ATATCharacterAIBase> partnerClass);
	
};
