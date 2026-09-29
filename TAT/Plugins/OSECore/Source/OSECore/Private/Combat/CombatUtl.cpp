// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/CombatUtl.h"

// ue4
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CombatUtl)

bool FCombatActorInfoArray::ContainsCharacter(AOSECharacterBase* character) const
{
   check(character);
   for(const FCombatActorInfo& characterInfo : CharacterInfos)
   {
      if (character == characterInfo.Character)
         return true;
   }
   return false;
}

bool FCombatActorInfoArray::AddCharacter(AOSECharacterBase* character)
{
   check(character);
   if (!ContainsCharacter(character))
   {
      FCombatActorInfo& actorInfo = CharacterInfos.AddDefaulted_GetRef();
      actorInfo.Character = character;
      MarkItemDirty(actorInfo);
      return true;
   }
   return false;
}

bool FCombatActorInfoArray::RemoveCharacter(AOSECharacterBase* character)
{
   check(character);
   bool needsDirty = false;
   for (int idx = 0; idx < CharacterInfos.Num(); ++idx)
   {
      const FCombatActorInfo& actorInfo = CharacterInfos[idx];
      if (character == actorInfo.Character)
      {
         CharacterInfos.RemoveAt(idx);
         needsDirty = true;
      }
   }

   if (needsDirty)
   {
      MarkArrayDirty();
      return true;
   }

   return false;
}

