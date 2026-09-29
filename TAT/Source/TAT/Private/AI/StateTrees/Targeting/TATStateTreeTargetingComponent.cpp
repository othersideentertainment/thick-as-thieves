// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Targeting/TATStateTreeTargetingComponent.h"

// tat
#include "AI/Coordinators/TATLockdownCoordinator.h"
#include "AI/Perception/TATHearingTypes.h"
#include "AI/StateTrees/TATStateTreeEvents.h"
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerState.h"

// ose
#include "AI/Utility/ResponseCurve.h"

// ue
#include "Abilities/TATGameplayTags.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTargetingComponent)

DEFINE_LOG_CATEGORY(LogTATStateTreeTargeting);


void FTATStateTreeTargetingCalculationGroup::CalculateScoreForTargetActor(AOSEAIController* controller,
                                                                          AOSECharacterBase* character,
                                                                          const FTATStateTreeTargetingConsiderationTargetContext& targetContext)
{
   float totalScore = 1.f;   
   check(TargetingConsiderations);
   if(TargetingConsiderations == nullptr)
      return;
   int numConsiderations = 0;

   if (!ensureAlwaysMsgf(targetContext.IsValidForTargetType(TargetingConsiderations->TargetType), 
      TEXT("CalculateScoreForTargetActor called with an invalid 'targetContext'!")))
   {
      return;
   }

   const bool isEvaluatingCurrentTarget = (targetContext.Actor == BestTargetActor);

   // We want to count the number of considerations, except for the initial only considerations IF we are evaluating the current target
   for (const FTATStateTreeTargetingConsideration& consideration : TargetingConsiderations->Considerations)
   {
      if(!consideration.Enabled || (isEvaluatingCurrentTarget &&
         consideration.EvaluationCondition == ETATStateTreeTargetingConsiderationEvaluationCondition::IgnoreIfCurrentTarget))
      {
         continue;
      }
      numConsiderations++;
   }
   
   // See: https://www.gdcvault.com/play/1021848/Building-a-Better-Centaur-AI around 9:10
   const float modificationFactor = 1.0f - (1.0f / numConsiderations);
   for (const FTATStateTreeTargetingConsideration& consideration : TargetingConsiderations->Considerations)
   {
      const UTATStateTreeTargetingConsiderationInput* considerationInput = consideration.Input;
      if(considerationInput == nullptr)
      {
         return;
      }
      if (!consideration.Enabled || (isEvaluatingCurrentTarget &&
         consideration.EvaluationCondition == ETATStateTreeTargetingConsiderationEvaluationCondition::IgnoreIfCurrentTarget))
      {
         continue;
      }
      const float score = considerationInput->GetValue(controller, character, targetContext);
      float responseScore = [&score, &consideration]
      {
         if(consideration.ResponseCurve == nullptr)
         {
            return score;
         }
         return consideration.ResponseCurve->GetFloatValue(score);  
      }();
      
      // If the consideration score is below the minimum, force to zero. 
      if(responseScore < 0.001f)
      {
         responseScore = 0.f;
      }
      
      if(FMath::IsNearlyZero(responseScore))
      {
         // Short Circuit, this target is scoring zero, so it's not valid.
         if(BestTargetActor == targetContext.Actor)
         {
            ResetBestScore(false);
         }
         return;
      }
      // Apply compensation factor to this consideration's score.
      const float makeUpValue = (1.0f - responseScore) * modificationFactor;
      const float finalConsiderationScore = responseScore + (makeUpValue * responseScore);
      totalScore *= finalConsiderationScore;
   }
   if(BestTargetActor != nullptr && BestTargetActor == targetContext.Actor)
   {
      totalScore += TargetingConsiderations->TargetMomentumAdditionalScore;
   }
   if(totalScore <= BestScore)
   {
      return;
   }
   _SetBestTarget(totalScore, targetContext.Actor, targetContext.WorldTime);
}

void FTATStateTreeTargetingCalculationGroup::ResetBestScore(const bool forceEvent)
{
   _BestTargetChanged = forceEvent || BestTargetActor.IsValid();
   BestScore = -1.f;
   BestTargetActor = nullptr;
}

