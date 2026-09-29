// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Engine/DataAsset.h"

#include "TATTutorialScript.generated.h"

class UTATTutorialCondition;
class UTATTutorialAction;

UENUM()
enum class ETATTutorialStepCheckpointBehavior : uint8
{
   // Stays on the current step
   UseCurrent,
   // Rolls back to the previous step
   RollbackToPrevious,
};

USTRUCT()
struct FTATTutorialStep
{
   GENERATED_BODY()

   // The name of the step for debugging purposes
   UPROPERTY(EditAnywhere)
   FString StepName;

   // The requirement for entering the step
   UPROPERTY(EditAnywhere, Instanced)
   TObjectPtr<UTATTutorialCondition> EnterCondition;

   // The requirement for exiting the step
   // It will finish all running actions before checking the step
   UPROPERTY(EditAnywhere, Instanced)
   TObjectPtr<UTATTutorialCondition> ExitCondition;

   // if condition is met, will skip past the step, not running any actions
   UPROPERTY(EditAnywhere, Instanced)
   TObjectPtr<UTATTutorialCondition> SkipCondition;

   // What step to use if the tutorial is suspended and resumed at a checkpoint
   UPROPERTY(EditAnywhere)
   ETATTutorialStepCheckpointBehavior CheckpointBehavior = ETATTutorialStepCheckpointBehavior::UseCurrent;

   // The actions that are run while in the step
   UPROPERTY(EditAnywhere, Instanced, meta=(TitleProperty="{_debugNameForEditor}"))
   TArray<TObjectPtr<UTATTutorialAction>> Actions;
};

UCLASS()
class TAT_API UTATTutorialScript : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, meta = (TitleProperty="{StepName}"))
   TArray<FTATTutorialStep> Steps;

   // Other Tutorials that will be run in parallel with the tutorial
   // For optional things that may happen at any point in the tutorial
   //
   // NOTES:
   //   - Still only intended for in-level use, since it polls and does not persist
   //   - Since the primary tutorial may in any state, it must take care not to stomp
   //     on the primary tutorial, such as waiting for screens to close. But this is left
   //     up to the tutorial content. 
   //   - On Revive, it will restore to either unstarted or complete. No partial progress
   UPROPERTY(EditAnywhere)
   TArray<TObjectPtr<UTATTutorialScript>> AuxiliaryTutorials;

#if WITH_EDITOR
   EDataValidationResult IsDataValid(class FDataValidationContext& context) const override;
   void ValidateSteps(const UWorld* optionalWorld, TFunctionRef<void (const FText&)> reportError) const;
#endif
};
