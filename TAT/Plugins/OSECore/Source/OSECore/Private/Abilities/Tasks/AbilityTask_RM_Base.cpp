// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_RM_Base.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_RM_Base)


// Constructor
UAbilityTask_RM_Base::UAbilityTask_RM_Base(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
   , _rootMotionCharacter(nullptr)
   , _charMovementComp(nullptr)
   , _rootMotionSourceID(0)
   , _priority(10)
   , _duration(-1.0f)
{
   bTickingTask = true;
   bSimulatedTask = true;
}

// Called right before being marked for destruction due to network replication
void UAbilityTask_RM_Base::PreDestroyFromReplication()
{
   Super::PreDestroyFromReplication();

   EndTask();
}

// Tick function for this task, if bTickingTask == true
void UAbilityTask_RM_Base::TickTask(float deltaTime)
{
   Super::TickTask(deltaTime);

   if (HasTimedOut())
   {
      OnTimedOut();
      EndTask();
      return;
   }

   if (!TickRootMotion(deltaTime, GetRootMotionSource().Get()))
   {
      EndTask();
   }
}

// Called to trigger the actual task once the delegates have been set up
void UAbilityTask_RM_Base::Activate()
{
   const FGameplayAbilityActorInfo* actorInfo = Ability->GetCurrentActorInfo();
   if (!InitAndApplyRootMotion(actorInfo))
   {
      // Failure
      EndTask();
      return;
   }

   // Success
   SetWaitingOnAvatar();
}

// Initializes the task on simulated proxies
void UAbilityTask_RM_Base::InitSimulatedTask(UGameplayTasksComponent& inGameplayTasksComponent)
{
   Super::InitSimulatedTask(inGameplayTasksComponent);

   const FGameplayAbilityActorInfo* ActorInfo = AbilitySystemComponent->AbilityActorInfo.Get();
   if (!InitAndApplyRootMotion(ActorInfo))
   {
      // Failure
      EndTask();
      return;
   }
}

// End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending.
void UAbilityTask_RM_Base::OnDestroy(bool inOwnerFinished)
{
   if (_rootMotionSourceID != 0)
   {
      if (_rootMotionCharacter != nullptr)
      {
         if (!IsSimulating())
            _rootMotionCharacter->ForceNetUpdate();
      }

      if (_charMovementComp != nullptr)
      {
         _charMovementComp->SetMovementMode(MOVE_Falling);
         _charMovementComp->RemoveRootMotionSourceByID(_rootMotionSourceID);
      }

      _rootMotionSourceID = 0;
   }

   Super::OnDestroy(inOwnerFinished);
}

// Derived classes implement this to allocate the root motion source
TSharedPtr<FRootMotionSource> UAbilityTask_RM_Base::CreateRootMotion()
{
   return nullptr;
}

// Returns the root motion source that was created with this task
TSharedPtr<FRootMotionSource> UAbilityTask_RM_Base::GetRootMotionSource() const
{
   if (_rootMotionSourceID == 0)
      return nullptr;

   if (_charMovementComp == nullptr)
      return nullptr;

   return _charMovementComp->GetRootMotionSourceByID(_rootMotionSourceID);
}

// Initializes the root motion source that was created
bool UAbilityTask_RM_Base::InitRootMotion(TSharedPtr<FRootMotionSource> sourcePtr)
{
   if (sourcePtr == nullptr)
      return false;

   FName instName = GetInstanceName();
   sourcePtr->InstanceName = !instName.IsNone() ? instName : FName("UAbilityTask_RM_Base::InitRootMotion");
   sourcePtr->Priority = _priority;
   sourcePtr->Duration = _duration;
   sourcePtr->AccumulateMode = ERootMotionAccumulateMode::Override;
   sourcePtr->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
   sourcePtr->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
   sourcePtr->FinishVelocityParams.ClampVelocity = 0.0f;
   return true;
}

// Returns properties that are replicated for the lifetime of the actor channel
void UAbilityTask_RM_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UAbilityTask_RM_Base, _priority);
   DOREPLIFETIME(UAbilityTask_RM_Base, _duration);
}

// Creates, initializes, and applies the root motion source
bool UAbilityTask_RM_Base::InitAndApplyRootMotion(const FGameplayAbilityActorInfo* actorInfo)
{
   // Actor info is required
   if (actorInfo == nullptr)
      return false;

   // Character is required
   _rootMotionCharacter = Cast<ACharacter>(actorInfo->AvatarActor.Get());
   if (_rootMotionCharacter == nullptr)
      return false;

   // Character movement component is required
   _charMovementComp = Cast<UCharacterMovementComponent>(actorInfo->MovementComponent.Get());
   if (_charMovementComp == nullptr)
      return false;

   // Root motion source is required
   TSharedPtr<FRootMotionSource> motionSource = CreateRootMotion();
   if (motionSource == nullptr)
      return false;

   // Init root motion settings
   if (!InitRootMotion(motionSource))
      return false;

   // Apply it
   _rootMotionSourceID = _charMovementComp->ApplyRootMotionSource(motionSource);
   if (_rootMotionSourceID == 0)
      return false;

   // Success
   return true;
}

// Ticks the root motion source. Return true to continue the task, return false to end the task
bool UAbilityTask_RM_Base::TickRootMotion(float /*DeltaTime*/, FRootMotionSource* /*SourcePtr*/)
{
   return true;
}

// Returns true if the root motion has timed out or is finished
bool UAbilityTask_RM_Base::HasTimedOut() const
{
   auto SourcePtr = GetRootMotionSource();
   if (!SourcePtr.IsValid())
      return true;

   return SourcePtr->Status.HasFlag(ERootMotionSourceStatusFlags::Finished);
}

