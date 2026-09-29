// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tutorial/TATTutorialRunnerComponent.h"

// tat
#include "Player/TATPlayerState.h"
#include "Tutorial/TATTutorialAction.h"
#include "Tutorial/TATTutorialCondition.h"
#include "Tutorial/TATTutorialParams.h"
#include "Tutorial/TATTutorialScript.h"

// ue
#include "Engine/Canvas.h"
#include "GameFramework/Character.h"
#include "GameFramework/HUD.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTutorialRunnerComponent)

namespace TutorialRunnerHelpers
{
   static TConstArrayView<FTATTutorialStep> GetStepsSafe(const UTATTutorialScript* script)
   {
      if(script)
      {
         return script->Steps;
      }
      return {};
   }
}

// Basically a manual closure to look up a tutorial frame safely
//
// Don't want to assume its address is stable as long as the runner is alive,
// since it might be in an array that is reallocated
//
// Doing it manually gets:
// Only do the weak ptr handling once
// Defers heap allocations
struct FTATTutorialFrameResolver
{
   using FLookupFunc = FTATTutorialRunnerFrame* (*)(UTATTutorialRunnerComponent&, int);
   TWeakObjectPtr<UTATTutorialRunnerComponent> Runner;
   FLookupFunc LookupFunc = nullptr;
   int Context = 0;

   FTATTutorialRunnerFrame* Resolve() const
   {
      if(UTATTutorialRunnerComponent* runner = Runner.Get())
      {
         if(LookupFunc)
         {
            return LookupFunc(*runner, Context);
         }
      }

      return nullptr;
   }
};

void FTATTutorialRunnerFrame::Tick(const FTATTutorialParams& params, TConstArrayView<FTATTutorialStep> steps, const FTATTutorialFrameResolver& resolver)
{
   while (StepOnce(params, steps, resolver) == EUpdateResult::KeepRunning)
   {}
}

FTATTutorialRunnerFrame::EUpdateResult FTATTutorialRunnerFrame::StepOnce(const FTATTutorialParams& params, TConstArrayView<FTATTutorialStep> steps, const FTATTutorialFrameResolver& resolver)
{
   auto shouldSkip = [](const FTATTutorialStep& step, const FTATTutorialParams& params)
      {
         return step.SkipCondition && step.SkipCondition->IsMet(params);
      };

   // Basically a state machine that runs until it needs to suspend
   switch(State)
   {
   case EState::WaitToEnterStep:
      {
         if(!steps.IsValidIndex(CurrentStepIndex))
         {
            State = EState::Complete;
            return EUpdateResult::Suspend;
         }

         const FTATTutorialStep& step = steps[CurrentStepIndex];
         if (shouldSkip(step, params))
         {
            State = EState::WaitToExitStep;
            return EUpdateResult::KeepRunning;
         }

         bool enterConditionMet = step.EnterCondition == nullptr || step.EnterCondition->IsMet(params);
         if(!enterConditionMet)
         {
            return EUpdateResult::Suspend;
         }
         CurrentActionIndex = 0;
         State = EState::RunStepAction;
         return EUpdateResult::KeepRunning;
      }
   case EState::RunStepAction:
      {
         check(steps.IsValidIndex(CurrentStepIndex));
         const FTATTutorialStep& step = steps[CurrentStepIndex];
         if(!step.Actions.IsValidIndex(CurrentActionIndex))
         {
            State = EState::WaitToExitStep;
            return EUpdateResult::KeepRunning;
         }

         const UTATTutorialAction* action = step.Actions[CurrentActionIndex];
         if(!ensure(action))
         {
            CurrentActionIndex++;
            return EUpdateResult::KeepRunning;
         }

         // Set to waiting until the action finishes (which could be immediately)
         State = EState::WaitForAction;
         ActionHandles.Add(action->Run(params, [resolver, stepIndex = CurrentStepIndex, actionIndex = CurrentActionIndex]()
         {
            if(FTATTutorialRunnerFrame* self = resolver.Resolve())
            {
               // If still waiting for this action, go to the next action
               if(self->State == EState::WaitForAction && self->CurrentStepIndex == stepIndex && self->CurrentActionIndex == actionIndex)
               {
                  self->State = EState::RunStepAction;
                  self->CurrentActionIndex++;
               }
            }
         }));
         return State == EState::RunStepAction ? EUpdateResult::KeepRunning : EUpdateResult::Suspend;
      }
   case EState::WaitForAction:
      {
         check(steps.IsValidIndex(CurrentStepIndex));
         const FTATTutorialStep& step = steps[CurrentStepIndex];
         if (shouldSkip(step, params))
         {
            State = EState::WaitToExitStep;
            return EUpdateResult::KeepRunning;
         }

         // nothing to do here
         return EUpdateResult::Suspend;
      }
   case EState::WaitToExitStep:
      {
         check(steps.IsValidIndex(CurrentStepIndex));
         const FTATTutorialStep& step = steps[CurrentStepIndex];
         const bool canExit = step.ExitCondition == nullptr || step.ExitCondition->IsMet(params);
         if(!canExit && !shouldSkip(step, params))
         {
            return EUpdateResult::Suspend;
         }
         
         // complete step
         State = EState::WaitToEnterStep;
         CurrentStepIndex++;
         CurrentActionIndex = 0;
         ResetActions();
         return EUpdateResult::KeepRunning;
      }
   case EState::Complete:
      // nothing to do here
      return EUpdateResult::Suspend;
   }
   return EUpdateResult::Suspend;
}

