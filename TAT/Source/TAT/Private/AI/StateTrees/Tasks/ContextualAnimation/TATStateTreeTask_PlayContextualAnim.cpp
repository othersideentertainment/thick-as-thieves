// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/ContextualAnimation/TATStateTreeTask_PlayContextualAnim.h"

// ue
#include "ContextualAnimSceneActorComponent.h"
#include "ContextualAnimSceneAsset.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "ContextualAnimUtilities.h"
#include "StateTreeExecutionContext.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTask_PlayContextualAnim)

//-----------------------------------------------------
// UStateTreeTask_PlayContextualAnim_InstanceData
//-----------------------------------------------------

EStateTreeRunStatus UTATStateTreeTask_PlayContextualAnim_InstanceData::OnEnterState(const FStateTreeExecutionContext& context)
{
	if (SceneAsset == nullptr || !SceneAsset->HasValidData())
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Invalid Scene Asset"), __FUNCTION__);
		return EStateTreeRunStatus::Failed;
	}

	// PrimaryActor is mandatory, this would be the interactable object and will be always bound to the Primary role in the SceneAsset
	if (PrimaryActor == nullptr)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Invalid PrimaryActor"), __FUNCTION__);
		return EStateTreeRunStatus::Failed;
	}

	// Secondary actor is also mandatory (for this type of interactions to make sense we need at least two actors). Role needs to be explicitly defined
	if (SecondaryActor == nullptr || SecondaryRole == NAME_None)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Invalid SecondaryActor (%s) or Role (%s)"),
			__FUNCTION__, *GetNameSafe(SecondaryActor), *SecondaryRole.ToString());
		return EStateTreeRunStatus::Failed;
	}

	UAnimInstance* AnimInstance = UContextualAnimUtilities::TryGetAnimInstance(SecondaryActor);
	if (AnimInstance == nullptr)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Invalid AnimInstance. Actor: %s"),
			__FUNCTION__, *GetNameSafe(SecondaryActor));
		return EStateTreeRunStatus::Failed;
	}

	const EStateTreeRunStatus Result = Play(context);

	if (Result == EStateTreeRunStatus::Failed)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Failed to play the interaction"), __FUNCTION__);
		return EStateTreeRunStatus::Failed;
	}
	
	UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Log, TEXT("%hs: %s - %s"), __FUNCTION__, *GetName(), *UEnum::GetValueAsString(Result));

	AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &UTATStateTreeTask_PlayContextualAnim_InstanceData::OnNotifyBeginReceived);

	return EStateTreeRunStatus::Running;
}

bool UTATStateTreeTask_PlayContextualAnim_InstanceData::StartContextualAnim(const FStateTreeExecutionContext& context) const
{
	int32 SectionIdx = SceneAsset->GetSectionIndex(SectionName);
	if (SectionIdx == INDEX_NONE)
	{
		SectionIdx = 0;
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs. '%s' is not a valid section name in %s. Falling back to first section!"),
			__FUNCTION__, *SectionName.ToString(), *GetNameSafe(SceneAsset));
	}

	// Randomly select the set to play if there is more than one for the given section
	// @TODO: Temporarily here until we move it to the contextual anim plugin as part of the selection mechanism. 

	int32 AnimSetIdx = 0;
	const int32 NumSets = SceneAsset->GetNumAnimSetsInSection(SectionIdx);
	if (NumSets > 1)
	{
		const FContextualAnimSceneSection* Section = SceneAsset->GetSection(SectionIdx);

		int32 TotalWeight = 0;
		for (int32 Idx = 0; Idx < NumSets; Idx++)
		{
			TotalWeight += FMath::Max(Section->GetAnimSet(Idx)->RandomWeight, 0);
		}

		int32 RandomValue = FMath::RandRange(0, TotalWeight);
		for (int32 Idx = 0; Idx < NumSets; Idx++)
		{
			RandomValue -= FMath::Max(Section->GetAnimSet(Idx)->RandomWeight, 0);
			if (RandomValue <= 0)
			{
				AnimSetIdx = Idx;
				break;
			}
		}
	}

	const FName PrimaryRole = SceneAsset->GetPrimaryRole();
	FContextualAnimSceneBindings Bindings = FContextualAnimSceneBindings(*SceneAsset, SectionIdx, AnimSetIdx);

	// Add primary actor to the contextual anim scene bindings
	if (Bindings.BindActorToRole(*PrimaryActor, PrimaryRole) == false)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Failed to bind PrimaryActor"), __FUNCTION__);
		return false;
	}

	// Add secondary actor to the contextual anim scene bindings
	if (Bindings.BindActorToRole(*SecondaryActor, SecondaryRole) == false)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Failed to bind SecondaryActor"), __FUNCTION__);
		return false;
	}

	// If another actor is defined and a valid role is set, add it to the contextual anim scene bindings too
	// @TODO: This will be replaced by a loop over a dynamic array once we have that
	if (TertiaryActor && TertiaryRole != NAME_None)
	{
		if (Bindings.BindActorToRole(*TertiaryActor, TertiaryRole) == false)
		{
			UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Invalid TertiaryActor (%s) or Role (%s)"),
				__FUNCTION__, *GetNameSafe(TertiaryActor), *TertiaryRole.ToString());
			return false;
		}
	}

	// Ensure that Bindings are valid after adding the actors
	if (!Bindings.IsValid())
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Invalid Bindings."), __FUNCTION__);
		return false;
	}

	// Bump up unique id on each loop to ensure replication when all the relevant data in the bindings is the same
	// @TODO: Temp here until we move it to the comp
	Bindings.GenerateUniqueId();

	UContextualAnimSceneActorComponent* SceneActorComp = SecondaryActor->FindComponentByClass<UContextualAnimSceneActorComponent>();
	check(SceneActorComp);

	SceneActorComp->StartContextualAnimScene(Bindings, WarpTargets);
   SceneActorComp->OnLeftSceneDelegate.AddDynamic(this, &ThisClass::OnLeftScene);

	return true;
}

