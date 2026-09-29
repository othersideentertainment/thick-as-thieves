// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "UtilityAIBehaviorTargetInterface.generated.h"

class UUtilityAIStateBase;
class AOSECharacterBase;

// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(BlueprintType, MinimalAPI, Category = "AI|OSE|Utility")
class UUtilityAIBehaviorTargetInterface : public UInterface
{
   GENERATED_BODY()
};

//---------------------------------------------------------------------------------------------------------
/// Utility AI Behavior Target Interface
/// - Implement on actors that want to be aware when they're targeted by AI behaviors
//---------------------------------------------------------------------------------------------------------

class OSEAI_API IUtilityAIBehaviorTargetInterface
{
   GENERATED_BODY()

public:
   /// An AI has decided to target this actor with a behavior
   UFUNCTION(BlueprintNativeEvent, Category = "AI|OSE|Utility")
   void AuthorityOnEnterTargetedByBehavior(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior);
   virtual void AuthorityOnEnterTargetedByBehavior_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior) = 0;
   
   /// An AI is no longer targeting this actor with a behavior
   UFUNCTION(BlueprintNativeEvent, Category = "AI|OSE|Utility")
   void AuthorityOnExitTargetedByBehavior(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior);
   virtual void AuthorityOnExitTargetedByBehavior_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior) = 0;
   
   /// An AI has decided to target this actor with a behavior
   UFUNCTION(BlueprintNativeEvent, Category = "AI|OSE|Utility")
   void AuthorityOnEnterTargetedByGoal(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior);
   virtual void AuthorityOnEnterTargetedByGoal_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior) = 0;
   
   /// An AI is no longer targeting this actor with a behavior
   UFUNCTION(BlueprintNativeEvent, Category = "AI|OSE|Utility")
   void AuthorityOnExitTargetedByGoal(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior);
   virtual void AuthorityOnExitTargetedByGoal_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior) = 0;
};
