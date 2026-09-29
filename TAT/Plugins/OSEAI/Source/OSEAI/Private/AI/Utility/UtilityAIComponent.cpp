// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/UtilityAIComponent.h"

// ose
#include "AI/OSEAIController.h"
#include "AI/OSEAISettings.h"
#include "AI/Utility/ResponseCurve.h"
#include "AI/Utility/UtilityAITokenOwner.h"
#include "AI/Utility/UtilityAITokenRequester.h"
#include "Developer/OSEAIEditorSettings.h"

// ue4
#include "AI/Utility/UtilityAIManagerWorldSubsystem.h"
#include "Character/OSECharacterBase.h"
#include "Engine/NetDriver.h"
#include "Kismet/GameplayStatics.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAIComponent)

#define UTILITYAI_SCOPE(Name) DECLARE_SCOPE_CYCLE_COUNTER(TEXT(#Name), STAT_UtilityAI_ ## Name , STATGROUP_UtilityAI)

//---------------------------------------------------------------------------------------
// UUtilityAIComponent
//---------------------------------------------------------------------------------------

UUtilityAIComponent::UUtilityAIComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
}

void UUtilityAIComponent::BeginPlay()
{
   Super::BeginPlay();

   if (const auto aiController = Cast<AOSEAIController>(GetOwner()))
   {
      _aiController = aiController;
      _aiController->OnPossessedPawn.AddUniqueDynamic(this, &UUtilityAIComponent::_OnPawnPossessed);
      _aiController->OnUnPossessedPawn.AddUniqueDynamic(this, &UUtilityAIComponent::_OnPawnUnPossessed);

      if (APawn* pawn = _aiController->GetPawn())
      {
         _OnPawnPossessed(pawn);
      }
   }
}

void UUtilityAIComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);

   // once we EndPlay we should no longer listen to or access our owning controller
   if (_aiController)
   {
      _aiController->OnPossessedPawn.RemoveAll(this);
      _aiController->OnUnPossessedPawn.RemoveAll(this);
      _aiController = nullptr;
   }
}

void UUtilityAIComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   QUICK_SCOPE_CYCLE_COUNTER(STAT_UtilityAIComponent_Tick);

   // Don't tick if our AI controller does not have a Pawn.
   if (!_aiController || !_aiController->GetPawn())
   {
      return;
   }

   // Tick the current state
   if (_currentState.IsValid())
   {
      _currentState.Instance->Tick(deltaTime);
   }
}

void UUtilityAIComponent::SetEnabled(bool enabled)
{
   if (enabled != _isEnabled)
   {
      _isEnabled = enabled;

      if (!_isEnabled)
      {
         _ExitState();
#if (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
         _stateTargetDebugLogEntries.Reset();
#endif // (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
         _OnStateChanged(_currentStateTarget, _currentState);
      }
   }
}

void UUtilityAIComponent::SetStatesOwner(const UObject* statesOwner)
{
   _statesOwner = statesOwner;
}

void UUtilityAIComponent::SetStates(const TArray<FUtilityStateEvaluatorInstance>& states)
{
   _states = states;

   // if we are in a state that does not exist in the new list, exit it
   if (_currentState.IsValid() && !_states.Contains(_currentState))
   {
      _ExitState();
      _OnStateChanged(_currentStateTarget, _currentState);
   }

   // When states are set, check immediately for the next state of we will be waiting for the next evaluation via
   // UUtilityAIManagerWorldSubsystem. This _may_ lead to spiking if all NPC's do this at the same time, however the states
   // should only change infrequently.  We may also have this check occur twice the same frame for the same NPC if the
   // UUtilityAIManagerWorldSubsystem ALSO decides to call for a new state to be checked.
   CheckForNewState();
}