void FTATTutorialRunnerFrame::ResetActions()
{
   for (FTATTutorialActionHandle& handle : ActionHandles)
   {
      if (handle.CancelAction)
      {
         handle.CancelAction();
      }
   }
   ActionHandles.Reset();
}

void FTATTutorialRunnerFrame::StopRunning()
{
   ResetActions();
   State = EState::Complete;
   CurrentActionIndex = 0;
   CurrentStepIndex = 0;
}

void FTATTutorialRunnerFrame::StartRunningAtSnapshot(const FTATTutorialResumeSnapshot& snapshot, TConstArrayView<FTATTutorialStep> steps)
{
   StopRunning();

   using ESnapshotState = FTATTutorialResumeSnapshot::EState;

   if(snapshot.State == ESnapshotState::Complete)
   {
      return;
   }

   // We should have produced the snapshot, but can't fully trust it
   if(!ensure(steps.IsValidIndex(snapshot.StepIndex)))
   {
      return;
   }

   State = snapshot.State == ESnapshotState::WaitToEnter ? EState::WaitToEnterStep : EState::RunStepAction;
   CurrentStepIndex = snapshot.StepIndex;
   CurrentActionIndex = 0;
}

void FTATTutorialRunnerFrame::StartAtBeginning()
{
   State = EState::WaitToEnterStep;
   CurrentStepIndex = 0;
   CurrentActionIndex = 0;
}

FTATTutorialResumeSnapshot FTATTutorialRunnerFrame::SnapshotStateToResumeTo(TConstArrayView<FTATTutorialStep> steps) const
{
   using ESnapshotState = FTATTutorialResumeSnapshot::EState;
   
   if(State == EState::Complete)
   {
      return FTATTutorialResumeSnapshot {.State = ESnapshotState::Complete};
   }
   else
   {
      // walk backwards while states want to roll back
      int stepToUse = CurrentStepIndex;
      while (stepToUse > 0 && steps[stepToUse].CheckpointBehavior == ETATTutorialStepCheckpointBehavior::RollbackToPrevious)
      {
         stepToUse--;
      }

      const ESnapshotState snapshotState = State == EState::WaitToEnterStep && CurrentStepIndex == stepToUse ?
         ESnapshotState::WaitToEnter : ESnapshotState::StartStep;
      return FTATTutorialResumeSnapshot {.State = snapshotState, .StepIndex = stepToUse};
   }
}

