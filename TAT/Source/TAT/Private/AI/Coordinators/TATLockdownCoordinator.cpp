// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Coordinators/TATLockdownCoordinator.h"

// ue
#include "StateTreeEvents.h"

// tat
#include "AI/TATAIController.h"
#include "AI/StateTrees/TATStateTreeEvents.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLockdownCoordinator)

void FTATLockdownSearchStateData::Reset()
{
   FinishedSearching = false;
   InteractingCharacter = nullptr;
}

void FTATLockdownState::SendEventToAllCharactersInLockdown(const FStateTreeEvent& event)
{
   for (TWeakObjectPtr<ATATCharacterAIBase> lockdownCharacter : LockdownCharacters)
   {
      if (lockdownCharacter == nullptr)
         continue;
      ATATAIController* aiController = lockdownCharacter->GetController<ATATAIController>();
      if (aiController == nullptr)
         continue;
      aiController->SendStateTreeEvent(event);
   }
}

void FTATLockdownState::Reset()
{
   for (auto locationToSearch : LocationsToSearch)
   {
      locationToSearch.Reset();
   }
   GuardTurningOffAlarm.Reset();
   LockdownCharacters.Reset();
}

void SendEventToCharacter(const ATATCharacterAIBase* character, const FGameplayTag& tagToFire)
{
   const ATATAIController* aiController = character->GetController<ATATAIController>();
   if (aiController == nullptr)
      return;
   FStateTreeEvent event;
   event.Tag = tagToFire;
   aiController->SendStateTreeEvent(event);
}

void UTATLockdownCoordinator::JoinLockdown(ATATCharacterAIBase* character, ATATSquadAlarmStation* alarmStation)
{
   FTATLockdownState* lockdownState = _GetLockdownState(alarmStation);
   if (lockdownState == nullptr)
      return;

   lockdownState->LockdownCharacters.AddUnique(character);
   
}

void UTATLockdownCoordinator::LeaveLockdown(ATATCharacterAIBase* character)
{
   FTATLockdownState* lockdownState = _GetLockdownState(character);
   if (lockdownState == nullptr)
      return;

   FTATLockdownSearchStateData* searchLockdownState = _GetLockdownSearchStateDataForCharacter(character);
    if (searchLockdownState != nullptr)
   {
      searchLockdownState->Reset();  
      FStateTreeEvent event;
      event.Tag = TAG_StateTreeEvent_LockdownGuardLeaving;
      lockdownState->SendEventToAllCharactersInLockdown(event);
   }
   
   lockdownState->LockdownCharacters.Remove(character);

   if (lockdownState->GuardTurningOffAlarm == character)
   {
      lockdownState->GuardTurningOffAlarm = nullptr;
      if (lockdownState->LockdownCharacters.Num() > 0)
      {
         ATATCharacterAIBase* newGuardCharacter = lockdownState->LockdownCharacters[0].Get();
         lockdownState->GuardTurningOffAlarm = character;
         SendEventToCharacter(newGuardCharacter, TAG_StateTreeEvent_LockdownNewAlarmGuard);
      }
   }
}

void UTATLockdownCoordinator::AbandonLockdown(ATATSquadAlarmStation* alarmStation)
{
   FTATLockdownState* lockdownState = _GetLockdownState(alarmStation);
   if (lockdownState == nullptr)
      return;

   FStateTreeEvent event;
   event.Tag = TAG_StateTreeEvent_AbandonLockdown;
   lockdownState->SendEventToAllCharactersInLockdown(event);
   lockdownState->Reset();
}

ATATAIContextualLocation* UTATLockdownCoordinator::GetNextLockdownLocationToSearch(ATATCharacterAIBase* character)
{
   FTATLockdownSearchStateData* search = _GetAvailableLockdownSearchStateData(character);
   if (search == nullptr)
      return nullptr;

   search->InteractingCharacter = character;
   return search->LocationToSearch;
}

