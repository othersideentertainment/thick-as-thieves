// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "TATCharacterSaveId.generated.h"

UENUM(BlueprintType, meta = (ScriptName = "TATCharacterType"))
enum class ETATCharacter : uint8
{
   None,
   Character0            UMETA(DisplayName = "Spider"),
   Character1            UMETA(DisplayName = "Chameleon"),
   Character2            UMETA(DisplayName = "Crasher"),
   Character3            UMETA(DisplayName = "Tinker"),
   Character4            UMETA(DisplayName = "Ghost"),
   MAX                   UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FTATCharacterSaveId
{
   GENERATED_BODY()

public:
   // To be replaced with something else once character saves are not 1:1 with characters
   UPROPERTY()
   ETATCharacter Character = ETATCharacter::None;

   bool IsValid() const
   {
      return Character != ETATCharacter::None;
   }

   bool operator==(const FTATCharacterSaveId& other) const
   {
      return Character == other.Character;
   }

   bool operator!=(const FTATCharacterSaveId& other) const
   {
      return Character != other.Character;
   }

   // will not last forever, so more easily searchable
   static FTATCharacterSaveId FromCharacterType(ETATCharacter character)
   {
      return FTATCharacterSaveId{ character };
   }

   FString ToString() const;
};

inline uint32 GetTypeHash(const FTATCharacterSaveId& id)
{
   return GetTypeHash(id.Character);
}

UCLASS()
class TAT_API UTATCharacterSaveIdFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, meta=(DisplayName = "Is Valid (Character Save Id)", CompactNodeTitle = "Is Valid"), Category="Character Save Id")
   static bool CharacterSaveId_IsValid(const FTATCharacterSaveId& id) { return id.IsValid(); }

   UFUNCTION(BlueprintPure, meta=(DisplayName = "Equal (Character Save Id)", CompactNodeTitle = "==", ScriptOperator = "==", Keywords = "== equal"), Category="Character Save Id")
   static bool CharacterSaveId_Equals(const FTATCharacterSaveId& a, const FTATCharacterSaveId& b) { return a == b; }

   UFUNCTION(BlueprintPure, meta=(DisplayName = "Not Equal (Character Save Id)", CompactNodeTitle = "!=", ScriptOperator = "==", Keywords = "!= not equal"), Category="Character Save Id")
   static bool CharacterSaveId_NotEquals(const FTATCharacterSaveId& a, const FTATCharacterSaveId& b) { return a != b; }

   UFUNCTION(BlueprintPure, Category="Character Save Id")
   static FString CharacterSaveIdToDebugString(const FTATCharacterSaveId& id) { return id.ToString(); }

   UFUNCTION(BlueprintPure, Category = "Character Save Id", meta = (DisplayName = "To CharacterSaveId", CompactNodeTitle = "->", Keywords = "cast convert", BlueprintAutocast))
   static FTATCharacterSaveId Conv_TATCharacterToTATCharacterSaveId(const ETATCharacter character) { return FTATCharacterSaveId::FromCharacterType(character); }
};
