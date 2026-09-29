// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/OSEAIPerceptionHelpers.h"

// unreal
#include "AIHelpers.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

// ose
#include "Camera/OSECameraUtils.h"

#include "DrawDebugHelpers.h"

namespace OSEAIPerceptionHelpersCVars
{
   static int DebugDrawFrustumCheckTargetBounds = 0;
   FAutoConsoleVariableRef CVarDebugCombatHitboxes(
      TEXT("OSE.Perception.CheckIsTargetInFrustum.DebugDrawTargetBounds"),
      DebugDrawFrustumCheckTargetBounds,
      TEXT("Debug draw target bounds during frustum checking?"),
      ECVF_Default);
}

namespace OSEAIPerceptionHelpers
{
   void DrawDebugTargetFrustum(const UWorld* world,
      const FVector& observerLocation,
      const FRotator& observerRotation,
      const float halfFOVInDegrees,
      float nearClip,
      float farClip,
      const float frustumPitch,
      const float frustumAspectRatio,
      const FColor debugColor,
      float lifeTime)
   {

      const FMatrix projectionMatrix = CreateProjectionMatrix(observerLocation, observerRotation, halfFOVInDegrees, nearClip, farClip, frustumPitch, frustumAspectRatio);

      FConvexVolume frustum;
      GetViewFrustumBounds(frustum, projectionMatrix, true);

      DrawDebugFrustum(world, projectionMatrix.Inverse(), debugColor, false, lifeTime, 0U, 2.0f);
   }

   bool CheckIsTargetInFrustum(const UWorld* world,
      const FVector& observerLocation,
      const FRotator& observerRotation,
      const FVector& targetLocation,
      const FVector& targetExtents,
      const float halfFOVInDegrees,
      float nearClip,
      float farClip,
      const float frustumPitch,
      const float frustumAspectRatio)
   {
      nearClip = FMath::Max(nearClip, 1.0f);
      farClip = FMath::Max(farClip, nearClip + 1.0f);

      const FMatrix projectionMatrix = CreateProjectionMatrix(
         observerLocation,
         observerRotation,
         halfFOVInDegrees,
         nearClip,
         farClip,
         frustumPitch,
         frustumAspectRatio
      );

      FConvexVolume frustum;
      GetViewFrustumBounds(frustum, projectionMatrix, true);

      const bool isUsingPointCheck = targetExtents.IsNearlyZero();
#if ENABLE_DRAW_DEBUG
   if (OSEAIPerceptionHelpersCVars::DebugDrawFrustumCheckTargetBounds)
   {
      if(isUsingPointCheck)
      {
         DrawDebugPoint(world, targetLocation, 15.f, FColor::Blue, false, -1.0f, 0U);
      }
      else
      {
         DrawDebugBox(world, targetLocation, targetExtents, FColor::Blue, false, -1.0f, 0U, 2.0f);
      }
      DrawDebugFrustum(world, projectionMatrix.Inverse(), FColor::Red, false, -1.0f, 0U, 2.0f);
   }
#endif

      return isUsingPointCheck ? frustum.IntersectPoint(targetLocation) : frustum.IntersectBox(targetLocation, targetExtents);
   }

   OSEAI_API FMatrix CreateProjectionMatrix(const FVector& observerLocation, const FRotator& observerRotation, float halfFOVInDegrees, float nearClip, float farClip, float frustumPitch, float frustumAspectRatio) 
   {
      nearClip = FMath::Max(nearClip, 1.0f);
      farClip = FMath::Max(farClip, nearClip + 1.0f);

      const float halfFOVInRadians = FMath::DegreesToRadians(FMath::Clamp(halfFOVInDegrees, 0.1f, 89.9f));
      const float aspectRatio = FMath::Max(frustumAspectRatio, KINDA_SMALL_NUMBER);

      FRotator pitchedObserverRotation = FRotator(observerRotation.Pitch + frustumPitch, observerRotation.Yaw, observerRotation.Roll);

      FMatrix projectionMatrix = UOSECameraUtils::BuildViewProjectionMatrix(
         observerLocation,
         pitchedObserverRotation,
         halfFOVInRadians,
         aspectRatio,
         nearClip,
         farClip);

      return projectionMatrix;
   }

   OSEAI_API void GenerateFrustumSegmentList(const FMatrix& frustumToWorld, TArray<FVector>& outSegmentList)
   {
      FVector vertices[2][2][2];
      for (uint32 z = 0; z < 2; z++)
      {
         for (uint32 y = 0; y < 2; y++)
         {
            for (uint32 x = 0; x < 2; x++)
            {
               FVector4 unprojectedVertex = frustumToWorld.TransformFVector4(
                  FVector4(
                     (x ? -1.0f : 1.0f),
                     (y ? -1.0f : 1.0f),
                     (z ? 0.0f : 1.0f),
                     1.0f
                  )
               );
               vertices[x][y][z] = FVector(unprojectedVertex) / unprojectedVertex.W;
            }
         }
      }

      // 12 lines, two points each
      // near plane, far plane, and the lines connecting them 
      outSegmentList.Reset(24);

      outSegmentList.Add(vertices[0][0][0]);
      outSegmentList.Add(vertices[0][0][1]);

      outSegmentList.Add(vertices[1][0][0]);
      outSegmentList.Add(vertices[1][0][1]);

      outSegmentList.Add(vertices[0][1][0]);
      outSegmentList.Add(vertices[0][1][1]);

      outSegmentList.Add(vertices[1][1][0]);
      outSegmentList.Add(vertices[1][1][1]);

      outSegmentList.Add(vertices[0][0][0]);
      outSegmentList.Add(vertices[0][1][0]);

      outSegmentList.Add(vertices[1][0][0]);
      outSegmentList.Add(vertices[1][1][0]);

      outSegmentList.Add(vertices[0][0][1]);
      outSegmentList.Add(vertices[0][1][1]);

      outSegmentList.Add(vertices[1][0][1]);
      outSegmentList.Add(vertices[1][1][1]);

      outSegmentList.Add(vertices[0][0][0]);
      outSegmentList.Add(vertices[1][0][0]);

      outSegmentList.Add(vertices[0][1][0]);
      outSegmentList.Add(vertices[1][1][0]);

      outSegmentList.Add(vertices[0][0][1]);
      outSegmentList.Add(vertices[1][0][1]);

      outSegmentList.Add(vertices[0][1][1]);
      outSegmentList.Add(vertices[1][1][1]);
   }
}