bool UTATStateTreeTask_PlayContextualAnim_InstanceData::JoinContextualAnim(const FStateTreeExecutionContext& context) const
{
	UContextualAnimSceneActorComponent* SceneActorComp = PrimaryActor->FindComponentByClass<UContextualAnimSceneActorComponent>();
	if (SceneActorComp == nullptr)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Missing SceneActorComp on PrimaryActor"), __FUNCTION__);
		return false;
	}

	const bool bResult = SceneActorComp->LateJoinContextualAnimScene(SecondaryActor, SecondaryRole, WarpTargets);
	UE_CVLOG_UELOG(!bResult, context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: LateJoinContextualAnimScene Failed"), __FUNCTION__);

	return bResult;
}

bool UTATStateTreeTask_PlayContextualAnim_InstanceData::TransitionSingleActor(const FStateTreeExecutionContext& context) const
{
	UContextualAnimSceneActorComponent* SceneActorComp = SecondaryActor->FindComponentByClass<UContextualAnimSceneActorComponent>();
	if (SceneActorComp == nullptr)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Missing SceneActorComp on SecondaryActor"), __FUNCTION__);
		return false;
	}

	const UContextualAnimSceneAsset* Asset = SceneActorComp->GetBindings().GetSceneAsset();
	if (Asset == nullptr)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Invalid SceneAsset"), __FUNCTION__);
		return false;
	}

	const int32 SectionIdx = Asset->GetSectionIndex(SectionName);
	if (SectionIdx == INDEX_NONE)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: '%s' is not a valid section name in %s"),
			__FUNCTION__, *SectionName.ToString(), *GetNameSafe(Asset));
		return false;
	}

	// For now we always transition to the first AnimSet in the section. 
	// We may want to change that if we end up having multiple animations (variations) for a non primary section but keeping it simple for now.
	const bool bResult = SceneActorComp->TransitionSingleActor(SectionIdx, 0, WarpTargets);
	UE_CVLOG_UELOG(!bResult, context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed."), __FUNCTION__);

	return bResult;
}

bool UTATStateTreeTask_PlayContextualAnim_InstanceData::TransitionAllActors(const FStateTreeExecutionContext& context) const
{
	UContextualAnimSceneActorComponent* SceneActorComp = SecondaryActor->FindComponentByClass<UContextualAnimSceneActorComponent>();
	if (SceneActorComp == nullptr)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: Missing SceneActorComp on SecondaryActor"), __FUNCTION__);
		return false;
	}

	const bool bResult = SceneActorComp->TransitionContextualAnimScene(SectionName, WarpTargets);
	UE_CVLOG_UELOG(!bResult, context.GetOwner(), LogStateTree, Warning, TEXT("%hs Failed. Reason: TransitionContextualAnimScene Failed"), __FUNCTION__);

	return bResult;
}

void UTATStateTreeTask_PlayContextualAnim_InstanceData::OnExitState()
{
	CleanUp();
}

EStateTreeRunStatus UTATStateTreeTask_PlayContextualAnim_InstanceData::OnTick(const FStateTreeExecutionContext& context, float deltaTime)
{
	if (bMontageInterrupted)
	{
		return EStateTreeRunStatus::Failed;
	}
   
	if (!bLoopForever && CompletedLoops >= LoopsToRun)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// If it's not set we are running an animation, otherwise we need to run one or are waiting on the delay
	EStateTreeRunStatus RunStatus = EStateTreeRunStatus::Running;
	if (TimeBeforeStartingNewLoop.IsSet())
	{
		*TimeBeforeStartingNewLoop -= deltaTime;
		if (*TimeBeforeStartingNewLoop <= 0.f)
		{
			RunStatus = Play(context);
		}
	}

	return RunStatus;
}