void FTATStateTreeTargetingCalculationGroup::_SetBestTarget(const float score, AActor* target, double worldTime)
{
   if(FMath::IsNearlyZero(score))
      return;
   if(BestTargetActor == target || target == nullptr)
      return;
   BestScore = score;
   BestTargetActor = target;
   TimeSet = worldTime;
   SetBestTargetChanged();
}

UTATStateTreeTargetingComponent::UTATStateTreeTargetingComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
}

void UTATStateTreeTargetingComponent::AddTargetingConsiderations(UTATStateTreeTargetingConsiderations* asset)
{
   if (!ensureAlwaysMsgf(asset != nullptr, TEXT("[%s] Null UTATStateTreeTargetingConsiderations passed to AddTargetingConsiderations."), *GetName()))
   {
      return;
   }

   if (!ensureAlwaysMsgf(_GetTargetingGroupIndex(asset) == INDEX_NONE, TEXT("[%s] Already tracking %s."), *GetName(), *asset->GetName()))
   {
      return;
   }

   _TargetingCalculationGroups.Emplace(FTATStateTreeTargetingCalculationGroup(asset));
}

void UTATStateTreeTargetingComponent::RemoveTargetingConsiderations(UTATStateTreeTargetingConsiderations* asset)
{
   if (!ensureAlwaysMsgf(asset != nullptr, TEXT("[%s] Null UTATStateTreeTargetingConsiderations passed to RemoveTargetingConsiderations."), *GetName()))
   {
      return;
   }

   const int32 index = _GetTargetingGroupIndex(asset);
   if (_TargetingCalculationGroups.IsValidIndex(index))
   {
      _TargetingCalculationGroups.RemoveAtSwap(index, EAllowShrinking::No);
   }
}

bool UTATStateTreeTargetingComponent::GetStimDataForTag(FName tag, FTATHearingEventStimSettings& outStimSettings) const
{
   if (const FTATHearingEventStimSettings* stimSettings = UTATAISettings::GetHearingStimSettings()->FindRow<FTATHearingEventStimSettings>(
                                                                tag,
                                                                TEXT("TATStateTreeTargetingComponent")
                                                             ))
   {
      outStimSettings = *stimSettings;
      return true;
   }
   return false;
}

const FStimInfo& UTATStateTreeTargetingComponent::DetermineHighestPriorityStim(const FStimInfo& stimA, const FStimInfo& stimB)
{
   // If the severities are equal, prioritize the more recent.
   if (stimA.Severity == stimB.Severity)
   {
      return stimA.Timestamp >= stimB.Timestamp ? stimA : stimB;
   }

   // Prioritize the more severe.
   return stimA.Severity > stimB.Severity ? stimA : stimB;
}

void UTATStateTreeTargetingComponent::ForceBestTargetEventsToRetrigger()
{
   for (FTATStateTreeTargetingCalculationGroup& targetingCalculationGroup : _TargetingCalculationGroups)
   {
      if(targetingCalculationGroup.BestTargetActor.IsValid())
      {
         targetingCalculationGroup.SetBestTargetChanged();
      }
   }
}

void UTATStateTreeTargetingComponent::_DispatchStimEventToStateTree(const FStimInfo& stimInfo)
{
   _CurrentlyProcessedStims.Add(stimInfo);
   
   FStateTreeEvent event;
   event.Tag = TAG_StateTreeEvent_StimChange;
   event.Payload = FInstancedStruct::Make(
      FTATTargetingEvent_Stim(
         {
            stimInfo
            }
         )
      );
   
   _TATAIController->SendStateTreeEvent(event);
}

