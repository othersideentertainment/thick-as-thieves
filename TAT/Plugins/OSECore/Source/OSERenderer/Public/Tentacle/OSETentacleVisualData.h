// (c) 2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSETentacleVisualData.generated.h"


//---------------------------------------------------------------------------------------
/// Resolved visual data for a tentacle. Used for rendering and debugging.
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSERENDERER_API FOSETentacleVisualData
{
   GENERATED_BODY()

public:

   // Body
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   FVector BodyOrigin = FVector::ZeroVector;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   float BodyRadius = 0.0f;

   // Root
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   FVector RootPosition = FVector::ZeroVector;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   float RootRadius = 0.0f;

   // Tip
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   FVector TipPositionTarget = FVector::ZeroVector;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   FVector TipPositionCurrent = FVector::ZeroVector;

   // Length
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   float LengthTotal = 0.0f;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   float LengthCurrent = 0.0f;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   float LengthNormalized = 0.0f;

   // State
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   float ExtendWeight = 0.0f;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   float RetractWeight = 0.0f;

   /// \see FOSETentacleState::TimeRemaining
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   float TimeRemaining = 0.0f;

   /// \see FOSETentacleState::SphereIdx
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   int32 SphereIdx = 0;
};

