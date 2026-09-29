// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Tasks/Movement/TATStateTreeTaskRotateToFace.h"

// ue
#include "AIController.h"
#include "StateTreeExecutionContext.h"

// tat
#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTaskRotateToFace)

namespace TATStareTreeTaskRotateToFaceHelpers
{
	FORCEINLINE_DEBUGGABLE FVector::FReal CalculateAngleDifferenceDot(const FVector& vectorA, const FVector& vectorB)
	{
		return (vectorA.IsNearlyZero() || vectorB.IsNearlyZero())
			? 1.0f
			: vectorA.CosineAngle2D(vectorB);
	}
}

EStateTreeRunStatus FTATStateTreeTaskRotateToFace::HandleRotation(const FStateTreeExecutionContext& context) const
{
	const FInstanceDataType& instanceData = context.GetInstanceData(*this);
	ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
	if (aiController == nullptr)
	{
		UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRotateToFace::HandleRotation failed since AIController is not a TATAIController"));
		return EStateTreeRunStatus::Failed;
	}

	const APawn* aiPawn = instanceData.AIController->GetPawn();
	if (aiPawn == nullptr)
	{
		UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRegisterForRole::HandleRotation failed since AIController is not a TATAIController"));
		return EStateTreeRunStatus::Failed;
	}

	const float precisionDot = FMath::Cos(FMath::DegreesToRadians(instanceData.Precision));

	const FVector pawnForward = aiPawn->GetActorForwardVector();
	const FVector pawnLocation = aiPawn->GetActorLocation();

	switch (instanceData.Type)
	{
		case ETATStateTreeTaskRotateToFaceType::FaceActor:
		{
			if (const AActor* actor = instanceData.TargetActor)
			{
				const FVector actorLocation = actor->GetActorLocation();
				const FVector::FReal angleDifference = TATStareTreeTaskRotateToFaceHelpers::CalculateAngleDifferenceDot(pawnForward
					, (actorLocation - pawnLocation));

				const bool shouldFinishImmediately = (angleDifference >= precisionDot) && instanceData.EndTaskWhenFacing;
				if (shouldFinishImmediately)
				{
					return EStateTreeRunStatus::Succeeded;
				}
				else
				{
					aiController->SetFocus(instanceData.TargetActor, EAIFocusPriority::Gameplay);
					return EStateTreeRunStatus::Running;
				}
			}
			break;
		}

		case ETATStateTreeTaskRotateToFaceType::FaceLocation:
		{
			if (FAISystem::IsValidLocation(instanceData.TargetLocation))
			{
				const FVector::FReal angleDifference = TATStareTreeTaskRotateToFaceHelpers::CalculateAngleDifferenceDot(pawnForward
					, (instanceData.TargetLocation - pawnLocation));

				const bool shouldFinishImmediately = (angleDifference >= precisionDot) && instanceData.EndTaskWhenFacing;
				if (shouldFinishImmediately)
				{
					return EStateTreeRunStatus::Succeeded;
				}
				else
				{
					aiController->SetFocalPoint(instanceData.TargetLocation, EAIFocusPriority::Gameplay);
					return EStateTreeRunStatus::Running;
				}
			}
			break;
		}

		case ETATStateTreeTaskRotateToFaceType::MatchActorRotation:
		case ETATStateTreeTaskRotateToFaceType::MatchRotator:
		{
			FVector direction = FVector::ZeroVector;

			if (instanceData.Type == ETATStateTreeTaskRotateToFaceType::MatchActorRotation)
			{
				if (const AActor* actor = instanceData.TargetActor)
				{
					direction = actor->GetActorRotation().Vector();
				}
				else
				{
					break;
				}
			}
			else
			{
				if (FAISystem::IsValidRotation(instanceData.TargetRotation))
				{
					direction = instanceData.TargetRotation.Vector();
				}
				else
				{
					break;
				}
			}
			
			const FVector::FReal angleDifference = TATStareTreeTaskRotateToFaceHelpers::CalculateAngleDifferenceDot(pawnForward, direction);

			const bool shouldFinishImmediately = (angleDifference >= precisionDot) && instanceData.EndTaskWhenFacing;
			if (shouldFinishImmediately)
			{
				return EStateTreeRunStatus::Succeeded;
			}
			else
			{
				const FVector focalPoint = pawnLocation + direction * 10000.0f;
				// set focal somewhere far in the indicated direction
				aiController->SetFocalPoint(focalPoint, EAIFocusPriority::Gameplay);
				return EStateTreeRunStatus::Running;
			}
		}

		default: checkNoEntry(); return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FTATStateTreeTaskRotateToFace::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
	return HandleRotation(context);
}

EStateTreeRunStatus FTATStateTreeTaskRotateToFace::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
	return HandleRotation(context);
}

void FTATStateTreeTaskRotateToFace::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
	FInstanceDataType& instanceData = context.GetInstanceData(*this);

	ATATAIController* aiController = Cast<ATATAIController>(instanceData.AIController);
	if (aiController == nullptr)
	{
		UE_VLOG(context.GetOwner(), LogStateTree, Error, TEXT("FTATStateTreeTaskRotateToFace::ExitState failed since AIController is not a TATAIController"));
		return;
	}

	aiController->ClearFocus(EAIFocusPriority::Gameplay);
}
