// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskRotateToFace.generated.h"

class AAIController;

UENUM()
enum class ETATStateTreeTaskRotateToFaceType : uint8
{
   None UMETA(Hidden),

   // AI turns to face the specified actor. Tracks that actor's movement over the lifespan of this task.
   FaceActor,
   // AI matches the world rotation of the specified actor. Does not track changes in actor rotation after EnterState.
   MatchActorRotation,
   // AI turns to face the specified location. Does not track changes to the location after EnterState.
   FaceLocation,
   // AI matches the rotation of the specified rotator. Does not track changes to the rotator after EnterState.
   MatchRotator
};

USTRUCT()
struct FTATStateTreeTaskRotateToFaceData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "In")
   ETATStateTreeTaskRotateToFaceType Type = ETATStateTreeTaskRotateToFaceType::None;

   UPROPERTY(EditAnywhere, Category = "In", meta = (EditCondition = "Type == ETATStateTreeTaskRotateToFaceType::FaceActor || Type == ETATStateTreeTaskRotateToFaceType::MatchActorRotation", EditConditionHides))
   TObjectPtr<AActor> TargetActor = nullptr;

   UPROPERTY(EditAnywhere, Category = "In", meta = (EditCondition = "Type == ETATStateTreeTaskRotateToFaceType::FaceLocation", EditConditionHides))
   FVector TargetLocation = FVector(ForceInit);

   UPROPERTY(EditAnywhere, Category = "In", meta = (EditCondition = "Type == ETATStateTreeTaskRotateToFaceType::MatchRotator", EditConditionHides))
   FRotator TargetRotation = FRotator(ForceInit);

   UPROPERTY(EditAnywhere, Category = "In", meta = (ClampMin = "0.0", Units = deg))
   float Precision = 10.0f;

   // If true, finish this task once the 'Type' condition is met.
   // 
   // Can be set to false to have the AI's rotation continually follow the
   // specified actor/location/rotation, but task will not end itself. Must
   // be ended by some other state transition.
   UPROPERTY(EditAnywhere, Category = "In")
   bool EndTaskWhenFacing = true;
};

USTRUCT()
struct TAT_API FTATStateTreeTaskRotateToFace : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskRotateToFaceData;
	
   FTATStateTreeTaskRotateToFace() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   EStateTreeRunStatus HandleRotation(const FStateTreeExecutionContext& context) const;
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
