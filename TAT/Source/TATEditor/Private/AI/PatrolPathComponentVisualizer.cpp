// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/PatrolPathComponentVisualizer.h"

// tat
#include "AI/Patrol/PatrolPath.h"
#include "AI/Patrol/AIPathInterface.h"

// ue
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "SceneManagement.h"

#include "Kismet/KismetMathLibrary.h"
#include "Math/Box.h"

void FPatrolPathComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   if (component == nullptr)
   {
      return;
   }
   AActor* pathActor = component->GetOwner();
   const IAIPathInterface* pathInterface = Cast<IAIPathInterface>(pathActor);
   if (!pathActor || !pathInterface) return;

   auto drawArrow = [&](const FVector& p1, const FVector& p2, const FLinearColor color, const float headPosition)
   {
      FTransform transform(UKismetMathLibrary::FindLookAtRotation(p1, p2), p1);

      const float thickness = 2.f;
      const float halfLength = FVector::Distance(p1, p2) * headPosition;
      const float arrowSize = 20.f;

      pdi->DrawLine(p1, p2, color, SDPG_Foreground, thickness);
      pdi->DrawLine(transform.TransformPosition(FVector(halfLength, 0, 0)), transform.TransformPosition(FVector(halfLength - arrowSize, arrowSize, 0)), color, SDPG_Foreground, thickness);
      pdi->DrawLine(transform.TransformPosition(FVector(halfLength, 0, 0)), transform.TransformPosition(FVector(halfLength - arrowSize, -arrowSize, 0)), color, SDPG_Foreground, thickness);
   };

   const APatrolPath* patrolPath = Cast<APatrolPath>(pathActor);

   const int32 numPoints = pathInterface->GetNumPoints();
   for (int32 i = 0; i < numPoints; ++i)
   {
      if (patrolPath && patrolPath->Points[i].HasFacing)
      {
         const FRotator r = patrolPath->Points[i].Facing;
         const FVector a = pathInterface->GetPointLocationWorldSpace(i);
         const FVector b = a + r.RotateVector(FVector::ForwardVector) * 75.0f;
         drawArrow(a, b, FColor::Yellow, 1.0f);
      }
      const FNextPointData nextPointData = pathInterface->GetNextPoint(i, true);
      
      if(nextPointData.nextIndexID == -1)
         break;
      
      // for ping pong, we don't want to draw an arrow backwards
      if(nextPointData.forwardMovementDirection == false)
         break;
      
      
      const FLinearColor color = pathInterface->GetColorForPoint(i);
      const FVector a = pathInterface->GetPointLocationWorldSpace(i);
      const FVector b = pathInterface->GetPointLocationWorldSpace(nextPointData.nextIndexID);

      if(GEditor->GetSelectedActorCount() == 1)
      {
         UNavigationPath* path = UNavigationSystemV1::FindPathToLocationSynchronously(pathActor, a, b);
         if(path != nullptr)
         {
            for(int p1 = 0; p1 < path->PathPoints.Num() - 1; ++p1)
            {
               const int nextPathIndex = p1 + 1;
               if(path->PathPoints.IsValidIndex(nextPathIndex))
               {
                  drawArrow(path->PathPoints[p1], path->PathPoints[p1+1], FLinearColor::Red, 0.5f);
               }
            }
         }
      }
      drawArrow(a, b, color, 0.5f);
   }
}
