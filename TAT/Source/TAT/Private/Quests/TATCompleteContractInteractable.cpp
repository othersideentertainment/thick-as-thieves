// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATCompleteContractInteractable.h"

// tat
#include "Analytics/TATAnalyticsManager.h"
#include "Interactables/TATInteractHighlightUtils.h"
#include "Quests/TATContractProgression.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestInfo.h"
#include "Quests/TATThievesDenQuestSubsystem.h"
#include "SaveGame/TATSaveGame.h"
#include "UI/Quests/TATQuestFlowUtils.h"
#include "UI/Queue/TATUIQueue.h"

// ue
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCompleteContractInteractable)


ATATCompleteContractInteractable::ATATCompleteContractInteractable()
{
   PrimaryActorTick.bCanEverTick = false;
   _flow = ETATContractOutroFlow::Interactable;
}

void ATATCompleteContractInteractable::BeginPlay()
{
   Super::BeginPlay();

   if (UTATThievesDenQuestSubsystem* questSubsystem = GetWorld()->GetSubsystem<UTATThievesDenQuestSubsystem>())
   {
      questSubsystem->OnCompletableContractsChanged.AddUObject(this, &ATATCompleteContractInteractable::_RefreshContract);
      _RefreshContract();
   }
}

void ATATCompleteContractInteractable::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UTATThievesDenQuestSubsystem* questSubsystem = GetWorld()->GetSubsystem<UTATThievesDenQuestSubsystem>())
   {
      questSubsystem->OnCompletableContractsChanged.RemoveAll(this);
   }

   Super::EndPlay(endPlayReason);
}

void ATATCompleteContractInteractable::_RefreshContract()
{
   UTATThievesDenQuestSubsystem* questSubsystem = GetWorld()->GetSubsystem<UTATThievesDenQuestSubsystem>();
   check(questSubsystem);
   
   FGameplayTag newQuest = questSubsystem->GetCompletableContractForFlow(_flow);
   if (newQuest != _currentContract)
   {
      const FGameplayTag oldQuest = _currentContract;
      _currentContract = newQuest;
      BP_OnCurrentContractChanged(oldQuest, newQuest);
   }
}

void ATATCompleteContractInteractable::_CompleteContract()
{
   const UTATQuestDataSubsystem& questDataSubsystem = UTATQuestDataSubsystem::Get(this);
   const FGameplayTag questTag = _currentContract;
   const FTATContractInfo* questInfo = questDataSubsystem.FindContractInfo(questTag);
   if (questInfo == nullptr)
   {
      return;
   }
      
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
   const FTATCharacterSaveId saveId = saveGame->GetSelectedCharacter();
   TATContractProgression::CompleteContract(saveGame, saveId, *questInfo);

#if !UE_SERVER
   UGameInstance* gameInstance = GetGameInstance();
   check(gameInstance);
   gameInstance->GetSubsystem<UTATAnalyticsManager>()->OnContractCompleted();
#endif

   BP_OnContractCompleted();
}

bool ATATCompleteContractInteractable::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return _currentContract.IsValid();
}

void ATATCompleteContractInteractable::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   outPrompt.PressAction = _interactPrompt;
}

FInteractStartResult ATATCompleteContractInteractable::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   const UTATQuestDataSubsystem& questDataSubsystem = UTATQuestDataSubsystem::Get(this);
   const FGameplayTag questTag = _currentContract;
   const FTATContractInfo* questInfo = questDataSubsystem.FindContractInfo(questTag);
   if (questInfo == nullptr)
   {
      return FInteractStartResult();
   }

   BP_OnContractOutroStarted(questTag);
   
   UTATUIQueue* queue = UTATUIQueue::Create(interactingCharacter->GetController<APlayerController>());
   if(_completionTiming == ETATContractInteractableCompletionTiming::Immediate)
   {
      queue->AddInstantAction(FSimpleDelegate::CreateUObject(this, &ATATCompleteContractInteractable::_CompleteContract));
   }
   UTATQuestFlowUtils::AddContractPreCompleteFlow(queue, *questInfo);
   if(_completionTiming == ETATContractInteractableCompletionTiming::Middle)
   {
      queue->AddInstantAction(FSimpleDelegate::CreateUObject(this, &ATATCompleteContractInteractable::_CompleteContract));
   }
   UTATQuestFlowUtils::AddContractPostCompleteFlow(queue, *questInfo);
   if(_completionTiming == ETATContractInteractableCompletionTiming::End)
   {
      queue->AddInstantAction(FSimpleDelegate::CreateUObject(this, &ATATCompleteContractInteractable::_CompleteContract));
   }
   queue->Run();

   return FInteractStartResult();
}

void ATATCompleteContractInteractable::ShowHighlight_Implementation(bool showHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, showHighlight);
}
