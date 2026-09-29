// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// This is almost a carbon copy of Engine/Plugins/Runtime/GameplayInteractions/Source/GameplayInteractionsModule/Private/StateTree/StateTreeTask_PlayContextualAnim.h
// Because we want to make our own modifications without also modifying the engine version I decided to duplicate it, which has an added benefit of
// removing the dependency on the GameplayInteractions Module.
#pragma once

// ue
#include "CoreMinimal.h"
#include "StateTreeExecutionTypes.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTask_PlayContextualAnim.generated.h"

class UContextualAnimSceneActorComponent;
struct FContextualAnimWarpTarget;
class UContextualAnimSceneAsset;
class AActor;

UENUM(BlueprintType)
enum class ETATPlayContextualAnimExecutionMethod : uint8
{
	/** Start a new interaction */
	StartInteraction,

	/** Join an existing interaction */
	JoinInteraction,

	/** Transition a single actor in an interaction to a different section */
	TransitionAllActors,

	/** Transition all the actors in an interaction to a different sections */
	TransitionSingleActor
};

UCLASS()
class UTATStateTreeTask_PlayContextualAnim_InstanceData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> PrimaryActor = nullptr;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> SecondaryActor = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName SecondaryRole;

	UPROPERTY(EditAnywhere, Category = Parameter)
	TObjectPtr<AActor> TertiaryActor = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName TertiaryRole;

	UPROPERTY(EditAnywhere, Category = Parameter)
	TObjectPtr<const UContextualAnimSceneAsset> SceneAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName SectionName = NAME_None;

	UPROPERTY(EditAnywhere, Category = Parameter)
	ETATPlayContextualAnimExecutionMethod ExecutionMethod = ETATPlayContextualAnimExecutionMethod::StartInteraction;

	/**
	 * There are cases where we may be 'chaining' multiple interactions animations through different state tree tasks (e.g start -> loop -> end)
	 * This could cause visual artifacts specially noticeable in multiplayer environment when the replicated package to start the next animation may not arrive right after the current animation ends.
	 * To avoid this we can set the montage to do not auto blend out and add a Montage Notify at the end of the animation to let us know that we reached the end
	 *
	 * Note: This is really a workaround. Ideally we should have an event that let us know when the montage reaches its end regardless of the Auto Blend Out property.
	*/
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bWaitForNotifyEventToEnd = false;

	/** When bWaitForNotifyEventToEnd is set to true this is the name of the notify that we will look for to signal the end of the task */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (EditCondition = "bWaitForNotifyEventToEnd"))
	FName NotifyEventNameToEnd = NAME_None;

	/** How many times the animation should be run before completing the task. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (EditCondition = "!bLoopForever", ClampMin = "1"))
	int32 LoopsToRun = 1;

	/** If true the animation will loop forever until a transition stops it. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bLoopForever = false;

	/** How many seconds should we wait between each animation loop (won't be used if RunLoops is equal 1). */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float DelayBetweenLoops = 0.f;

	/** Adds random range to the DelayBetweenLoops. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float RandomDeviationBetweenLoops = 0.f;

	/** Manual warp targets for specific roles. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	TArray<FContextualAnimWarpTarget> WarpTargets;

   UPROPERTY(EditAnywhere, Category = Parameter)
   bool bShouldExitOutOfContextualAnimOnCleanUp = true;

	/** Internal count of how many loops have been completed. The Task finishes when the this is equal the provided RunLoops. */
	int32 CompletedLoops = 0;

	/** It can happen that the montage is interrupted, in which case we want to fail the task. */
	bool bMontageInterrupted = false;

	/** Handling of specific case: when we forcibly start a new montage from a notify event,
	 * an interruption will happen from the previous event, so we need to make sure to ignore the first occurrence. */
	bool bExpectInterruptionFromNotifyEventLoop = false;

	/** Used internally to handle the delay between loops. When is set and <= 0 we run a new animation loop. */
	TOptional<float> TimeBeforeStartingNewLoop;
	
	/** Cached value that we reset on task exit. */
	bool bPreviousForceControlRotationFromPawnOrientationForPrimaryActor = false;
	
	/** Cached value that we reset on task exit. */
	bool bPreviousForceControlRotationFromPawnOrientationForSecondaryActor = false;
	
	/** Cached value that we reset on task exit. */
	bool bPreviousForceControlRotationFromPawnOrientationForTertiaryActor = false;

	EStateTreeRunStatus OnEnterState(const FStateTreeExecutionContext& context);

	/** Start a new interaction */
	bool StartContextualAnim(const FStateTreeExecutionContext& context) const;
	/** Join an existing interaction */
	bool JoinContextualAnim(const FStateTreeExecutionContext& context) const;
	/** Transition a single actor in an interaction to a different section */
	bool TransitionSingleActor(const FStateTreeExecutionContext& context) const;
	/** Transition all the actors in an interaction to a different sections */
	bool TransitionAllActors(const FStateTreeExecutionContext& context) const;

	void OnExitState();
   
   EStateTreeRunStatus OnTick(const FStateTreeExecutionContext& context, float deltaTime);
   EStateTreeRunStatus Play(const FStateTreeExecutionContext& context);

   void CleanUp();

   UFUNCTION()
   void OnLeftScene(UContextualAnimSceneActorComponent* sceneActorComponent);

   UFUNCTION()
   void OnNotifyBeginReceived(FName notifyName, const FBranchingPointNotifyPayload& branchingPointNotifyPayload);
};

USTRUCT(meta = (DisplayName = "[TAT] Play Contextual Anim"))
struct FTATStateTreeTask_PlayContextualAnim : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()

   using UInstanceDataType = UTATStateTreeTask_PlayContextualAnim_InstanceData;

   FTATStateTreeTask_PlayContextualAnim();

   virtual const UStruct* GetInstanceDataType() const override { return UInstanceDataType::StaticClass(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
};