UTATTutorialRunnerComponent::UTATTutorialRunnerComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTATTutorialRunnerComponent::BeginPlay()
{
   Super::BeginPlay();

   if(_tutorialScript)
   {
      SetComponentTickEnabled(true);

      _auxiliaryFrames.SetNum(_tutorialScript->AuxiliaryTutorials.Num());
   }

   AHUD::OnShowDebugInfo.AddUObject(this, &ThisClass::_OnShowDebugInfo);
}

void UTATTutorialRunnerComponent::EndPlay(EEndPlayReason::Type endPlayReason)
{
   AHUD::OnShowDebugInfo.RemoveAll(this);
   Super::EndPlay(endPlayReason);
}

void UTATTutorialRunnerComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   check(_tutorialScript);

   FTATTutorialParams params;
   params.World = GetWorld();
   params.Controller = params.World->GetFirstPlayerController();
   params.Character = params.Controller ? params.Controller->GetPawn<ACharacter>() : nullptr;
   params.PlayerState = params.Controller ? params.Controller->GetPlayerState<ATATPlayerState>() : nullptr;
   
   _primaryFrame.Tick(params, _tutorialScript->Steps, FTATTutorialFrameResolver {
      .Runner = this,
      .LookupFunc = +[](UTATTutorialRunnerComponent& runner, int) { return &runner._primaryFrame; }
   });
   
   check(_tutorialScript->AuxiliaryTutorials.Num() >= _auxiliaryFrames.Num());
   for(int i = 0; i < _auxiliaryFrames.Num(); i++)
   {
      _auxiliaryFrames[i].Tick(params,
         TutorialRunnerHelpers::GetStepsSafe(_tutorialScript->AuxiliaryTutorials[i]),
         FTATTutorialFrameResolver {
            .Runner = this,
            .LookupFunc = +[](UTATTutorialRunnerComponent& runner, int index) { return runner._auxiliaryFrames.IsValidIndex(index) ? &runner._auxiliaryFrames[index] : nullptr; },
            .Context = i
         });
   }
}

FTATTutorialResumeSnapshot UTATTutorialRunnerComponent::SnapshotStateToResumeTo() const
{
   FTATTutorialResumeSnapshot snapshot = _primaryFrame.SnapshotStateToResumeTo(_tutorialScript->Steps);

   check(_auxiliaryFrames.Num() <= sizeof(FTATTutorialResumeSnapshot::AuxiliaryMask)*8);
   for(int i = 0; i < _auxiliaryFrames.Num(); i++)
   {
      if(_auxiliaryFrames[i].State != EState::Complete)
      {
         snapshot.AuxiliaryMask |= 1 << i;
      }
   }

   return snapshot;
}

void UTATTutorialRunnerComponent::StopRunning()
{
   _primaryFrame.StopRunning();
   for(FTATTutorialRunnerFrame& frame : _auxiliaryFrames)
   {
      frame.StopRunning();
   }
}

void UTATTutorialRunnerComponent::StartRunningAtSnapshot(const FTATTutorialResumeSnapshot& snapshot)
{
   _primaryFrame.StartRunningAtSnapshot(snapshot, _tutorialScript->Steps);

   // Aux tutorial either resume at beginning or are complete
   check(_auxiliaryFrames.Num() <= sizeof(FTATTutorialResumeSnapshot::AuxiliaryMask)*8);
   for(int i = 0; i < _auxiliaryFrames.Num(); i++)
   {
      if((snapshot.AuxiliaryMask & (1 << i)) != 0)
      {
         _auxiliaryFrames[i].StartAtBeginning();
      }
      else
      {
         _auxiliaryFrames[i].StopRunning();
      }
   }
}

#if WITH_EDITOR
void UTATTutorialRunnerComponent::CheckForErrors()
{
   Super::CheckForErrors();

   if (_tutorialScript)
   {
      FMessageLog messageLog("MapCheck");
      auto reportError = [&messageLog](const FText& message)
      {
         messageLog.Error(message);
      };
      
      _tutorialScript->ValidateSteps(GetWorld(), reportError);

      for(const UTATTutorialScript* tutorial : _tutorialScript->AuxiliaryTutorials)
      {
         if(tutorial)
         {
            tutorial->ValidateSteps(GetWorld(), reportError);
         }
      }
   }
}
#endif