void UUtilityAIComponent::CheckForNewState()
{
   // let external systems respond to state checking about to run
   if (IsValid(_aiController) && IsValid(_aiController->GetCharacter()))
   {
      OnAboutToCheckForNewStates.Broadcast(_aiController->GetCharacter(), this);
   }

   if (_isEnabled)
   {
      FUtilityStateTarget stateTarget = FUtilityStateTarget::Invalid;

      if (!_currentState.IsValid() || _currentState.Instance->IsInterruptible())
      {
         UTILITYAI_SCOPE(CheckForNewState);

         UE_LOG(LogOSEUtilityAI, Verbose, TEXT("**** Checking for new state for %s ****"), *GetOwner()->GetName());

         const FUtilityStateEvaluatorInstance* topScoringState = _CalculateTopScoredState(stateTarget);
         _TrySetState(topScoringState ? *topScoringState : FUtilityStateEvaluatorInstance::Invalid, stateTarget);
      }
   }

   // let external systems respond to state checking having been run
   if (IsValid(_aiController) && IsValid(_aiController->GetCharacter()))
   {
      OnCheckedForNewStates.Broadcast(_aiController->GetCharacter(), this);
   }
}

void UUtilityAIComponent::ClearStates()
{
   _ExitState();
#if (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
   _stateTargetDebugLogEntries.Reset();
#endif // (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
   _OnStateChanged(_currentStateTarget, _currentState);
   _states.Reset();
}

void UUtilityAIComponent::AddForcedTargetForType(const UObject* owner, EUtilityStateTargeting type, const FUtilityStateTarget& target)
{
   _forcedTargetingOwner = owner;
   TArray<FUtilityStateTarget>& targets = _forcedTargeting.Add(type);
   targets.Emplace(target);
}

void UUtilityAIComponent::ClearForcedTargetForType()
{
   _forcedTargetingOwner = nullptr;
   _forcedTargeting.Reset();
}

FUtilityStateEvaluatorInstance* UUtilityAIComponent::_CalculateTopScoredState(FUtilityStateTarget& outTarget)
{
   UTILITYAI_SCOPE(CalculateTopScoredState);

   // Now let's score all our states and see if we wanna switch!
   float topScore = 0.0f;
   FUtilityStateEvaluatorInstance* topScoringState = nullptr;
   FUtilityStateTarget stateTarget = FUtilityStateTarget::Invalid;

#if (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
   // clear all previous entries, we're regenerating new ones...
   _stateTargetDebugLogEntries.Reset();
   const UOSEAIEditorSettings& editorSettings = UOSEAIEditorSettings::GetOSEAIEditorSettings();
#endif // (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)

   // TODO: Generate all targets for each state and sort the state/target pairs by weight, including bonuses.

   for (FUtilityStateEvaluatorInstance& state : _states)
   {
      UE_LOG(LogOSEUtilityAI, Verbose, TEXT(" * %s (%s) evaluation:"), *state.Name.ToString(), *state.Evaluator->GetName());

      UtilityTargetingUtl::ForEachTargetingType(state.Evaluator->TargetingFlags, state.Evaluator->TargetingGroup,
         [this, &state, &stateTarget, &topScore, &topScoringState](EUtilityStateTargeting targetingType, const FGameplayTag& targetingGroup)
      {
         if (targetingType == EUtilityStateTargeting::None)
         {
            // Untargeted state, so just calculate one score for it.
            const float bonus = _GetStateTargetBonus(state, FUtilityStateTarget::Invalid);
            const float weight = state.Weight + bonus;
            bool wasFullyConsidered;

            _DebugInfoAddStateAndTarget(state, FUtilityStateTarget::Invalid);
            const float currentScore = _CalculateStateScore(state, FUtilityStateTarget::Invalid, weight, topScore, wasFullyConsidered);
            _DebugInfoAddStateAndTargetScore(state, FUtilityStateTarget::Invalid, currentScore, bonus, wasFullyConsidered);

            if (currentScore > topScore)
            {
               topScore = currentScore;
               topScoringState = &state;
               stateTarget = FUtilityStateTarget::Invalid;
            }
         }
         else
         {
            // Get a list of potential targets for this state.
            const TArray<FUtilityStateTarget>& potentialTargets = _CacheOrFindTargetsFor(targetingType, targetingGroup);

            UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * Examining %d targets"), potentialTargets.Num());

            if (potentialTargets.Num() > 0)
            {
               // Calculate score for each potential target.
               for (const FUtilityStateTarget& target : potentialTargets)
               {
                  const float bonus = _GetStateTargetBonus(state, target);
                  const float weight = state.Weight + bonus;
                  bool wasFullyConsidered;

                  _DebugInfoAddStateAndTarget(state, target);
                  const float currentScore = _CalculateStateScore(state, target, weight, topScore, wasFullyConsidered);
                  _DebugInfoAddStateAndTargetScore(state, target, currentScore, bonus, wasFullyConsidered);

                  if (currentScore > topScore)
                  {
                     topScore = currentScore;
                     topScoringState = &state;
                     stateTarget = target;
                  }
               }
            }
            else
            {
               // add debug logging for states that aren't valid because we didn't find any targets
               _DebugInfoAddStateAndTarget(state, FUtilityStateTarget::Invalid);
               _DebugInfoAddConsideration(state, FUtilityStateTarget::Invalid, TEXT("No potential targets found!"), 0.0f);
               _DebugInfoAddStateAndTargetScore(state, FUtilityStateTarget::Invalid, 0.0f, 0.0f, false);
            }
         }
      });
   }

   // Log to the Visual Logger to force an Actor snapshot to be taken. We want to do it after collecting all the snapshot
   // data so that all scores show up.
   UE_VLOG(GetOwner(), LogOSEUtilityAI, Verbose, TEXT("Scored states."));

   UE_LOG(LogOSEUtilityAI, Verbose, TEXT(" * Top scoring state for %s was %s with target %s with %.02f score")
      , _aiController ? _aiController->GetOSECharacter() ? *_aiController->GetOSECharacter()->GetName() : TEXT("None") : TEXT("None")
      , topScoringState ? *topScoringState->Name.ToString() : TEXT("NONE")
      , *stateTarget.ToString()
      , topScore);

   // reset potential targets for our next check
   _ResetPotentialTargetCache();

   outTarget = stateTarget;
   return topScoringState;
}

