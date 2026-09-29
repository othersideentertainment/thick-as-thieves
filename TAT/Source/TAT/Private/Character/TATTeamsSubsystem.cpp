// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "Character/TATTeamsSubsystem.h"
#include "Player/TATCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTeamsSubsystem)

void UTATTeamsSubsystem::RegisterCharacterToTeam(ATATCharacter* character, const uint8 team)
{
   if (TArray<TWeakObjectPtr<ATATCharacter>>* charactersInTeam = _teamsMap.Find(team))
   {
      charactersInTeam->AddUnique(character);
      for (TWeakObjectPtr<ATATCharacter> characterInTeam : *charactersInTeam)
      {
         if (characterInTeam == character)
         {
            continue;
         }
         if(characterInTeam.IsValid())
         {
            characterInTeam->HandleTeamMemberJoined(character);
         }
      }
      return;
   }

   TArray<TWeakObjectPtr<ATATCharacter>> newTeam;
   newTeam.Add(character);

   _teamsMap.Add(team, newTeam);
}

void UTATTeamsSubsystem::UnregisterCharacter(ATATCharacter* character)
{
   for (TTuple<uint8, TArray<TWeakObjectPtr<ATATCharacter>>>& tuple : _teamsMap)
   {
      tuple.Value.Remove(character);
   }
}

void UTATTeamsSubsystem::ChangeCharacterTeam(ATATCharacter* character, const uint8 newTeam)
{
   for (TTuple<uint8, TArray<TWeakObjectPtr<ATATCharacter>>>& team : _teamsMap)
   {
      if (team.Value.Contains(character))
      {
         team.Value.RemoveSingle(character);
         break;
      }
   }

   RegisterCharacterToTeam(character, newTeam);
}

TConstArrayView<TWeakObjectPtr<ATATCharacter>> UTATTeamsSubsystem::GetAllMembersOfTeam(const uint8 team) const
{
   if (const TArray<TWeakObjectPtr<ATATCharacter>>* charactersInTeam = _teamsMap.Find(team))
   {
      return *charactersInTeam;
   }

   return TConstArrayView<TWeakObjectPtr<ATATCharacter>>();
}
