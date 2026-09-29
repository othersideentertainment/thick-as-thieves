// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATSavedFtueState.generated.h"

// The point in the FTUE (First Time User Experience) that the player has reached
//
// Only covers parts that the gets saved and resumed at
//
// Compatibility note: The enum is persisted by name in the save game,
//                     so these can be renumbered if needed, as long as the game handles
//                     that state
UENUM(BlueprintType)
enum class ETATSavedFtueState : uint8
{
   // No tutorial has started
   Unstarted,
   // The FTUE level has been completed successfully
   InThievesDen,
   // Fully completed
   Complete
};

USTRUCT()
struct TAT_API FTATSavedFtueState
{
   GENERATED_BODY()

   static constexpr uint8 kCurrentVersion = 0;

   UPROPERTY()
   ETATSavedFtueState State = ETATSavedFtueState::Unstarted;

   // For future compatibility shims
   // Could have just skipped it for now, but :shrug:
   UPROPERTY()
   uint8 Version = kCurrentVersion;
};