bool UUtilityAIComponent::_TrySetState(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target)
{
   // Do we want to switch state or target?
   if(state.IsValid() == false)
   {
      // We couldn't find a valid state, so instead of hanging onto the old state and potentially getting
      // locked into an incorrect behavior, we force the current state to exit.
      if (_currentState.IsValid())
      {
         UE_LOG(LogOSEUtilityAI, Verbose, TEXT("**** Exiting state %s as their is no valid states to move to"),
                *_currentState.Name.ToString());
         _ExitState();
      }
      return false;
   }
   if ((state != _currentState || target != _currentStateTarget || (_currentState.IsValid() && _currentState.Instance->IsComplete())))
   {
      UE_LOG(LogOSEUtilityAI, Verbose, TEXT("**** Switching from state %s to %s.")
      , _currentState.IsValid() ? *_currentState.Name.ToString() : TEXT("<none>")
      , state.IsValid() ? *state.Name.ToString() : TEXT("<none>"));

      // exit prev
      if (_currentState.IsValid())
      {
         _ExitState();
      }

      // enter new
      _EnterState(state, target);

      // state changed
      _OnStateChanged(_currentStateTarget, _currentState);

      return true;
   }
   return false;
}

void UUtilityAIComponent::_DebugInfoAddStateAndTarget(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target)
{
#if (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
   UTILITYAI_SCOPE(CalculateStateScore_DebugInfoAddStateAndTarget);
   FStateTargetDebugLogEntry& entry = _stateTargetDebugLogEntries.Emplace_GetRef();
   entry.Evaluator = state.Evaluator;
   entry.Target = target;
   entry.Name = state.Name;
   entry.Weight = state.Weight;
#endif // (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
}

void UUtilityAIComponent::_DebugInfoAddConsideration(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target, FName name, float score) const
{
#if (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
   UTILITYAI_SCOPE(CalculateStateScore_DebugInfoAddConsideration);
   // should already have this entry...
   FStateTargetDebugLogEntry& entry = const_cast<FStateTargetDebugLogEntry&>(_stateTargetDebugLogEntries[_stateTargetDebugLogEntries.Num() - 1]);
   check(entry.Evaluator == state.Evaluator);
   check(entry.Name == state.Name);
   check(entry.Weight == state.Weight);

   FConsiderationDebugLogEntry& considerationEntry = entry.ConsiderationLogEntries.Emplace_GetRef();
   considerationEntry.Name = name;
   considerationEntry.Score = score;
#endif // (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
}

