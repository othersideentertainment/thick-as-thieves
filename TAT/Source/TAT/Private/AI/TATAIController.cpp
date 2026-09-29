// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/TATAIController.h"

// tat
#include "AI/Alertness/TATCharacterAIAlertnessComponent.h"
#include "AI/Escalation/TATEscalationComponent.h"
#include "AI/Navigation/TATNavLinkCustomComponent.h"
#include "AI/Navigation/TATPathFollowingComponent.h"
#include "AI/Tasks/AITask_UseSpecificGameplayBehaviorOnSmartObject.h"
#include "AI/TATVoiceKnowledgeTags.h"
#include "AI/TATKnowledgeComponent.h"
#include "Character/TATCharacterAIBase.h"

// ose
#include "AI/Alertness/OSEAlertnessInterface.h"
#include "OSEIndividualKnowledgeComponent.h"

// ue4
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AI/TATIndividualKnowledgeComponent.h"
#include "AI/Utility/UtilityAIBehaviorComponent.h"
#include "AI/Utility/UtilityAIGoalComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Perception/AISenseConfig_Sight.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIController)

DEFINE_LOG_CATEGORY(LogTATAIController);
ATATAIController::ATATAIController(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer.SetDefaultSubobjectClass(TEXT("PathFollowingComponent"), UTATPathFollowingComponent::StaticClass()))
{
   _stimDatabase = CreateDefaultSubobject<UOSEStimDatabase>("TATAIStimDatabase");
   _escalationComponent = CreateDefaultSubobject<UTATEscalationComponent>(TEXT("EscalationComponent"));
   _voiceLineKnowledgeComponent = CreateDefaultSubobject<UOSEVoiceLineKnowledgeComponent>(TEXT("VoiceLineKnowledge"));
   _individualKnowledgeComponent = CreateDefaultSubobject<UTATIndividualKnowledgeComponent>(TEXT("IndividualKnowledge"));
   _stateTreeAIComponent = CreateDefaultSubobject<UTATStateTreeAIComponent>(TEXT("MainStateTree"));
   _offNavLinkAIComponent = CreateDefaultSubobject<UTATStateTreeAIComponent>(TEXT("OffNavLinkBehavior"));
   
   BrainComponent = _stateTreeAIComponent;
}

void ATATAIController::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   TATKnowledgeComponent = FindComponentByClass<UTATKnowledgeComponent>();
   _tatStateTreeTargetingComponent = FindComponentByClass<UTATStateTreeTargetingComponent>();
}

void ATATAIController::BeginPlay()
{
   Super::BeginPlay();
   if(TATKnowledgeComponent)
   {
      TATKnowledgeComponent->OnDetectionStateChanged.AddDynamic(this, &ThisClass::_HandleDetectionStateChanged);
   }
   if(_offNavLinkAIComponent && _shouldEnableOffNavLinkBehavior)
   {
      _offNavLinkAIComponent->OnStateTreeRunStatusChanged.AddUniqueDynamic(this, &ThisClass::_OnNavLinkTreeRunStateChanged);
   }
}

UOSEIndividualAttitudeComponent* ATATAIController::GetAttitudeComponent() const
{
   if (const IOSEIndividualAttitudeInterface* attitudeInterface = Cast<IOSEIndividualAttitudeInterface>(GetPawn()))
      return attitudeInterface->GetAttitudeComponent();
   return nullptr;
}

UOSEIndividualKnowledgeComponent* ATATAIController::GetIndividualKnowledgeComponent() const
{
   return _individualKnowledgeComponent;
}

void ATATAIController::GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const
{
   // GetOwnedGameplayTags actually resets the tag container. So grab that first.
   if(const ATATCharacterBase* characterBase = GetPawn<ATATCharacterBase>())
   {
      characterBase->GetTraits(tagContainer);
   }
   tagContainer.AppendTags(Traits);
}

UOSEVoiceLineKnowledgeComponent* ATATAIController::GetVoiceLineKnowledgeComponent() const
{
   return _voiceLineKnowledgeComponent;
}

#if ENABLE_VISUAL_LOG
void ATATAIController::GrabDebugSnapshot(FVisualLogEntry* snapshot) const
{
   Super::GrabDebugSnapshot(snapshot);

   if(UOSEIndividualAttitudeComponent* attitudeComponent = GetAttitudeComponent())
   {
      attitudeComponent->DescribeSelfToVisLog(snapshot);
   }
}
#endif