EStateTreeRunStatus UTATStateTreeTask_PlayContextualAnim_InstanceData::Play(const FStateTreeExecutionContext& context)
{
	// Need to unset so that no new animation loop will be run by the tick!
	TimeBeforeStartingNewLoop.Reset();

	bool bResult = false;
	switch (ExecutionMethod)
	{
	   case ETATPlayContextualAnimExecutionMethod::StartInteraction:
		   bResult = StartContextualAnim(context);
		   break;
	   case ETATPlayContextualAnimExecutionMethod::JoinInteraction:
		   bResult = JoinContextualAnim(context);
		   break;
	   case ETATPlayContextualAnimExecutionMethod::TransitionAllActors:
		   bResult = TransitionAllActors(context);
		   break;
	   case ETATPlayContextualAnimExecutionMethod::TransitionSingleActor:
		   bResult = TransitionSingleActor(context);
		   break;
	   default: check(false); break;
	}

	return bResult ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

void UTATStateTreeTask_PlayContextualAnim_InstanceData::CleanUp()
{
   CompletedLoops = 0;
   bMontageInterrupted = false;
   bExpectInterruptionFromNotifyEventLoop = false;
   TimeBeforeStartingNewLoop.Reset();

   if(SecondaryActor)
   {
      if(UContextualAnimSceneActorComponent* otherSceneActorComp = SecondaryActor->FindComponentByClass<UContextualAnimSceneActorComponent>())
      {
         if(bShouldExitOutOfContextualAnimOnCleanUp)
         {
            otherSceneActorComp->EarlyOutContextualAnimScene();
         }
         otherSceneActorComp->OnLeftSceneDelegate.RemoveAll(this);
      }
   }
}

void UTATStateTreeTask_PlayContextualAnim_InstanceData::OnLeftScene(UContextualAnimSceneActorComponent* sceneActorComponent)
{
   CompletedLoops++;
   TimeBeforeStartingNewLoop = FMath::FRandRange(FMath::Max(0.0f, DelayBetweenLoops - RandomDeviationBetweenLoops), (DelayBetweenLoops + RandomDeviationBetweenLoops));
   UE_LOG(LogStateTree, Verbose, TEXT("%s - %hs Left Scene - Finished %s"),	*GetNameSafe(this), __FUNCTION__, CompletedLoops >= LoopsToRun ? TEXT("Yes") : TEXT("No"));
}

void UTATStateTreeTask_PlayContextualAnim_InstanceData::OnNotifyBeginReceived(FName notifyName, const FBranchingPointNotifyPayload& branchingPointNotifyPayload)
{
	UE_LOG(LogStateTree, Verbose, TEXT("%hs NotifyName: %s Anim: %s"),	__FUNCTION__, *notifyName.ToString(), *GetNameSafe(branchingPointNotifyPayload.SequenceAsset));

	// Increment the completed counter, if we are waiting for notify event to end and we receive the event with the expected notify name
	if (bWaitForNotifyEventToEnd && NotifyEventNameToEnd == notifyName)
	{
		bExpectInterruptionFromNotifyEventLoop = true;
		CompletedLoops++;
		TimeBeforeStartingNewLoop = FMath::FRandRange(FMath::Max(0.0f, DelayBetweenLoops - RandomDeviationBetweenLoops), (DelayBetweenLoops + RandomDeviationBetweenLoops));
	   UE_LOG(LogStateTree, Verbose, TEXT("%hs Time Set %s"),	__FUNCTION__, TimeBeforeStartingNewLoop.IsSet() ? TEXT("Set") : TEXT("Not Set"));
	}
}


//-----------------------------------------------------
// FStateTreeTask_PlayContextualAnim
//-----------------------------------------------------

FTATStateTreeTask_PlayContextualAnim::FTATStateTreeTask_PlayContextualAnim()
{
   bShouldCopyBoundPropertiesOnTick = false;
   bShouldCopyBoundPropertiesOnExitState = false;
}

EStateTreeRunStatus FTATStateTreeTask_PlayContextualAnim::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
   UInstanceDataType* InstanceData = Context.GetInstanceDataPtr<UInstanceDataType>(*this);
   check(InstanceData);

   return InstanceData->OnEnterState(Context);
}

void FTATStateTreeTask_PlayContextualAnim::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
   UInstanceDataType* InstanceData = Context.GetInstanceDataPtr<UInstanceDataType>(*this);
   check(InstanceData);

   InstanceData->OnExitState();
}

EStateTreeRunStatus FTATStateTreeTask_PlayContextualAnim::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
   UInstanceDataType* InstanceData = Context.GetInstanceDataPtr<UInstanceDataType>(*this);
   check(InstanceData);

   return InstanceData->OnTick(Context, DeltaTime);
}