void UTATLockdownCoordinator::FinishSearchingLocation(ATATCharacterAIBase* character)
{
   FTATLockdownSearchStateData* search = _GetLockdownSearchStateDataForCharacter(character);
   if (search == nullptr)
      return;
   search->FinishedSearching = true;
   search->InteractingCharacter = nullptr;
}

ATATSquadAlarmStation* UTATLockdownCoordinator::GetAlarmStationForCharacter(ATATCharacterAIBase* character)
{
   FTATLockdownState* lockdownState = _GetLockdownState(character);
   if (lockdownState == nullptr)
      return nullptr;

   return lockdownState->SquadAlarmStation.Get();
}

bool UTATLockdownCoordinator::_ShouldTryTurningOffAlarm(ATATCharacterAIBase* character)
{
   FTATLockdownState* lockdownState = _GetLockdownState(character);
   if (lockdownState == nullptr)
      return false;

   const FTATLockdownSearchStateData* foundLocation = lockdownState->LocationsToSearch.FindByPredicate([](const FTATLockdownSearchStateData& data)
   {
      return data.InteractingCharacter != nullptr || data.FinishedSearching == false;
   });

   if (foundLocation == nullptr)
      return false;
   return true;
}

bool UTATLockdownCoordinator::TrySetCharacterTurningOffAlarm(ATATCharacterAIBase* character)
{
   if (_ShouldTryTurningOffAlarm(character) == false)
      return false;
   
   FTATLockdownState* lockdownState = _GetLockdownState(character);
   if (lockdownState == nullptr)
      return false;

   if (lockdownState->GuardTurningOffAlarm != nullptr)
      return false;
   
   lockdownState->GuardTurningOffAlarm = character;
   return true;
}

FTATLockdownSearchStateData* UTATLockdownCoordinator::_GetAvailableLockdownSearchStateData(ATATCharacterAIBase* character)
{
   FTATLockdownState* lockdownState = _GetLockdownState(character);
   if (lockdownState == nullptr)
      return nullptr;

   return lockdownState->LocationsToSearch.FindByPredicate([](const FTATLockdownSearchStateData& data)
   {
      return data.InteractingCharacter == nullptr && data.FinishedSearching == false; 
   });
}

FTATLockdownSearchStateData* UTATLockdownCoordinator::_GetLockdownSearchStateDataForCharacter(ATATCharacterAIBase* character)
{
   FTATLockdownState* lockdownState = _GetLockdownState(character);
   if (lockdownState == nullptr)
      return nullptr;

   return lockdownState->LocationsToSearch.FindByPredicate([character](const FTATLockdownSearchStateData& data)
   {
      return data.InteractingCharacter == character;
   });
}

FTATLockdownState* UTATLockdownCoordinator::_GetLockdownState(ATATCharacterAIBase* character)
{
   for (FTATLockdownState& lockdownState : _lockdownStates)
   {
      if (lockdownState.LockdownCharacters.Contains(character))
      {
         return &lockdownState;
      }
   }
   return nullptr;
}

FTATLockdownState* UTATLockdownCoordinator::_GetLockdownState(ATATSquadAlarmStation* alarmStation)
{
   for (FTATLockdownState& lockdownState : _lockdownStates)
   {
      if (lockdownState.SquadAlarmStation == alarmStation)
      {
         return &lockdownState;
      }
   }
   
   FTATLockdownState& lockdownState = _lockdownStates.AddDefaulted_GetRef();
   lockdownState.SquadAlarmStation = alarmStation;
   for (const TObjectPtr<ATATAIContextualLocation>& lockdownLocation : alarmStation->GetLockdownLocations())
   {
      FTATLockdownSearchStateData& locationData = lockdownState.LocationsToSearch.AddDefaulted_GetRef();
      locationData.Reset();
      locationData.LocationToSearch = lockdownLocation;
   }
   return &lockdownState;
}