bool ATATAIController::HasTrait(const FGameplayTag& traitTag) const
{
   return Traits.HasTag(traitTag);
}

/**
 * Starts the navigation link behavior.
 * When starting the nav link behavior we make sure that we aren't currently in a nav link (this shouldn't happen, as
 * the path following component should exit the previous nav link before starting another). We then activate the nav link behavior
 * ability immediately, then listen for goal / behavior changes so we can exit out of the nav link behavior.
 * We are also passing in the nav link component to the ability so it can fire callbacks directly to the nav link component.
 */
void ATATAIController::StartNavLinkBehavior(const TSubclassOf<UOSEGameplayAbility> navLinkBehavior, UTATNavLinkCustomComponent* tatNavLink)
{
   if(UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn()))
   {
      FGameplayAbilitySpec spec = FGameplayAbilitySpec(navLinkBehavior, 1, INDEX_NONE, tatNavLink);
      _navLinkBehaviorHandle = abilitySystemComponent->GiveAbilityAndActivateOnce(spec);
      if(UUtilityAIGoalComponent* goalComponent = GetUtilityAIGoalComponent())
      {
         goalComponent->OnAIStateEnter.AddUniqueDynamic(this, &ThisClass::OnAIGoalStateEnteredWithNavLinkBehavior);
      }
      if(UUtilityAIBehaviorComponent* behaviorComponent = GetUtilityAIBehaviorComponent())
      {
         behaviorComponent->OnAIStateEnter.AddUniqueDynamic(this, &ThisClass::OnAIBehaviorStateEnteredWithNavLinkBehavior);
      }
   }
}

/**
 * FinishNavLinkBehavior
 *
 * This method is used to finish the navigation link behavior for the AI controller.
 * It cancels any active ability handle related to the navigation link behavior
 * and removes the dynamic event bindings for entering AI goal and behavior states
 * with the navigation link behavior.
 */
void ATATAIController::FinishNavLinkBehavior() const
{
   if(UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn()))
   {
     if(_navLinkBehaviorHandle.IsValid())
     {
        abilitySystemComponent->CancelAbilityHandle(_navLinkBehaviorHandle);
     }
   }
   if(UUtilityAIGoalComponent* goalComponent = GetUtilityAIGoalComponent())
   {
      goalComponent->OnAIStateEnter.RemoveDynamic(this, &ThisClass::OnAIGoalStateEnteredWithNavLinkBehavior);
   }
   if(UUtilityAIBehaviorComponent* behaviorComponent = GetUtilityAIBehaviorComponent())
   {
      behaviorComponent->OnAIStateEnter.RemoveDynamic(this, &ThisClass::OnAIBehaviorStateEnteredWithNavLinkBehavior);
   }
}

bool ATATAIController::GetMoveFromLocationRequested() const
{
   return _moveFromLocationRequested;
}

void ATATAIController::SetMoveFromLocationRequested(const bool locationMoveRequested)
{
   _moveFromLocationRequested = locationMoveRequested;
}

void ATATAIController::UpdateControlRotation(float deltaTime, bool updatePawn)
{
   if(IsLookInputIgnored())
   {
      return;
   }
   Super::UpdateControlRotation(deltaTime, updatePawn);
}

void ATATAIController::SetCurrentActiveSmartObjectBehavior(UAITask_UseSpecificGameplayBehaviorOnSmartObject* newSmartObjectBehaviorTask)
{
   if(newSmartObjectBehaviorTask != nullptr)
   {
      check(_currentlyActiveSmartObjectBehaviorTask == nullptr);
      _currentlyActiveSmartObjectBehaviorTask = newSmartObjectBehaviorTask;
   }
   else
   {
      _currentlyActiveSmartObjectBehaviorTask.Reset();
      if(_tatStateTreeTargetingComponent)
      {
         _tatStateTreeTargetingComponent->ForceBestTargetEventsToRetrigger();
      }
   }
}

void ATATAIController::SendStateTreeEvent(const FStateTreeEvent& event) const
{
   if(_currentlyActiveSmartObjectBehaviorTask.IsValid())
   {
      _currentlyActiveSmartObjectBehaviorTask->SendStateTreeEvent(event);
      return;
   }
   if(_stateTreeAIComponent != nullptr)
   {
      _stateTreeAIComponent->SendStateTreeEvent(event);
   }
}