void UUtilityAIComponent::_DebugInfoAddStateAndTargetScore(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target, float score, float bonus, bool wasFullyConsidered)
{
#if (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
   UTILITYAI_SCOPE(CalculateStateScore_DebugInfoAddStateAndTargetScore);
   // should already have this entry...
   FStateTargetDebugLogEntry& entry = _stateTargetDebugLogEntries[_stateTargetDebugLogEntries.Num() - 1];
   check(entry.Evaluator == state.Evaluator);
   check(entry.Name == state.Name);
   check(entry.Weight == state.Weight);

   entry.Score = score;
   entry.Bonus = bonus;
   entry.WasFullyConsidered = wasFullyConsidered;
#endif // (ENABLE_VISUAL_LOG || WITH_GAMEPLAY_DEBUGGER)
}

float UUtilityAIComponent::_CalculateStateScore(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target, float weight, float scoreToBeat, bool& outWasFullyConsidered) const
{
   UTILITYAI_SCOPE(CalculateStateScore);

   outWasFullyConsidered = true;

   check(state.Instance && state.Evaluator);
   const UUtilityStateEvaluator& evaluator = *state.Evaluator;
   const UUtilityAIStateBase& stateInstance = *state.Instance;
   const UOSEAISettings& settings = UOSEAISettings::Get();

   const int numConsiderations = evaluator.GetNumConsiderations();

   if (numConsiderations <= 0)
   {
      UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * State Score: %.02f | Target: %s | No Considerations (Used Weight)"), weight, *target.ToString());
      outWasFullyConsidered = true;
      return weight;
   }

   // Create the consideration context.
   AOSECharacterBase* character = _aiController ? _aiController->GetOSECharacter() : nullptr;
   const bool isInitialStateEvaluation = (_currentState != state || _currentStateTarget != target || (_currentState == state && _currentState.Instance->IsComplete()));
   FConsiderationContext ctx(_aiController, character, this, target, &stateInstance, isInitialStateEvaluation);

   // Ensure the target has any required tokens
   if (evaluator.HasAnyValidTargetTokens())
   {
      UTILITYAI_SCOPE(CalculateStateScore_TokenCheck);

      for (const FOSEAITokenInfo& tokenInfo : evaluator.TargetTokens)
      {
         // skip anything malformed at runtime; this will be caught in validation
         if (!tokenInfo.IsValid())
            continue;

         // state requires a token but...
         // - but the interface/obj isn't implemented for this target, or
         // - it does not currently have the token
         //   - and our current state does not currently have the token
         if (!HasAccessToAITokenForTarget(tokenInfo, target))
         {
            UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * State Score: 0.00 | Target: %s | No Token"), *target.ToString());
            _DebugInfoAddConsideration(state, target, TEXT("No access to token for target"), 0.0f);
            return 0.0f;
         }
      }
   }

   // Since we're multiplying scores between 0 and 1, use a compensation factor.
   // See: https://www.gdcvault.com/play/1021848/Building-a-Better-Centaur-AI around 9:10
   const float modificationFactor = 1.0f - (1.0f / numConsiderations);

   float stateScore = weight;

   {
      UTILITYAI_SCOPE(CalculateStateScore_ConsiderationEvaluations);
   
      evaluator.ForEachConsideration(
         [this, &state, &target, &stateScore, &scoreToBeat, &outWasFullyConsidered, modificationFactor, &ctx, &settings, &isInitialStateEvaluation](const FBehaviorConsideration& consideration, bool& done)
         {
            if (stateScore < scoreToBeat)
            {
               UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * Done with considerations, score %.02f cannot beat score %.02f"), stateScore, scoreToBeat);

               // We can't win anymore, so stop processing and return the current score.
               outWasFullyConsidered = false;
               done = true;
               return;
            }

            if (stateScore < settings.MinStateScore)
            {
               UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * Done with considerations, score %.02f is less than the min state score %.02f"), stateScore, settings.MinStateScore);

               // We can't win anymore, so stop processing and return the current score.
               outWasFullyConsidered = false;
               done = true;
               return;
            }

            // skip disabled considerations
            if (!consideration.Enabled)
            {
               UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * Consideration Skipped | Consideration %s | Disabled"), *consideration.Input->GetName());
               return;
            }

            // skip conditions that have invalid evaluation conditions
            switch(consideration.EvaluationCondition)
            {
            case EConsiderationEvaluationCondition::EvaluateInitialOnly:
               {
                  if (!isInitialStateEvaluation)
                  {
                     UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * Consideration Skipped | Consideration %s | %s"), *consideration.Input->GetName(), *UEnum::GetValueAsString(consideration.EvaluationCondition));
                     return;
                  }
               }
               break;
            case EConsiderationEvaluationCondition::EvaluateOngoingOnly:
               {
                  if (isInitialStateEvaluation)
                  {
                     UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * Consideration Skipped | Consideration %s | %s"), *consideration.Input->GetName(), *UEnum::GetValueAsString(consideration.EvaluationCondition));
                     return;
                  }
               }
               break;
            case EConsiderationEvaluationCondition::EvaluateBoth:
               break; // continue on!
            }

            // Get the consideration's input data value.
            const float inputValue = consideration.Input->GetValue(ctx);

            // Score the input value!
            float score = consideration.ResponseCurve->GetFloatValue(inputValue);
            score = FMath::Clamp(score, 0.0f, 1.0f);

            // If the consideration score is below the minimum, force to zero. 
            if (score < settings.MinConditionalScore)
            {
               // Without this, the curve lookup may return an "almost" zero value - like 0.0000000397503399
               // when multiplied against a large weight it may return a "valid" value, for example 99999 * 0000000397503399 = 0.04
               // leading to weirdness when the state is activated after all considerations failed
               score = 0.f;
            }
            
            // Apply compensation factor to this consideration's score.
            const float makeUpValue = (1.0f - score) * modificationFactor;
            const float finalConsiderationScore = score + (makeUpValue * score);
            stateScore *= finalConsiderationScore;

            _DebugInfoAddConsideration(state, target, consideration.Name, finalConsiderationScore);

            UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * Consideration Score: %.02f | Consideration %s"), finalConsiderationScore, *consideration.Input->GetName());
         }
      );
   }

   // anything below this threshold might as well be zero -- let's not consider it
   if (stateScore < settings.MinStateScore)
   {
      stateScore = 0.0f;
   }

   UE_LOG(LogOSEUtilityAI, Verbose, TEXT("  * State Score: %.02f | Target: %s"), stateScore , *target.ToString());

   return stateScore;
}

