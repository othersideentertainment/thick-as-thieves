// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTagContainer.h"

#include "TATCharacterLoadout.generated.h"

/// What kind of loadout is this (eg. tools or abilities)
UENUM(BlueprintType)
enum class ETATLoadoutType : uint8
{
   Generic = 0,
   Tool,
   Ability,
   Outfit,
   MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ETATCharacterOutfitSlot :uint8
{
   None,
   Head,
   Body,
   CallingCard,
   MAX UMETA(Hidden)
};

/// Player-centric input binding that can be assigned to a tool.
/// Assumes that there is a gamepad input dedicated to each of these that the player can choose for a particular tool.
UENUM(BlueprintType)
enum class ETATCharacterInputActionType : uint8
{
   None,
   Primary,
   Secondary,
   Utility,
   MAX UMETA(Hidden)
};

/// One loadout entry in a FTATCharacterLoadout. Can be used for gear, abilities, etc.
USTRUCT(BlueprintType)
struct TAT_API FTATCharacterLoadoutEntry
{
   GENERATED_BODY()

   /// The type of tool (eg. "Tool.Zipwire") or ability (eg. "Ability.Type.Dash") for this slot
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Loadout Entry")
   FGameplayTag LoadoutTag;

   /// If not set to None, the input to use
   /// Note: Only implemented for ability loadouts, not gear loadouts (as of 3/17/2025)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Loadout Entry")
   ETATCharacterInputActionType InputAction = ETATCharacterInputActionType::None;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Loadout Entry")
   ETATCharacterOutfitSlot OutfitSlot = ETATCharacterOutfitSlot::None;

   FORCEINLINE bool operator==(const FTATCharacterLoadoutEntry& rhs) const { return LoadoutTag == rhs.LoadoutTag && InputAction == rhs.InputAction; }
   FORCEINLINE bool operator!=(const FTATCharacterLoadoutEntry& rhs) const { return !operator==(rhs); }
};

template<>
struct TStructOpsTypeTraits<FTATCharacterLoadoutEntry> : public TStructOpsTypeTraitsBase2<FTATCharacterLoadoutEntry>
{
   enum
   {
      WithIdenticalViaEquality = true,
   };
};

/// Somewhat generic player-customizable loadout for a character. Can be used for gear, abilities, etc.
USTRUCT(BlueprintType)
struct TAT_API FTATCharacterLoadout
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Loadout")
   ETATLoadoutType Type = ETATLoadoutType::Generic;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Loadout")
   TArray<FTATCharacterLoadoutEntry> Entries;

   FORCEINLINE bool operator==(const FTATCharacterLoadout& rhs) const { return Type == rhs.Type && Entries == rhs.Entries; }
   FORCEINLINE bool operator!=(const FTATCharacterLoadout& rhs) const { return !operator==(rhs); }

   FORCEINLINE int32 NumEntries() const { return Entries.Num(); }

   /// Checks if the loadout has at least one valid tag
   bool IsEmpty() const;
};

template<>
struct TStructOpsTypeTraits<FTATCharacterLoadout> : public TStructOpsTypeTraitsBase2<FTATCharacterLoadout>
{
   enum
   {
      WithIdenticalViaEquality = true,
   };
};