FPathFollowingRequestResult ATATAIController::MoveTo(const FAIMoveRequest& moveRequest, FNavPathSharedPtr* outPath)
{
   if(_forceMoveToRequestsToBlockPathfinding)
   {
      FAIMoveRequest mutMoveRequest = moveRequest;
      mutMoveRequest.SetUsePathfinding(false).
      SetRequireNavigableEndLocation(false).
      SetProjectGoalLocation(false).
      SetAllowPartialPath(true);
      // this will force direct pathing.
      return Super::MoveTo(mutMoveRequest, outPath);
   }
   if(moveRequest.IsUsingPathfinding() && _shouldEnableOffNavLinkBehavior)
   {
      UNavigationSystemV1* navSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
      const FNavAgentProperties& agentProps = GetNavAgentPropertiesRef();
      FNavLocation projectedLocation;
      const FVector startLocation = GetNavAgentLocation();

      if (navSys && !navSys->ProjectPointToNavigation(startLocation, projectedLocation, INVALID_NAVEXTENT, &agentProps))
      {
         UE_LOG(
            LogTATAIController,
            Warning,
            TEXT("%s is trying to move from an invalid nav mesh point - %s"),
            *GetNameSafe(this),
            *moveRequest.ToString());
         
         _stateTreeAIComponent->PauseLogic(TEXT("Off Nav Link"));
         _offNavLinkAIComponent->StartLogic();
         return FPathFollowingRequestResult();
      }
   }
   return Super::MoveTo(moveRequest, outPath);
}

void ATATAIController::_HandleDetectionStateChanged(const FTATActorKnowledge& actorKnowledge, EActorDetectionState prevDetectionState)
{
   if(_individualKnowledgeComponent == nullptr)
      return;
   if(prevDetectionState == EActorDetectionState::Identified)
   {
      _individualKnowledgeComponent->AddTag(actorKnowledge.GetActor(),TAG_VoiceKnowledge_Character_LostTarget);
      _individualKnowledgeComponent->RemoveTag(actorKnowledge.GetActor(),TAG_VoiceKnowledge_Character_Identified);
   }
   if(actorKnowledge.GetDetectionState() == EActorDetectionState::Identified)
   {
      _individualKnowledgeComponent->AddTag(actorKnowledge.GetActor(),TAG_VoiceKnowledge_Character_Identified);
      // Remove the "Lost Target" tag after we've started chasing them
   }
}

void ATATAIController::_OnNavLinkTreeRunStateChanged(EStateTreeRunStatus stateTreeRunStatus)
{
   if(stateTreeRunStatus == EStateTreeRunStatus::Succeeded)
   {
      _offNavLinkAIComponent->StopLogic(TEXT("Back on nav"));
      _stateTreeAIComponent->ResumeLogic(TEXT("Back on nav"));
   }
}

void ATATAIController::OnAIGoalStateEnteredWithNavLinkBehavior(const FUtilityStateEvaluatorInstance& state,
                                                               const FUtilityStateTarget& target)
{
   // Exit out of nav link behavior if the goal state changed.
   // TODO: We may want to STAY in the nav link behavior for things like climbing ladders. As we would want them to either
   // climb to the top/bottom or drop off the ladder. Even if the goal changes. To do this we may want to have the nav link
   // behaviors override this behavior? (We could stop AI logic updates during the behavior for example, but that may look
   // odd. Especially in the ladder instance) 
   FinishNavLinkBehavior();
}

void ATATAIController::OnAIBehaviorStateEnteredWithNavLinkBehavior(const FUtilityStateEvaluatorInstance& state,
   const FUtilityStateTarget& target)
{
   // Similar to the goal changing, if the target behavior changes we should exit out of the nav link behavior immediately
   // Again, we may want to override this based on the nav link behavior.
   FinishNavLinkBehavior();
}

void ATATAIController::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   if (_stimDatabase)
   {
      const float now = GetWorld()->GetTimeSeconds();
      _stimDatabase->UpdateDatabase(deltaTime, now);
   }
}

void ATATAIController::SetPawn(APawn* inPawn)
{
   Super::SetPawn(inPawn);
}

