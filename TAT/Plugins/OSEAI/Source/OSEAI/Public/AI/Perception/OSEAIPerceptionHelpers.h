// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// unreal
#include "CoreMinimal.h"
#include "Perception/AISense_Sight.h"
#include "ConvexVolume.h"

struct FPerceptionListener;
class UAIPerceptionComponent;

namespace OSEAIPerceptionHelpers
{
   OSEAI_API void DrawDebugTargetFrustum(const UWorld* world,
      const FVector& observerLocation,
      const FRotator& observerRotation,
      float halfFOVInDegrees,
      float nearClip,
      float farClip,
      float frustumPitch,
      float frustumAspectRatio,
      FColor debugColor,
      float lifeTime = -1.0f);

   OSEAI_API bool CheckIsTargetInFrustum(const UWorld* world,
      const FVector& observerLocation,
      const FRotator& observerRotation,
      const FVector& targetLocation,
      const FVector& targetExtents,
      float halfFOVInDegrees,
      float nearClip,
      float farClip,
      float frustumPitch,
      float frustumAspectRatio);

   OSEAI_API FMatrix CreateProjectionMatrix(const FVector& observerLocation, 
      const FRotator& observerRotation,
      float halfFOVInDegrees,
      float nearClip,
      float farClip,
      float frustumPitch,
      float frustumAspectRatio);

   // Generates an array of points that can be used with FGameplayDebuggerShape::MakeSegmentList
   // to generate a gameplay-debugger friendly frustum shape.
   OSEAI_API void GenerateFrustumSegmentList(const FMatrix& frustumToWorld, TArray<FVector>& outSegmentList);
};
