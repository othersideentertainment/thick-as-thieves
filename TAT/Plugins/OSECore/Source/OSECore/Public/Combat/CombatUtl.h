// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Net/Serialization/FastArraySerializer.h"

#include "CombatUtl.generated.h"

class AOSECharacterBase;

USTRUCT(BlueprintType)
struct FCombatActorInfo : public FFastArraySerializerItem
{
   GENERATED_USTRUCT_BODY()

   UPROPERTY(BlueprintReadOnly)
   AOSECharacterBase* Character = nullptr;
};

USTRUCT()
struct FCombatActorInfoArray : public FFastArraySerializer
{
   GENERATED_USTRUCT_BODY()

   UPROPERTY()
   TArray<FCombatActorInfo> CharacterInfos;

   bool ContainsCharacter(AOSECharacterBase* character) const;
   bool AddCharacter(AOSECharacterBase* character);
   bool RemoveCharacter(AOSECharacterBase* character);

   bool NetDeltaSerialize(FNetDeltaSerializeInfo& deltaParms)
   {
      return FFastArraySerializer::FastArrayDeltaSerialize<FCombatActorInfo, FCombatActorInfoArray>(CharacterInfos, deltaParms, *this);
   }
};

template<>
struct TStructOpsTypeTraits<FCombatActorInfoArray> : public TStructOpsTypeTraitsBase2<FCombatActorInfoArray>
{
   enum
   {
      WithNetDeltaSerializer = true,
   };
};
