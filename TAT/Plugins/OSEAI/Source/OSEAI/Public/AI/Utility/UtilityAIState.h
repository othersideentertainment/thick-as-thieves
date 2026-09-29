// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SmartObjectRuntime.h"

#include "UtilityAIState.generated.h"

class AOSEAIController;
class AOSECharacterBase;
class UOSEAbilitySystemComponent;
class UUtilityAIComponent;

UENUM(BlueprintType)
enum class EBehaviorState : uint8
{
   // The behavior can be interrupted by another higher scoring behavior.
   Interruptible,
   // The behavior cannot be interrupted by other behaviors. Currently used for behaviors tied to
   // gameplay abilities (i.e. combat).
   NotInterruptible,
   // The behavior is done doing it's actions (i.e. the gameplay ability has finished).
   Completed,
};

/// A state for the utility AI
UCLASS(Abstract)
class OSEAI_API UUtilityAIStateBase : public UObject
{
   GENERATED_BODY()

public:

   UUtilityAIStateBase();

   virtual class UWorld* GetWorld() const;

   virtual void Init(UUtilityAIComponent& utilityAIComponent);

   /// Called when the state is created.
   UFUNCTION(BlueprintImplementableEvent, Category = "AI|OSE|Utility", meta = (DisplayName="Init", ScriptName="Init"))
   void BP_Init();

   virtual void Reset();

   /// Called before Enter is called to reset any internal state.
   UFUNCTION(BlueprintImplementableEvent, Category = "AI|OSE|Utility", meta = (DisplayName = "Reset", ScriptName = "Reset"))
   void BP_Reset();

   virtual void Enter();

   /// Called when the state is entered.
   UFUNCTION(BlueprintImplementableEvent, Category = "AI|OSE|Utility", meta = (DisplayName="Enter", ScriptName="Enter"))
   void BP_Enter();

   virtual void Exit();

   /// Called when the state is exited.
   UFUNCTION(BlueprintImplementableEvent, Category = "AI|OSE|Utility", meta=(DisplayName="Exit", ScriptName="Exit"))
   void BP_Exit();

   virtual void Tick(float deltaTime);

   /// Called on every tick of the state.
   UFUNCTION(BlueprintImplementableEvent, Category = "AI|OSE|Utility", meta=(DisplayName="Tick", ScriptName="Tick"))
   void BP_Tick(float deltaTime);

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   UUtilityAIComponent* GetUtilityAIComponent() const { return _utilityAIComponent; }

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   AOSEAIController* GetController() const;

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   AOSECharacterBase* GetCharacter() const;

   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility")
   UOSEAbilitySystemComponent* GetAbilitySystemComponent() const;

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   FSmartObjectClaimHandle TryGetSmartObjectClaimHandleFromStateTarget() const;

   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility")
   const FGameplayTagContainer& GetStateTags() const { return _stateTags; }

   float GetMostRecentStartTime() const { return _mostRecentStartTime; }
   float GetMostRecentEndTime() const { return _mostRecentEndTime; }

   virtual bool IsInterruptible() const { return false; }
   virtual bool IsComplete() const { return false; }

protected:
   UPROPERTY(Transient)
   UUtilityAIComponent* _utilityAIComponent = nullptr;

   /// Tags describing this state
   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility", meta = (Categories = "AI.State.Metadata"), DisplayName = "StateTags")
   FGameplayTagContainer _stateTags;

private:
   // Server world time that this behavior was most recently started
   float _mostRecentStartTime = 0.0;
   // Server world time that this behavior was most recently ended
   float _mostRecentEndTime = 0.0;
};
