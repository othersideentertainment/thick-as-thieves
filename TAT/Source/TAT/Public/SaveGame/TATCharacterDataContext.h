// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "SaveGame/TATCharacterSaveId.h"

#include "TATCharacterDataContext.generated.h"

class UTATSaveGame;
struct FTATCharacterProgression;
struct FTATPlayerProgression;

USTRUCT(BlueprintType)
struct FTATCharacterDataContext
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Data Context")
   TObjectPtr<UTATSaveGame> SaveGame;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Data Context")
   FTATCharacterSaveId SaveId;

   bool IsValid() const
   {
      return SaveGame != nullptr && SaveId.IsValid();
   }

   bool operator==(const FTATCharacterDataContext& other) const
   {
      return SaveGame == other.SaveGame && SaveId == other.SaveId;
   }

   FORCEINLINE bool operator!=(const FTATCharacterDataContext& other) const
   {
      return !operator==(other);
   }

   const FTATCharacterProgression& GetCharacterDataChecked() const;
   const FTATPlayerProgression& GetPlayerDataChecked() const;

   FString ToString() const;
};

UCLASS()
class TAT_API UTATCharacterDataContextUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   /// Gets the character progression data context (the save game and current character save id) from the player state
   UFUNCTION(BlueprintPure, Category = "Character Data Context")
   static FTATCharacterDataContext GetCharacterDataContextFromPlayerState(APlayerState* playerState);

   /// Gets the character progression data context (the save game and current character save id) from the player controller
   UFUNCTION(BlueprintPure, Category = "Character Data Context")
   static FTATCharacterDataContext GetCharacterDataContextFromController(APlayerController* controller);

   UFUNCTION(BlueprintPure, Category = "Character Data Context", Meta = (DisplayName = "Is Valid (Character Data Context)", CompactNodeTitle = "Is Valid"))
   static bool CharacterDataContext_IsValid(const FTATCharacterDataContext& id) { return id.IsValid(); }

   UFUNCTION(BlueprintPure, Category = "Character Data Context", Meta = (DisplayName = "Equal (Character Data Context)", CompactNodeTitle = "==", ScriptOperator = "==", Keywords = "== equal"))
   static bool CharacterDataContext_Equals(const FTATCharacterDataContext& a, const FTATCharacterDataContext& b) { return a == b; }

   UFUNCTION(BlueprintPure, Category = "Character Data Context", Meta = (DisplayName = "Not Equal (Character Data Context)", CompactNodeTitle = "!=", ScriptOperator = "==", Keywords = "!= not equal"))
   static bool CharacterDataContext_NotEquals(const FTATCharacterDataContext& a, const FTATCharacterDataContext& b) { return a != b; }

   UFUNCTION(BlueprintPure, Category = "Character Data Context")
   static FString CharacterDataContextToDebugString(const FTATCharacterDataContext& id) { return id.ToString(); }
};