float UUtilityAIComponent::_GetStateTargetBonus(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target) const
{
   if (_currentStateTarget == target)
   {
      if (_currentState == state)
      {
         return state.MomentumBonus;
      }

      // Check if the TargetingGroup was set on the state and they match. This allows multiple
      // state to all act in a single larger scope for targeting. Eg, switching between
      // light attack and heavy attack should still favor a consistent target.
      if (_currentState.IsValid() &&
          state.IsValid() &&
          _currentState.Evaluator->TargetingGroup.IsValid() &&
          _currentState.Evaluator->TargetingGroup == state.Evaluator->TargetingGroup)
      {
         return state.MomentumBonus;
      }
   }

   return 0.0f;
}

void UUtilityAIComponent::_ExitState()
{
   if (_currentState.IsValid())
   {
      UE_LOG(LogOSEUtilityAI, Verbose, TEXT("Exiting Utility State %s with target %s")
         , *_currentState.Name.ToString()
         , *_currentStateTarget.ToString());

      UE_VLOG(GetOwner(), LogOSEUtilityAI, Log, TEXT("Exiting Utility State %s with target %s")
         , *_currentState.Name.ToString()
         , *_currentStateTarget.ToString());

      // give all the tokens back that we took when entering the state
      if (_currentState.Evaluator->HasAnyValidTargetTokens())
      {
         if (IsValid(_currentTokenOwnerObj))
         {
            for (const FOSEAITokenInfo& tokenInfo : _currentState.Evaluator->TargetTokens)
            {
               if (!tokenInfo.IsValid())
                  continue;

               _currentTokenOwnerObj->AuthorityGrantAITokenInfo(_currentStateTarget, tokenInfo);

               UE_LOG(LogOSEUtilityAI, Verbose, TEXT("%s granted %d token(s) of type %s back to %s"),
                  _aiController && _aiController->GetPawn() ? *_aiController->GetPawn()->GetName() : TEXT("Null character"),
                  tokenInfo.Count,
                  *tokenInfo.TokenTag.ToString(),
                  *_currentStateTarget.GetTargetUObject()->GetName());
            }
         }
      }

      _currentState.Instance->Exit();

      FUtilityStateEvaluatorInstance exitingState = _currentState;
      FUtilityStateTarget exitingStateTarget = _currentStateTarget;

      // clear the "current" things so that external things that bind to the StateExited() events can call GetCurrent()-type apis
      // without hitting the states we're currently exiting
      _currentState = FUtilityStateEvaluatorInstance::Invalid;
      _currentStateTarget = FUtilityStateTarget::Invalid;
      _currentTokenOwnerObj = nullptr;

      // let subclasses handle the exit, too
      _OnStateExited(exitingStateTarget, exitingState);
      
      // and external systems
      OnAIStateExit.Broadcast(exitingState, exitingStateTarget);
   }
}

