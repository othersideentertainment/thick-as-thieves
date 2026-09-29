// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"

// OSE
#include "OSEAnimTypes.generated.h"


OSECORE_API DECLARE_LOG_CATEGORY_EXTERN(LogOSEAnimation, Log, All);


//--------------------------------------------------------------------------------------------------
/// Animation-specific interpretation of character state. Mostly corresponds to movement modes.
//--------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EOSEAnimState : uint8
{
   Unknown,    ///< Unknown or undefined state.
   Walking,    ///< In the walking / nav walking movement mode. This is when the character is on the ground, whether they are moving or not.
   Falling,    ///< In the falling movement mode. This is when the character is in the air.
   Flying,     ///< In the flying movement mode. Used during some abilities.
   Swimming,   ///< In the swimming movement mode.
   Mantling,   ///< In the custom mantling movement mode.
   Scrambling,   ///< In the custom climbing movement mode.
};


//--------------------------------------------------------------------------------------------------
/// Animation states converted to packed boolean flags, indicating if a specific state is equal or
/// is not equal to a given value. This is simply to provide a workaround for animation graph
/// state transitions: comparing enum values (and even negating booleans) in transitions marks the
/// transition as "uses Blueprint to update its values". Packing the results in this struct
/// effectively works around this limitation for this common use case.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimStateFlags
{
   GENERATED_BODY()

public:

   FOSEAnimStateFlags();
   FOSEAnimStateFlags(EOSEAnimState inState);

   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 IsUnknown     : 1;  // State == Unknown
   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 IsWalking     : 1;  // State == Walking
   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 IsFalling     : 1;  // State == Falling
   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 IsFlying      : 1;  // State == Flying
   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 IsSwimming    : 1;  // State == Swimming
   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 IsMantling    : 1;  // State == Mantling
   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 IsScrambling  : 1;  // State == Scrambling
};
