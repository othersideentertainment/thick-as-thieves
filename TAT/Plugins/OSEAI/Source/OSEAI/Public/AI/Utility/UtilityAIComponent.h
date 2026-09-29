// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/StimInfo.h"
#include "AI/Utility/ConsiderationInput.h"
#include "AI/Utility/UtilityAITypes.h"

// ue4
#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"

// self
#include "UtilityAIComponent.generated.h"

class AOSEAIController;
class AOSECharacterAIBase;
#if ENABLE_VISUAL_LOG
struct FVisualLogEntry;
#endif // ENABLE_VISUAL_LOG

UCLASS(Abstract)
class OSEAI_API UUtilityAIComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAIStateEnter, const FUtilityStateEvaluatorInstance&, state, const FUtilityStateTarget&, target);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAIStateExit, const FUtilityStateEvaluatorInstance&, state, const FUtilityStateTarget&, target);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAboutToCheckForNewStates, AActor*, aiCharacter, UUtilityAIComponent*, utilityAIComp);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCheckedForNewStates, AActor*, aiCharacter, UUtilityAIComponent*, utilityAIComp);

public:

   UUtilityAIComponent();
   
   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

public:

   /// Returns whether a state is currently running.
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   bool IsRunning() const { return _currentState.IsValid(); }

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   UUtilityAIStateBase* GetCurrentState() const { return _currentState.Instance; }
   
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   const FUtilityStateEvaluatorInstance& GetCurrentEvaluatorInstance() const { return _currentState; }

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   const TArray<FUtilityStateEvaluatorInstance>& GetStates() const { return _states; }

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   FName GetCurrentStateName() const;

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   const FUtilityStateTarget& GetCurrentTarget() const { return _currentStateTarget; }

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   UObject* GetCurrentTargetObject() const { return _currentStateTarget.GetTargetUObject(); }

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   AOSEAIController* GetAIController() const { return _aiController; }

   UPROPERTY(BlueprintAssignable, Category = "AI|OSE|Utility")
   FAIStateEnter OnAIStateEnter;

   UPROPERTY(BlueprintAssignable, Category = "AI|OSE|Utility")
   FAIStateExit OnAIStateExit;

   // api for managing an enabled state externally
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   void SetEnabled(bool enabled);
   
   // api for managing utility states externally
   const UObject* GetStatesOwner() const { return _statesOwner; }
   virtual void SetStatesOwner(const UObject* stateOwner);
   virtual void SetStates(const TArray<FUtilityStateEvaluatorInstance>& states);
   virtual void CheckForNewState();
   virtual void ClearStates();

   UPROPERTY(BlueprintAssignable, Category = "AI|OSE|Utility")
   FOnAboutToCheckForNewStates OnAboutToCheckForNewStates;

   UPROPERTY(BlueprintAssignable, Category = "AI|OSE|Utility")
   FOnCheckedForNewStates OnCheckedForNewStates;   

   // api for managing utility targets externally
   const UObject* GetForcedTargetOwner() const { return _forcedTargetingOwner; }
   virtual void AddForcedTargetForType(const UObject* owner, EUtilityStateTargeting type, const FUtilityStateTarget& target);
   virtual void ClearForcedTargetForType();

   bool HasAccessToAITokenForTarget(const FOSEAITokenInfo& tokenInfo, const FUtilityStateTarget& target) const;

#if ENABLE_VISUAL_LOG
   // for subclasses
   virtual void DescribeSelfToVisLog(FVisualLogEntry* snapshot) const { }
#endif // ENABLE_VISUAL_LOG

#if (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
   struct FConsiderationDebugLogEntry
   {
      FName Name;
      float Score = 0.0f;
   };
   struct FStateTargetDebugLogEntry
   {
      UUtilityStateEvaluator* Evaluator = nullptr;
      FUtilityStateTarget Target;
      FName Name;
      float Weight = 0.0f;
      float Score = 0.0f;
      float Bonus = 0.0f;
      bool WasFullyConsidered = true;
      TArray<FConsiderationDebugLogEntry> ConsiderationLogEntries;
   };
   const TArray<FStateTargetDebugLogEntry>& GetStateTargetDebugLogEntries() const { return _stateTargetDebugLogEntries; }
#endif // (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)

protected:

   UPROPERTY(Transient)
   bool _isEnabled = true;
   
   UPROPERTY(Transient)
   bool _shouldRegisterAsGoalUtilityBehavior = false;

   UPROPERTY(Transient)
   AOSEAIController* _aiController = nullptr;

   UPROPERTY(Transient)
   const UObject* _statesOwner = nullptr;

   UPROPERTY(Transient)
   TArray<FUtilityStateEvaluatorInstance> _states;

   FUtilityStateEvaluatorInstance _currentState;

   UPROPERTY(Transient)
   FUtilityStateTarget _currentStateTarget = FUtilityStateTarget::Invalid;

   UPROPERTY(Transient)
   UUtilityAITokenOwner* _currentTokenOwnerObj = nullptr;

   UPROPERTY(Transient)
   const UObject* _forcedTargetingOwner = nullptr;
   TMap<EUtilityStateTargeting, TArray<FUtilityStateTarget>> _forcedTargeting;

#if (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
   TArray<FStateTargetDebugLogEntry> _stateTargetDebugLogEntries;
#endif // (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)

   // for subclasses to provide game-specific targets
   virtual bool _HasCachedPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const { return false; }
   virtual void _CachePotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) { }
   virtual const TArray<FUtilityStateTarget>& _GetPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const;
   virtual void _ResetPotentialTargetCache() { }

   // for subclasses to hook into state changes
   virtual void _OnStateEntered(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state) { }
   virtual void _OnStateExited(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state) { }
   virtual void _OnStateChanged(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state) { }

   virtual FUtilityStateEvaluatorInstance* _CalculateTopScoredState(FUtilityStateTarget& outTarget); // TODO: should make this const but there's some internal mutable state which isn't great
   virtual bool _TrySetState(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target);

   // for debugging logging
   virtual void _DebugInfoAddStateAndTarget(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target);
   virtual void _DebugInfoAddConsideration(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target, FName name, float score) const;
   virtual void _DebugInfoAddStateAndTargetScore(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target, float score, float bonus, bool wasFullyConsidered);

   // utl
   AOSECharacterBase* _GetOwnerCharacter() const;

private:
   /// Exits the current state and clears it. Currently called in Tick method and on AI pawn Unpossess.
   void _ExitState();
   void _EnterState(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& stateTarget);

   float _CalculateStateScore(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target, float weight, float scoreToBeat, bool& outWasFullyConsidered) const;
   float _GetStateTargetBonus(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target) const;

   const TArray<FUtilityStateTarget>& _CacheOrFindTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup);

   UFUNCTION()
   void _OnPawnPossessed(APawn* pawn);

   UFUNCTION()
   void _OnPawnUnPossessed();
};
