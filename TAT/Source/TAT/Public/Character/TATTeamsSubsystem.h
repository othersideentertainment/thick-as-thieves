// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TATTeamsSubsystem.generated.h"

class ATATCharacter;
/**
 * 
 */
UCLASS()
class TAT_API UTATTeamsSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()


public:
   void RegisterCharacterToTeam(ATATCharacter* character, const uint8 team);
   void UnregisterCharacter(ATATCharacter* character);
   void ChangeCharacterTeam(ATATCharacter* character, const uint8 newTeam);

   TConstArrayView<TWeakObjectPtr<ATATCharacter>> GetAllMembersOfTeam(const uint8 team) const;

private:
   TMap<uint8, TArray<TWeakObjectPtr<ATATCharacter>>> _teamsMap;
};
