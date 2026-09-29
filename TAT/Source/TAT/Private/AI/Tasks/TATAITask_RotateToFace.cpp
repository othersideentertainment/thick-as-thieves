// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Tasks/TATAITask_RotateToFace.h"

// ue
#include "AIController.h"
#include "GameplayTaskOwnerInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAITask_RotateToFace)

DEFINE_LOG_CATEGORY(LogTATAITask_RotateToFace);

namespace TATAITaskRotateToFaceHelpers
{
   FORCEINLINE_DEBUGGABLE static FVector::FReal CalculateAngleDifferenceDot(const FVector& VectorA, const FVector& VectorB)
   {
      return (VectorA.IsNearlyZero() || VectorB.IsNearlyZero())
         ? 1.f
         : VectorA.CosineAngle2D(VectorB);
   }
}

namespace AIRotationCVars
{
   static int DebugDrawRotateToFace = 0;
   FAutoConsoleVariableRef CVarDebugRotation(
      TEXT("TAT.AI.DebugRotateToFace"),
      DebugDrawRotateToFace,
      TEXT("Draw debug for rotating to face"),
      ECVF_Default);
}

UTATAITask_RotateToFace::UTATAITask_RotateToFace(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   bTickingTask = true;
}

// static
UTATAITask_RotateToFace* UTATAITask_RotateToFace::RotateToFaceInitialize(AAIController* controller, TScriptInterface<IGameplayTaskOwnerInterface> taskOwner, float precision)
{
   if (controller == nullptr)
   {
      UE_LOG(LogTATAITask_RotateToFace, Error, TEXT("UTATAITask_RotateToFace called with a null controller!"));
      return nullptr;
   }

   // must have a controlled pawn to rotate
   if (controller->GetPawn() == nullptr)
   {
      UE_LOG(LogTATAITask_RotateToFace, Error, TEXT("UTATAITask_RotateToFace called with a controller (%s) not yet controlling a pawn!"), *controller->GetName());
      return nullptr;
   }

   UTATAITask_RotateToFace* myTask = UAITask::NewAITask<UTATAITask_RotateToFace>(*controller, *taskOwner.GetInterface(), EAITaskPriority::High);
   if (myTask == nullptr)
   {
      UE_LOG(LogTATAITask_RotateToFace, Error, TEXT("UTATAITask_RotateToFace failed to create task for %s!"), *controller->GetName());
      return nullptr;
   }

   myTask->PrecisionDot = FMath::Cos(FMath::DegreesToRadians(precision));
   myTask->WasSuccessful = false;

   return myTask;
}

// static
UTATAITask_RotateToFace* UTATAITask_RotateToFace::RotateToFaceLocation(AAIController* controller, FVector location, float precision)
{
   TScriptInterface<IGameplayTaskOwnerInterface> taskOwner = controller;
   return RotateToFaceLocation(controller, location, taskOwner, precision);
}

// static
UTATAITask_RotateToFace* UTATAITask_RotateToFace::RotateToFaceLocation(AAIController* controller, FVector location, TScriptInterface<IGameplayTaskOwnerInterface> taskOwner, float precision)
{
   if (UTATAITask_RotateToFace* myTask = RotateToFaceInitialize(controller, taskOwner, precision))
   {
      // cache this to compare in TickTask to ensure something else doesn't
      // override the Focal Point we desired to rotate to
      myTask->LocationToFace = location;

      // setting the focal point is what actually does the rotational work
      controller->SetFocalPoint(location, EAIFocusPriority::Gameplay);

      return myTask;
   }

   return nullptr;
}

// static
UTATAITask_RotateToFace* UTATAITask_RotateToFace::RotateToFaceDirection(AAIController* controller, FRotator direction, float precision)
{
   TScriptInterface<IGameplayTaskOwnerInterface> taskOwner = controller;
   return RotateToFaceDirection(controller, direction, taskOwner, precision);
}

// static
UTATAITask_RotateToFace* UTATAITask_RotateToFace::RotateToFaceDirection(AAIController* controller, FRotator direction, TScriptInterface<IGameplayTaskOwnerInterface> taskOwner, float precision)
{
   if (UTATAITask_RotateToFace* myTask = RotateToFaceInitialize(controller, taskOwner, precision))
   {
      // assumes pawn turning occurs around the pawn's location
      const APawn* pawn = controller->GetPawn();
      const FVector pawnLocation = pawn->GetActorLocation();

      // use the eye's Z-location to prevent unnecessary pitch-ing
      FVector eyesLocation = FVector::ZeroVector;
      FRotator eyesRotation = FRotator::ZeroRotator;
      controller->GetActorEyesViewPoint(eyesLocation, eyesRotation);

      // extend the direction vector a bit to account for precision issues
      const FVector directionLocation = FVector(pawnLocation.X, pawnLocation.Y, eyesLocation.Z) + direction.Vector() * 100.0f;

      // cache this to compare in TickTask to ensure something else doesn't
      // override the Focal Point we desired to rotate to
      myTask->LocationToFace = directionLocation;

      // setting the focal point is what actually does the rotational work
      controller->SetFocalPoint(directionLocation, EAIFocusPriority::Gameplay);

      return myTask;
   }

   return nullptr;
}

void UTATAITask_RotateToFace::TickTask(float deltaTime)
{
   Super::TickTask(deltaTime);

   // the actual AI rotation does not occur here, but instead from within the AI's controller
   // via previous setting of the Focal Point which updates the AI's control rotation
   // here, we're just checking whether the AI's current rotation is within some precision
   // of where we intend it to be (based on the Focal Point set in initialization of the task)

   if (OwnerController == NULL || OwnerController->GetPawn() == NULL)
   {
      WasSuccessful = false;
      OnFailed.Broadcast();
      EndTask();
   }
   else
   {
      const FVector pawnDirection = OwnerController->GetPawn()->GetActorForwardVector();
      const FVector focalPoint = OwnerController->GetFocalPointForPriority(EAIFocusPriority::Gameplay);

      // if the current focal point doesn't match our cached value
      // then someone else has changed the focal point - let's end if that occurs
      if (focalPoint != FAISystem::InvalidLocation && focalPoint == LocationToFace)
      {
         const FVector actorPosition = OwnerController->GetPawn()->GetActorLocation();
#if ENABLE_DRAW_DEBUG
         if(AIRotationCVars::DebugDrawRotateToFace != 0)
         {
            DrawDebugDirectionalArrow(GetWorld(), actorPosition, actorPosition + pawnDirection * 100.f, 1.f, FColor::Green, false, 0.f);
            DrawDebugDirectionalArrow(GetWorld(), actorPosition, actorPosition + (focalPoint - OwnerController->GetPawn()->GetActorLocation()).GetSafeNormal2D() * 100.f, 1.f, FColor::Orange, false, 0.f);
         }
#endif
         
         if (TATAITaskRotateToFaceHelpers::CalculateAngleDifferenceDot(pawnDirection, focalPoint - actorPosition) >= PrecisionDot)
         {
            WasSuccessful = true;
            OnSucceeded.Broadcast();
            EndTask();
         }
      }
      else
      {
         WasSuccessful = false;
         OnFailed.Broadcast();
         EndTask();
      }
   }
}

void UTATAITask_RotateToFace::OnDestroy(bool bInOwnerFinished)
{
   if (OwnerController != nullptr)
   {
      OwnerController->ClearFocus(EAIFocusPriority::Gameplay);
   }
   Super::OnDestroy(bInOwnerFinished);
}