void UTATStateTreeTargetingComponent::OnStimAdded(FStimInfo& stimInfo)
{
   // TODO: Remove hardcoded "Tag" for the alarm, this should be data driven.
   ATATCharacterAIBase* aiControlledPawn = _TATAIController->GetPawn<ATATCharacterAIBase>();
   check(aiControlledPawn);
   if(stimInfo.Tag == "SecurityDeviceTriggeredAlarm")
   {
      const IGameplayTagAssetInterface* gameplayTagAssetInterfaceInstigator = Cast<IGameplayTagAssetInterface>(stimInfo.Instigator.Get());
      if(aiControlledPawn == nullptr || gameplayTagAssetInterfaceInstigator == nullptr)
      {
         return;
      }

      const UTATPrivateSpaceCharacterComponent* aiControlledPawnActorComponent = aiControlledPawn->GetPrivateSpaceCharacterComponent();
      if(aiControlledPawnActorComponent == nullptr)
      {
         return;
      }
      const FGameplayTagContainer aiControlledPawnAllowedTags = aiControlledPawnActorComponent->AuthorityGetAllAllowedPrivateZone();
      if(gameplayTagAssetInterfaceInstigator->HasAnyMatchingGameplayTags(aiControlledPawnAllowedTags))
      {
         _DispatchStimEventToStateTree(stimInfo);
         /*
         if(aiControlledPawn->CanJoinLockdown())
         {
            ATATSquadAlarmStation* alarmStation = Cast<ATATSquadAlarmStation>(stimInfo.Instigator.Get());
            if (alarmStation == nullptr)
            {
               return;
            }
            if(UBlackboardComponent* blackboardComponent = _TATAIController->GetBlackboardComponent())
            {
               blackboardComponent->SetValueAsObject("AlarmInstigator", stimInfo.Instigator.Get());
            }

            UTATLockdownCoordinator* lockdownCoordinator = GetWorld()->GetSubsystem<UTATLockdownCoordinator>();
            if(lockdownCoordinator != nullptr)
            {
               lockdownCoordinator->JoinLockdown(aiControlledPawn, alarmStation);
            }
            FStateTreeEvent event;
            event.Tag = TAG_StateTreeEvent_LockdownEvent;
            _TATAIController->SendStateTreeEvent(event);
         }
         else
         {
            FStateTreeEvent event;
            event.Tag = TAG_StateTreeEvent_StimChange;
            event.Payload = FInstancedStruct::Make(FTATTargetingEvent_Stim({stimInfo}));
            _TATAIController->SendStateTreeEvent(event);
         }
         */
      }
   }
   else 
   {
      if (const UOSEIndividualKnowledgeComponent* individualKnowledgeComponent = _TATAIController->
         GetIndividualKnowledgeComponent())
      {
         if (const FIndividualKnowledge* knowledge = individualKnowledgeComponent->FindKnowledgeForActor(
            stimInfo.Instigator.Get()))
         {
            if (knowledge->GameplayTagContainer.HasTag(TAG_IndividualKnowledge_InteractingWithActor))
            {
               // If the knowledge we have of this actor include this tag, we should ignore stims from the source to 
               // avoid having a loop:
               // NPC Interacting > Stim from source > NPC Investigate > NPC See Incorrect Object > NPC Interact > ETC.
               // Instead, we add knowledge during the "interaction" that we are interacting with the asset, that will 
               // cause us to ignore future stims _until_ that knowledge expires. 
               return;
            }
         }
      }
      _DispatchStimEventToStateTree(stimInfo);
   }
}

void UTATStateTreeTargetingComponent::OnActorKnowledgeRemoved(const FTATActorKnowledge& tatActorKnowledge)
{
   for (FTATStateTreeTargetingCalculationGroup& currentCalculationGroup : _TargetingCalculationGroups)
   {
      if(currentCalculationGroup.BestTargetActor == tatActorKnowledge.GetActor())
      {
         currentCalculationGroup.ResetBestScore(true);
      }
   }
}

void UTATStateTreeTargetingComponent::BeginPlay()
{
   Super::BeginPlay();
   _TATAIController = CastChecked<ATATAIController>(GetOwner());
   check(_TATAIController);

   _KnowledgeComponent = _TATAIController->GetTATKnowledgeComponent();
   check(_KnowledgeComponent);

   _StimDatabase = _TATAIController->AuthorityGetStimDatabase();
   check(_StimDatabase);

   _KnowledgeComponent->OnActorKnowledgeAboutToBeRemoved.AddUObject(this, &ThisClass::OnActorKnowledgeRemoved);
   _TATAIController->OnStimAdded.AddUObject(this, &ThisClass::OnStimAdded);
}