UTATTutorialRunnerComponent* UTATTutorialRunnerComponent::Find(UWorld* world)
{
   // This is only use infrequently in the tutorial, so should be good enough
   // (relative to putting it somewhere it can be found more easily)
   for(UTATTutorialRunnerComponent* runner : TObjectRange<UTATTutorialRunnerComponent>())
   {
      if(runner && runner->GetWorld() == world)
      {
         return runner;
      }
   }

   return nullptr;
}

void UTATTutorialRunnerComponent::_OnShowDebugInfo(AHUD* hud, UCanvas* canvas, const FDebugDisplayInfo& displayInfo, float& yl, float& ypos) const
{
   // TODO: probably check for the category
   
   static constexpr auto stateToString = [] (EState state)-> const TCHAR*
   {
      switch(state)
      {
      case EState::WaitToEnterStep:
         return TEXT("WaitToEnterStep");
      case EState::RunStepAction:
         return TEXT("RunStepAction");
      case EState::WaitForAction:
         return TEXT("WaitForAction");
      case EState::WaitToExitStep:
         return TEXT("WaitToExitStep");
      case EState::Complete:
         return TEXT("Complete");
      default:
         checkNoEntry();
         return TEXT("UNKNOWN");
      }
   };
   
   FDisplayDebugManager& displayDebugManager = canvas->DisplayDebugManager;

   displayDebugManager.SetFont(GEngine->GetMediumFont());
   displayDebugManager.SetDrawColor(FColor::White);
   // Just skip down a bit so it doesn't overlap HUD objectives
   displayDebugManager.GetYPosRef() += 100;

   displayDebugManager.DrawString(TEXT("TUTORIAL"));

   if (!IsValid(_tutorialScript))
   {
      displayDebugManager.DrawString(TEXT("No Tutorial Script"));
      return;
   }

   auto drawFrame = [&](const FTATTutorialRunnerFrame& frame, TConstArrayView<FTATTutorialStep> steps)
   {
      const FTATTutorialStep* step = steps.IsValidIndex(frame.CurrentStepIndex) ? &steps[frame.CurrentStepIndex] : nullptr;
      displayDebugManager.DrawString(FString::Printf(TEXT("State: %s"), stateToString(frame.State)));

      if (frame.State != EState::Complete)
      {
         displayDebugManager.DrawString(FString::Printf(TEXT("Step: %d - %s"), frame.CurrentStepIndex, step ? *step->StepName : TEXT("NONE")));
      }
      if (step)
      {
         auto printCondition = [&](const TCHAR* label, const UTATTutorialCondition* condition)
         {
            displayDebugManager.DrawString(FString::Printf(TEXT("%s: %s"), label, condition ? *condition->GetDebugName() : TEXT("NONE")));
         };
      
         if (frame.State == EState::WaitForAction)
         {
            const UTATTutorialAction* action = step->Actions.IsValidIndex(frame.CurrentActionIndex) ? step->Actions[frame.CurrentActionIndex] : nullptr;
            displayDebugManager.DrawString(FString::Printf(TEXT("Action: %d - %s"), frame.CurrentActionIndex, action ? *action->GetDebugName() : TEXT("NONE")));
         }
         else if (frame.State == EState::WaitToEnterStep)
         {
            printCondition(TEXT("EnterCondition"), step->EnterCondition);
         }
         else if (frame.State == EState::WaitToExitStep)
         {
            printCondition(TEXT("ExitCondition"), step->ExitCondition);
         }

         if (step->SkipCondition)
         {
            printCondition(TEXT("SkipCondition"), step->SkipCondition);
         }
      }
   };

   drawFrame(_primaryFrame, _tutorialScript->Steps);

   if(_auxiliaryFrames.Num())
   {
      displayDebugManager.DrawString(TEXT("\nAux Tutorials:"));

      for(int i = 0; i < _auxiliaryFrames.Num(); i++)
      {
         drawFrame(_auxiliaryFrames[i], TutorialRunnerHelpers::GetStepsSafe(_tutorialScript->AuxiliaryTutorials[i]));
      }
   }

}