void ATATAIController::OnPossess(APawn* inPawn)
{
   Super::OnPossess(inPawn);
   if(inPawn)
   {
      _escalationComponent->SetState(ETATEscalationState::Fresh);
   }
}

void ATATAIController::OnUnPossess()
{
   Super::OnUnPossess();
}

void ATATAIController::OnEnterTargetingActorForStateTree(AActor* actor)
{
   _currentCombatTargetForStateTree = actor;
}

void ATATAIController::OnExitTargetingActorForStateTree()
{
   _currentCombatTargetForStateTree.Reset();
}

UOSEStimDatabase* ATATAIController::AuthorityGetStimDatabase() const
{
   // not in a squad, just use our own database
   return _stimDatabase;
}

void ATATAIController::AuthorityOnStimPerceivedByActorsChanged(FStimInfo& stimInfo, AActor* perceivedBy)
{
   // should be us!
   check(perceivedBy == GetPawn());

   // let the alertness component know about this
   if (IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(perceivedBy))
   {
      if (UTATCharacterAIAlertnessComponent* alertnessComp = Cast<UTATCharacterAIAlertnessComponent>(alertnessInterface->GetAlertnessComponent()))
      {
         alertnessComp->OnStimReceived(stimInfo);
      }
   }   
}

void ATATAIController::LockAIResources(bool lockMovement, bool lockLogic)
{
   UE_CLOG(!(lockMovement || lockLogic), LogTATAIController, Warning, TEXT("ATATAIController::LockAIResources called with all params set to false."));

   bool lockChanged = false;

   bool movementLocked = false;
   if (UPathFollowingComponent* pathFollowingComponent = GetPathFollowingComponent())
   {
      const bool previouslyLocked = pathFollowingComponent->IsResourceLocked();
      if (lockMovement)
      {
         pathFollowingComponent->LockResource(EAIRequestPriority::HardScript);
      }
      movementLocked = pathFollowingComponent->IsResourceLocked();

      lockChanged |= (previouslyLocked != movementLocked);
   }

   bool logicLocked = false;
   if (BrainComponent != nullptr)
   {
      const bool previouslyLocked = BrainComponent->IsResourceLocked();
      if (lockLogic)
      {
         BrainComponent->LockResource(EAIRequestPriority::HardScript);
      }
      logicLocked = BrainComponent->IsResourceLocked();

      lockChanged |= (previouslyLocked != movementLocked);
   }

   if (lockChanged)
   {
      OnAIResourceLockChanged.Broadcast(movementLocked, logicLocked);
   }
}

void ATATAIController::UnlockAIResources(bool unlockMovement, bool unlockLogic)
{
   UE_CLOG(!(unlockMovement || unlockLogic), LogTATAIController, Warning, TEXT("ATATAIController::UnlockAIResources called with all params set to false."));

   bool lockChanged = false;

   bool movementLocked = false;
   if (UPathFollowingComponent* pathFollowingComponent = GetPathFollowingComponent())
   {
      const bool previouslyLocked = pathFollowingComponent->IsResourceLocked();
      if (unlockMovement)
      {
         pathFollowingComponent->ClearResourceLock(EAIRequestPriority::HardScript);
      }
      movementLocked = pathFollowingComponent->IsResourceLocked();

      lockChanged |= (previouslyLocked != movementLocked);
   }

   bool logicLocked = false;
   if (BrainComponent != nullptr)
   {
      const bool previouslyLocked = BrainComponent->IsResourceLocked();
      if (unlockLogic)
      {
         BrainComponent->ClearResourceLock(EAIRequestPriority::HardScript);
      }
      logicLocked = BrainComponent->IsResourceLocked();

      lockChanged |= (previouslyLocked != movementLocked);
   }

   if (lockChanged)
   {
      OnAIResourceLockChanged.Broadcast(movementLocked, logicLocked);
   }
}

void ATATAIController::GetAIResourceLockStates(bool& isMovementLocked, bool& isLogicLocked)
{
   if (const UPathFollowingComponent* pathFollowingComponent = GetPathFollowingComponent())
   {
      isMovementLocked = pathFollowingComponent->IsResourceLocked();
   }
   else
   {
      isMovementLocked = false;
   }

   if (BrainComponent != nullptr)
   {
      isLogicLocked = BrainComponent->IsResourceLocked();
   }
   else
   {
      isLogicLocked = false;
   }
}
