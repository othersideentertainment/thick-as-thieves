// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATTalkingDoor.h"

// tat
#include "Interactables/TATInteractHighlightUtils.h"
#include "Variation/Clues/TATKnownCluesComponent.h"
#include "Quests/TATQuestDependentActorHelpers.h"

// ue
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTalkingDoor)


UTATTalkingDoorClueComponent::UTATTalkingDoorClueComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   _clueType = ETATClueType::TalkingDoor;
}

void UTATTalkingDoorClueComponent::SetClue(const FTATTalkingDoorClueData& clue)
{
   CastChecked<ATATTalkingDoor>(GetOwner())->AuthoritySetClue(clue);
}

ATATTalkingDoor::ATATTalkingDoor()
{
   PrimaryActorTick.bCanEverTick = false;
   NetDormancy = DORM_Initial;
   bReplicates = true;
   _clueComponent = CreateDefaultSubobject<UTATTalkingDoorClueComponent>("ClueSpawner");
}

void ATATTalkingDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATTalkingDoor, _clueData);
   DOREPLIFETIME(ATATTalkingDoor, _isShowingClue);
}

void ATATTalkingDoor::AuthoritySetClue(const FTATTalkingDoorClueData& clue)
{
   check(HasAuthority());
   FlushNetDormancy();
   const FTATTalkingDoorClueData previous = _clueData;
   _clueData = clue;
   _OnRep_ClueData(previous);
}

bool ATATTalkingDoor::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return _clueData.IsValid() && !_previousPlayers.Contains(interactingCharacter) && !_isShowingClue
      && TATQuestDependentActorHelpers::IsRelevantForCharacter(_clueData.SourceTag, interactingCharacter);
}

FInteractStartResult ATATTalkingDoor::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   _previousPlayers.AddUnique(interactingCharacter);

   // maybe don't bother predicting this?
   _SetShowingClue(true);

   if(HasAuthority())
   {
      if (UTATKnownCluesComponent* knownClues = UTATKnownCluesComponent::Get(interactingCharacter->GetPlayerState()))
      {
         knownClues->AddClueFacts(_clueData.Facts.Get());
         // Is this the correct source tag to use?
         knownClues->AddSyntheticFact(_clueData.Facts->GetSourceTag(), _additionalClueFactTag);
      }
      GetWorldTimerManager().SetTimer(_expireClueTimer, FTimerDelegate::CreateUObject(this, &ThisClass::_ExpireActiveClue), _clueDisplayDuration, false);
   }
   
   return {};
}

void ATATTalkingDoor::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   outPrompt.PressAction = _getCluePrompt;
}

void ATATTalkingDoor::ShowHighlight_Implementation(bool showHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, showHighlight);
}

TSoftObjectPtr<UAkAudioEvent> ATATTalkingDoor::GetClueAudioEvent() const
{
   return _clueData.AudioEvent;
}

void ATATTalkingDoor::_SetShowingClue(bool newShowingClue)
{
   if(newShowingClue == _isShowingClue) return;
   
   const bool wasShowingClue = _isShowingClue;
   FlushNetDormancy();
   _isShowingClue = newShowingClue;
   _OnRep_ShowingClue(wasShowingClue);
}

void ATATTalkingDoor::_ExpireActiveClue()
{
   _SetShowingClue(false);
}

void ATATTalkingDoor::_SetLocalClueState(ETATTalkingDoorLocalState newState)
{
   if(newState != _localClueState)
   {
      _localClueState = newState;
      BP_OnLocalClueStateChanged(_localClueState);
   }
}

void ATATTalkingDoor::_OnRep_ClueData(const FTATTalkingDoorClueData& previous)
{
   if(!previous.IsValid() && _clueData.IsValid() && !IsNetMode(NM_DedicatedServer))
   {
      // NB: Raw `this` capture is fine only because OnceRelevantToLocalPlayer handles that
      TATQuestDependentActorHelpers::OnceRelevantToLocalPlayer(_clueData.SourceTag, this, [this]() {
         if (_localClueState == ETATTalkingDoorLocalState::NoClue)
         {
            _SetLocalClueState(ETATTalkingDoorLocalState::UnseenClue);
         }
      });
   }
}

void ATATTalkingDoor::_OnRep_ShowingClue(const bool& wasShowing)
{
   if(wasShowing == _isShowingClue) return;

   BP_OnShowingClueChanged(_isShowingClue);

   if(!_isShowingClue && !IsNetMode(NM_DedicatedServer))
   {
      if(_localClueState == ETATTalkingDoorLocalState::UnseenClue && _previousPlayers.FindByPredicate([](TWeakObjectPtr<const ACharacter> c) { return c.IsValid() && c->IsLocallyControlled(); }))
      {
         _SetLocalClueState(ETATTalkingDoorLocalState::SeenClue);
      }
   }
}

