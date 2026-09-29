// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"

#include "TATTutorialRunnerComponent.generated.h"


struct FTATTutorialStep;
struct FTATTutorialActionHandle;
struct FTATTutorialParams;
struct FTATTutorialFrameResolver;
class UTATTutorialScript;

// Very coarse state to resume at
struct FTATTutorialResumeSnapshot
{
   enum class EState : uint8
   {
      WaitToEnter,
      StartStep,
      Complete
   };

   EState State = EState::Complete;
   int StepIndex = -1;
   // Bitmask of auxiliary tutorials that are still active
   uint32 AuxiliaryMask = 0;
};

// The runtime state of a tutorial state machine
//
// extracted so that there can be multiple of them
// (e.g. auxiliary tutorial in parallel, or sub-tutorial steps)
USTRUCT()
struct FTATTutorialRunnerFrame
{
   GENERATED_BODY()
   
   enum class EState : uint8
   {
      WaitToEnterStep,
      RunStepAction,
      WaitForAction,
      WaitToExitStep,
      Complete
   };

   enum class EUpdateResult : uint8
   {
      Suspend, KeepRunning
   };

   int32 CurrentStepIndex = 0;
   int32 CurrentActionIndex = 0;
   EState State = EState::WaitToEnterStep;

   UPROPERTY(Transient)
   TArray<FTATTutorialActionHandle> ActionHandles;

   void Tick(const FTATTutorialParams& params, TConstArrayView<FTATTutorialStep> steps, const FTATTutorialFrameResolver& resolver);
   EUpdateResult StepOnce(const FTATTutorialParams& params, TConstArrayView<FTATTutorialStep> steps, const FTATTutorialFrameResolver& resolver);

   void ResetActions();
   void StopRunning();
   void StartRunningAtSnapshot(const FTATTutorialResumeSnapshot& snapshot, TConstArrayView<FTATTutorialStep> steps);
   void StartAtBeginning();
   FTATTutorialResumeSnapshot SnapshotStateToResumeTo(TConstArrayView<FTATTutorialStep> steps) const;
};


// A component that runs the assigned tutorial script
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATTutorialRunnerComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATTutorialRunnerComponent();

protected:
   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type endPlayReason) override;

public:
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

   static UTATTutorialRunnerComponent* Find(UWorld* world);

   FTATTutorialResumeSnapshot SnapshotStateToResumeTo() const;
   void StopRunning();
   void StartRunningAtSnapshot(const FTATTutorialResumeSnapshot& snapshot);

private:
   void _OnShowDebugInfo(AHUD* hud, UCanvas* canvas, const FDebugDisplayInfo& displayInfo, float& yl, float& ypos) const;
   
   UPROPERTY(EditAnywhere)
   TObjectPtr<UTATTutorialScript> _tutorialScript = nullptr;

   using EState = FTATTutorialRunnerFrame::EState;

   UPROPERTY(Transient)
   FTATTutorialRunnerFrame _primaryFrame;

   UPROPERTY(Transient)
   TArray<FTATTutorialRunnerFrame> _auxiliaryFrames;
};