void UUtilityAIComponent::_EnterState(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& stateTarget)
{
   check(state.IsValid());

   if ((_aiController == nullptr) || (_aiController->GetPawn() == nullptr))
   {
      UE_VLOG(GetOwner(), LogOSEUtilityAI, Log, TEXT("Failed to enter Utility State %s with target %s due to missing AI Pawn."),
         *state.Name.ToString(),
         *stateTarget.ToString());
      return;
   }

   _currentState = state;
   _currentStateTarget = stateTarget;

   // take all the tokens required to enter this state
   if (_currentState.Evaluator->HasAnyValidTargetTokens())
   {
      // all of this should succeed because we validated the tokens
      // upfront during the consideration pass before deciding to enter this state
      UObject* targetTokenOwnerObject = _currentStateTarget.GetTargetUObject();
      _currentTokenOwnerObj = UUtilityAITokenOwner::AuthorityTryGetTokenOwnerFromObject(targetTokenOwnerObject);
      check(_currentTokenOwnerObj);

      for (const FOSEAITokenInfo& tokenInfo : _currentState.Evaluator->TargetTokens)
      {
         if (!tokenInfo.IsValid())
            continue;

         FOSEAITakeTransactionResult takeTokenInfo = _currentTokenOwnerObj->AuthorityTakeAITokenInfo(_currentStateTarget, tokenInfo);
         check(takeTokenInfo.Success);

         // if taking this token did NOT put us into debt then we can
         // clear any open requests for this target + token pair since we actually "own" it now
         if (!takeTokenInfo.WasDebtToken)
         {
            if (IUtilityAITokenRequesterInterface* sourceTokenRequester = Cast<IUtilityAITokenRequesterInterface>(_GetOwnerCharacter()))
            {
               UUtilityAITokenRequester* sourceTokenRequesterObj = sourceTokenRequester->AuthorityGetTokenRequester();
               check(sourceTokenRequesterObj);

               FFOSEAITokenRequestHandle handle = sourceTokenRequesterObj->FindAITokenRequest(targetTokenOwnerObject, tokenInfo);
               if (handle.IsValid())
               {
                  sourceTokenRequesterObj->CancelAITokenRequest(handle);
               }
            }
         }

         UE_LOG(LogOSEUtilityAI, Verbose, TEXT("%s took %d token(s) of type %s from %s"),
            *_aiController->GetPawn()->GetName(),
            tokenInfo.Count,
            *tokenInfo.TokenTag.ToString(),
            *_currentStateTarget.GetTargetUObject()->GetName());
      }
   }

   UE_VLOG(GetOwner(), LogOSEUtilityAI, Log, TEXT("Entering Utility State %s with target %s"),
      *_currentState.Name.ToString(),
      *_currentStateTarget.ToString());
   UE_LOG(LogOSEUtilityAI, Verbose, TEXT("Entering Utility State %s with target %s"),
      *_currentState.Name.ToString(),
      *_currentStateTarget.ToString());

   _currentState.Instance->Reset();
   _currentState.Instance->Enter();

   // let subclsses handle the enter
   _OnStateEntered(_currentStateTarget, _currentState);

   // and external systems
   OnAIStateEnter.Broadcast(_currentState, _currentStateTarget);
}

FName UUtilityAIComponent::GetCurrentStateName() const
{
   if (!_currentState.IsValid())
   {
      return FName();
   }
   return _currentState.Instance->GetFName();
}

