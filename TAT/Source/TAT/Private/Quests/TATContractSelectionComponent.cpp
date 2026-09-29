// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATContractSelectionComponent.h"

// tat
#include "SaveGame/TATSaveGame.h"

// ue5
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATContractSelectionComponent)

UTATContractSelectionComponent::UTATContractSelectionComponent()
{
   SetIsReplicatedByDefault(true);
}

void UTATContractSelectionComponent::BeginPlay()
{
   Super::BeginPlay();
}

void UTATContractSelectionComponent::InitFromLocalSave()
{
   if (ensure(_IsLocalPlayer()))
   {
      if (const UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this))
      {
         SetSelectedContract(saveGame->GetLastActiveContract());
      }
   }
}

void UTATContractSelectionComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.Condition = COND_SkipOwner;

   DOREPLIFETIME_WITH_PARAMS_FAST(UTATContractSelectionComponent, _selectedQuest, params);
}

bool UTATContractSelectionComponent::_IsLocalPlayer() const
{
   // NOTE: This is only correct if the controller has already replicated before PlayerState beginPlay for a local player
   //       Need to either verify the controller always replicates before the player state
   //       or just move the call to initialize this into the controller
   return GetOwner()->HasLocalNetOwner();
}

void UTATContractSelectionComponent::SetSelectedContract(FGameplayTag questTag)
{
   if (_selectedQuest == questTag)
   {
      return;
   }

   _ServerSetSelectedContract(questTag);

   if (!GetOwner()->HasAuthority())
   {
      _selectedQuest = questTag;
      _OnRep_SelectedContract();
   }
}

void UTATContractSelectionComponent::_ServerSetSelectedContract_Implementation(FGameplayTag questTag)
{
   _selectedQuest = questTag;
   MARK_PROPERTY_DIRTY_FROM_NAME(UTATContractSelectionComponent, _selectedQuest, this);
   _OnRep_SelectedContract();
}

void UTATContractSelectionComponent::_OnRep_SelectedContract()
{
   OnSelectedContractChanged.Broadcast(_selectedQuest);
}

