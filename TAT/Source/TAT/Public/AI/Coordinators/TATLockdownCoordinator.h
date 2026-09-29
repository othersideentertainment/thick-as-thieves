// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "StateTreeEvents.h"
#include "Subsystems/WorldSubsystem.h"

// tat
#include "AI/Squad/TATSquadAlarmStation.h"
#include "Character/TATCharacterAIBase.h"

#include "TATLockdownCoordinator.generated.h"

USTRUCT()
struct FTATLockdownSearchStateData
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<ATATAIContextualLocation> LocationToSearch;

   UPROPERTY(Transient)
   TWeakObjectPtr<ATATCharacterAIBase> InteractingCharacter;

   UPROPERTY(Transient)
   bool FinishedSearching { false };
   
   void Reset();
};

USTRUCT()
struct FTATLockdownState
{
   GENERATED_BODY()

   UPROPERTY(Transient)
   TWeakObjectPtr<ATATSquadAlarmStation> SquadAlarmStation;
   
   UPROPERTY(Transient)
   TArray<TWeakObjectPtr<ATATCharacterAIBase>> LockdownCharacters;

   UPROPERTY(Transient)
   TArray<FTATLockdownSearchStateData> LocationsToSearch;

   UPROPERTY(Transient)
   TWeakObjectPtr<ATATCharacterAIBase> GuardTurningOffAlarm;

   void SendEventToAllCharactersInLockdown(const FStateTreeEvent& event);
   void Reset();
};

/**
 * Handles the coordination between multiple characters reacting to the same alarm lockdown state.
 */
UCLASS()
class TAT_API UTATLockdownCoordinator : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   void JoinLockdown(ATATCharacterAIBase* character, ATATSquadAlarmStation* alarmStation);
   void LeaveLockdown(ATATCharacterAIBase* character);
   void AbandonLockdown(ATATSquadAlarmStation* alarmStation);

   ATATAIContextualLocation* GetNextLockdownLocationToSearch(ATATCharacterAIBase* character);
   void FinishSearchingLocation(ATATCharacterAIBase* character);
      
   ATATSquadAlarmStation* GetAlarmStationForCharacter(ATATCharacterAIBase* character);

   bool TrySetCharacterTurningOffAlarm(ATATCharacterAIBase* character);   
private:
   bool _ShouldTryTurningOffAlarm(ATATCharacterAIBase* character);

   FTATLockdownSearchStateData* _GetAvailableLockdownSearchStateData(ATATCharacterAIBase* character);
   FTATLockdownSearchStateData* _GetLockdownSearchStateDataForCharacter(ATATCharacterAIBase* character);
   
   FTATLockdownState* _GetLockdownState(ATATSquadAlarmStation* alarmStation);
   FTATLockdownState* _GetLockdownState(ATATCharacterAIBase* character);
   
   UPROPERTY()
   TArray<FTATLockdownState> _lockdownStates;
};