void UUtilityAIComponent::_OnPawnPossessed(APawn* pawn)
{
   if (GetOwner()->HasAuthority())
   {
      if (UUtilityAIManagerWorldSubsystem* utilityAIMgr = GetWorld()->GetSubsystem<UUtilityAIManagerWorldSubsystem>())
      {
         if(_shouldRegisterAsGoalUtilityBehavior)
         {
            utilityAIMgr->RegisterAIGoalComponent(this);
         }
         else
         {
            utilityAIMgr->RegisterAIBehaviorComponent(this);
         }
      }
   }
}

void UUtilityAIComponent::_OnPawnUnPossessed()
{
   _ExitState();

   if (GetOwner()->HasAuthority())
   {
      if (UUtilityAIManagerWorldSubsystem* utilityAIMgr = GetWorld()->GetSubsystem<UUtilityAIManagerWorldSubsystem>())
      {
         
         if(_shouldRegisterAsGoalUtilityBehavior)
         {
            utilityAIMgr->DeregisterAIGoalComponent(this);
         }
         else
         {
            utilityAIMgr->DeregisterAIBehaviorComponent(this);
         }
      }
   }
}

AOSECharacterBase* UUtilityAIComponent::_GetOwnerCharacter() const
{
   if (_aiController)
   {
      return _aiController->GetOSECharacter();
   }
   return nullptr;
}

bool UUtilityAIComponent::HasAccessToAITokenForTarget(const FOSEAITokenInfo& tokenInfo, const FUtilityStateTarget& target) const
{
   const IUtilityAITokenRequesterInterface* sourceTokenRequester = Cast<IUtilityAITokenRequesterInterface>(_GetOwnerCharacter());
   UObject* targetTokenOwnerObject = target.GetTargetUObject();
   UUtilityAITokenOwner* targetTokenOwnerObj = UUtilityAITokenOwner::AuthorityTryGetTokenOwnerFromObject(targetTokenOwnerObject);

   // can't have access to a token if the target isn't a token owner
   if (!targetTokenOwnerObj)
      return false;

   UUtilityAITokenRequester* sourceTokenRequesterObj = sourceTokenRequester ? sourceTokenRequester->AuthorityGetTokenRequester() : nullptr;

   // simple case: there is an available token, so let them take it
   if (targetTokenOwnerObj->AuthorityHasAITokenInfo(target, tokenInfo))
   {
      return true;
   }

   // there was no available token, but we really want one and will go into token debt over it if it's possible
   if (sourceTokenRequesterObj &&
       sourceTokenRequesterObj->HasAITokenRequest(targetTokenOwnerObject, tokenInfo) &&
       targetTokenOwnerObj->AuthorityCanGoIntoTokenDebtFromInfo(target, tokenInfo))
   {
      return true;
   }

   // if we're in token debt for this token we can't have it, even if our current state has access.  we need actors choosing
   // new states to start giving up tokens to get back to our allowed count
   if (targetTokenOwnerObj->AuthorityIsInTokenDebtFromInfo(target, tokenInfo))
   {
      return false;
   }

   // ASSUMPTION: if our current state has this token we're going to grant it + take it back, so we can consider us having it
   const bool currentStateHasToken = 
      _currentState.IsValid() && 
      _currentStateTarget == target &&
      _currentState.Evaluator->ContainsTokenInfo(tokenInfo);

   // there aren't available tokens, and we're not in token debt, so we only have access to the token if our current state is using it
   return currentStateHasToken;
}

const TArray<FUtilityStateTarget>& UUtilityAIComponent::_GetPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const
{
   // empty in base class
   static TArray<FUtilityStateTarget> sEmptyTargetList;
   return sEmptyTargetList;
}

const TArray<FUtilityStateTarget>& UUtilityAIComponent::_CacheOrFindTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup)
{
   // use forced targeting applied on us externally first, if applicable
   // TODO: forced targeting could also use a targeting group tag?
   if (_forcedTargeting.Contains(targeting))
      return _forcedTargeting[targeting];

   // cache targets of this type, if needed
   if (!_HasCachedPotentialTargetsFor(targeting, targetingGroup))
   {
      UTILITYAI_SCOPE(CachePoCachePotentialTargets);
      _CachePotentialTargetsFor(targeting, targetingGroup);

      // enforce that we don't need to re-cache again this frame
      check(_HasCachedPotentialTargetsFor(targeting, targetingGroup));
   }

   // get potential targets for this targeting type
   return _GetPotentialTargetsFor(targeting, targetingGroup);
}