void UTATStateTreeTargetingComponent::TickComponent(const float deltaTime, const ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   // We only calculate ONE targeting group per frame, this is because the state tree can't easily process multiple events of the same type per state
   // tree tick. This may be something we can solve in the future, but for now i'm keeping things simple and only having a few calculation groups anyway.
   if(_TargetingCalculationGroups.Num() == 0)
   {
      return;
   }
   
   _CurrentTargetingCalculationGroupIndex++;
   if(_TargetingCalculationGroups.IsValidIndex(_CurrentTargetingCalculationGroupIndex) == false)
   {
      _CurrentTargetingCalculationGroupIndex = 0;
   }
   
   FTATStateTreeTargetingCalculationGroup& currentCalculationGroup = _TargetingCalculationGroups[_CurrentTargetingCalculationGroupIndex];
   if(currentCalculationGroup.BestTargetActor.IsStale())
   {
      currentCalculationGroup.ResetBestScore(true);
   }
   FTATStateTreeTargetingConsiderationTargetContext targetContext;
   targetContext.WorldTime = GetWorld()->GetTimeSeconds();
   switch (currentCalculationGroup.TargetingConsiderations->TargetType)
   {
      case ETATStateTreeTargetingConsiderationTargetType::KnownActors:
      {
         for (FTATActorKnowledge& knowledge : _KnowledgeComponent->GetMutableKnownActors())
         {
            if(knowledge.GetActor() == nullptr)
               continue;
            targetContext.Actor = knowledge.GetActor();
            targetContext.SetKnowledge(knowledge);

            // TODO : can we add flags so a targeting group knows it has a test that requires
            // pathfinding checks, and only perform this operation based on that?
            knowledge.UpdatePathToLastKnownLocation(0.1f);
            currentCalculationGroup.CalculateScoreForTargetActor(_TATAIController, _TATAIController->GetOSECharacter(), targetContext);
         }
         break;
      }
      case ETATStateTreeTargetingConsiderationTargetType::Players:
      {
         const ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>();
         check(gameState != nullptr);

         for (ATATPlayerState* playerState : gameState->GetTATPlayerStates())
         {
            APawn* playerPawn = playerState->GetPawn();
            if (playerPawn == nullptr)
            {
               continue;
            }

            targetContext.Actor = playerPawn;
            targetContext.ClearKnowledge();

            currentCalculationGroup.CalculateScoreForTargetActor(_TATAIController, _TATAIController->GetOSECharacter(), targetContext);
         }
         break;
      }
      default: checkNoEntry(); return;
   }
   
   if(currentCalculationGroup.HasBestTargetChanged())
   {
      FStateTreeEvent event;
      event.Tag = TAG_StateTreeEvent_TargetChange;
      event.Payload = FInstancedStruct::Make(FTATAITargetingEvent_TargetChanged({
         currentCalculationGroup.BestTargetActor.Get(),
         currentCalculationGroup.TargetingConsiderations->TargetingTag
      }));
      _TATAIController->SendStateTreeEvent(event);
      currentCalculationGroup.ConsumeBestTarget();
      UE_VLOG(GetOwner(),
         LogTATStateTreeTargeting,
         Log,
         TEXT("Setting target for %s to %s"),
         *currentCalculationGroup.TargetingConsiderations->TargetingTag.ToString(),
         *GetNameSafe(currentCalculationGroup.BestTargetActor.Get())
         );
   }
}

AActor* UTATStateTreeTargetingComponent::GetBestTargetForTargetingGroup(const FGameplayTag& targetingGroup) const
{
   for (const FTATStateTreeTargetingCalculationGroup& element : _TargetingCalculationGroups)
   {
      if(element.TargetingConsiderations->TargetingTag == targetingGroup)
      {
         return element.BestTargetActor.Get();
      }
   }
   return nullptr;
}

float UTATStateTreeTargetingComponent::GetTimeTargetSetForTargetingGroup(const FGameplayTag& targetingGroup) const
{
   for (const FTATStateTreeTargetingCalculationGroup& element : _TargetingCalculationGroups)
   {
      if(element.TargetingConsiderations->TargetingTag == targetingGroup)
      {
         return element.TimeSet;
      }
   }
   return 0.f;
}

const TArray<FStimInfo>& UTATStateTreeTargetingComponent::GetCurrentlyProcessedStimInfo() const
{
   return _CurrentlyProcessedStims;
}

void UTATStateTreeTargetingComponent::ConsumeCurrentProcessedStims()
{
   _CurrentlyProcessedStims.Empty();
}

int32 UTATStateTreeTargetingComponent::_GetTargetingGroupIndex(UTATStateTreeTargetingConsiderations* asset) const
{
   return _TargetingCalculationGroups.IndexOfByPredicate([asset](const FTATStateTreeTargetingCalculationGroup& group)
   {
      return (group.TargetingConsiderations == asset);
   });
}
