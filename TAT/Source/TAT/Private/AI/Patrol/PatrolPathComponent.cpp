// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Patrol/PatrolPathComponent.h"

// tat
#include "AI/Patrol/AIPathInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PatrolPathComponent)

UPatrolPathComponent::UPatrolPathComponent()
{
}

#if WITH_EDITOR
FPrimitiveSceneProxy* UPatrolPathComponent::CreateSceneProxy()
{
   class FPatrolSceneProxy final : public FPrimitiveSceneProxy
   {
   public:
      SIZE_T GetTypeHash() const override
      {
         static size_t uniquePointer;
         return reinterpret_cast<size_t>(&uniquePointer);
      }

      FPatrolSceneProxy(const UPrimitiveComponent* inComponent, IAIPathInterface* path)
         : FPrimitiveSceneProxy(inComponent)
         , lineColor(FLinearColor::White)
      {
         if (path)
         {
            const int32 numPoints = path->GetNumPoints();
            pathPoints.Reserve(numPoints);
            for (int32 i = 0; i < numPoints; ++i)
            {
               const FVector location = path->GetPointLocationLocalSpace(i);
               pathPoints.Add(location);
            }
         }
      }

      virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& views, const FSceneViewFamily& viewFamily, uint32 visibilityMap, FMeshElementCollector& collector) const override
      {
         QUICK_SCOPE_CYCLE_COUNTER(STAT_PatrolSceneProxy_GetDynamicMeshElements);

         if (IsSelected())
         {
            return;
         }

         for (int32 viewIndex = 0; viewIndex < views.Num(); viewIndex++)
         {
            if (visibilityMap & (1 << viewIndex))
            {
               const FSceneView* view = views[viewIndex];
               FPrimitiveDrawInterface* pdi = collector.GetPDI(viewIndex);

               const FMatrix& localToWorld = GetLocalToWorld();

               // Taking into account the min and maximum drawing distance
               const float distanceSqr = (view->ViewMatrices.GetViewOrigin() - localToWorld.GetOrigin()).SizeSquared();
               if (distanceSqr < FMath::Square(GetMinDrawDistance()) || distanceSqr > FMath::Square(GetMaxDrawDistance()))
               {
                  continue;
               }


               for (int32 i = 1; i < pathPoints.Num(); ++i)
               {
                  pdi->DrawPoint(localToWorld.TransformPosition(pathPoints[i]), lineColor, 10, SDPG_World);
                  pdi->DrawLine(localToWorld.TransformPosition(pathPoints[i - 1]),
                     localToWorld.TransformPosition(pathPoints[i]), lineColor, SDPG_World, 0.f, 0.01f);
               }
            }
         }
      }

      virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
      {
         // NOTE: it probably isn't worth an engine mod to add a new ShowFlag for this, so gate on splines for now
         //       unless DebugAI would be preferable?
         FPrimitiveViewRelevance Result;
         Result.bDrawRelevance = !IsSelected() && IsShown(View) && View->Family->EngineShowFlags.Splines;
         Result.bDynamicRelevance = true;
         Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
         return Result;
      }

      virtual uint32 GetMemoryFootprint(void) const override { return sizeof * this + GetAllocatedSize(); }
      uint32 GetAllocatedSize(void) const { return FPrimitiveSceneProxy::GetAllocatedSize() + pathPoints.GetAllocatedSize(); }

   private:
      TArray<FVector> pathPoints;
      FLinearColor lineColor;
   };

   return new FPatrolSceneProxy(this, _GetPath());
}

FBoxSphereBounds UPatrolPathComponent::CalcBounds(const FTransform& localToWorld) const
{
   FBox boundingBox;
   boundingBox.Init();
   if (IAIPathInterface* path = _GetPath())
   {
      const int32 numPoints = path->GetNumPoints();
      for (int32 i = 0; i < numPoints; ++i)
      {
         const FVector location = path->GetPointLocationLocalSpace(i);
         boundingBox += location;
      }
   }

   return FBoxSphereBounds(boundingBox.TransformBy(localToWorld));
}

IAIPathInterface* UPatrolPathComponent::_GetPath() const
{
   return Cast<IAIPathInterface>(GetOwner());
}

#endif


