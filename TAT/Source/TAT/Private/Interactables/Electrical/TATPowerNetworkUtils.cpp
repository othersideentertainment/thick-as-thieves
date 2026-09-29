// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/Electrical/TATPowerNetworkUtils.h"

// ose
#include "Components/SplineComponent.h"
#include "Interactables/Electrical/TATPowerNetworkComponent.h"
#include "Interactables/Electrical/TATPowerNetworkInterface.h"
#include "Math/OSEMathFunctionLibrary.h"

namespace TATPowerNetworkUtils
{

// Helper for FDashedLine
static int32 GetDashedLineSegmentCount(const FDashedLine& dashed, float totalLength)
{
   int32 numSegments = FMath::Clamp(FMath::RoundToInt(totalLength / static_cast<float>(dashed.MaxSegments)), dashed.MinSegments, dashed.MaxSegments);
   while (numSegments > dashed.MinSegments && totalLength / static_cast<float>(numSegments) < 10.0f)
   {
      --numSegments;
   }
   return numSegments + (FMath::Max(0, dashed.ExtraNonDashedEndSegments) * 2);
}

// Helper for FDashedLine
static TOptional<FLinearColor> GetDashedLineSegmentColor(const FDashedLine& dashed, const FLinearColor& defaultColor, int32 segmentIndex, int32 numSegments)
{
   const int32 numEndSegments = FMath::Max(0, dashed.ExtraNonDashedEndSegments);
   if (dashed.Dashed && segmentIndex >= numEndSegments && segmentIndex <= numSegments - numEndSegments && (segmentIndex % dashed.DashInterval) != 0)
   {
      return dashed.DashColor;
   }
   return defaultColor;
}

FLinearColor GetColorForRole(ETATPowerNetworkRole powerNetworkRole)
{
   switch (powerNetworkRole)
   {
   case ETATPowerNetworkRole::None:
      break;
   case ETATPowerNetworkRole::Junction:
      return JunctionColor;
   case ETATPowerNetworkRole::Connector:
      return ConnectorColor;
   case ETATPowerNetworkRole::Provider:
      return ProviderColor;
   case ETATPowerNetworkRole::Consumer:
      return ConsumerColor;
   default:
      break;
   }
   return FLinearColor::White;
}

FLinearColor ApplyColorIntensity(const FLinearColor& color, float intensity)
{
   intensity = FMath::Clamp(intensity, 0.0f, 1.0f);
   FLinearColor hsvColor = color.LinearRGBToHSV();
   hsvColor.G *= UOSEMathFunctionLibrary::InterpolateNormalized(intensity, EOSEInterpMode::SinEaseOut); // saturation
   hsvColor.B *= UOSEMathFunctionLibrary::InterpolateNormalized(intensity, EOSEInterpMode::Log2EaseOut); // value
   return hsvColor.HSVToLinearRGB();
}

FVector GetPowerLinkLocation(AActor* srcActor, AActor* dstActor, bool forDebugVis)
{
   check(srcActor != nullptr);
   check(dstActor != nullptr);
   if (UTATPowerNetworkComponent* srcComp = UTATPowerNetworkComponent::GetPowerNetworkComponent(srcActor))
   {
      FVector result = FVector::ZeroVector;
      if (srcComp->GetWorldLocationForLinkedActor(dstActor, result, forDebugVis))
      {
         return result;
      }
   }
   return srcActor->GetActorLocation();
}

void GetActorBoundsInLocalSpace(const AActor* actor, FVector& outCenter, FVector& outExtent, bool nonColliding, bool includeChildActors)
{
   if (actor != nullptr)
   {
      const FBox box = actor->CalculateComponentsBoundingBoxInLocalSpace(nonColliding, includeChildActors);
      outCenter = box.GetCenter();
      outExtent = box.GetExtent();
   }
   else
   {
      outCenter = FVector::ZeroVector;
      outExtent = FVector::ZeroVector;
   }
}

void FPowerNetworkDebugDraw::DrawDashedLine(const FVector& start, const FVector& end, const FLinearColor& color, float thickness, const FDashedLine& dashed)
{
   if (!dashed.Dashed)
   {
      DrawLine(start, end, color, thickness);
      return;
   }

   const int32 numSegments = GetDashedLineSegmentCount(dashed, FVector::Distance(start, end));

   for (int32 segmentEndIdx = 1; segmentEndIdx <= numSegments; ++segmentEndIdx)
   {
      const TOptional<FLinearColor> segmentColor = GetDashedLineSegmentColor(dashed, color, segmentEndIdx, numSegments);
      if (!segmentColor)
      {
         continue;
      }
      const float startAlpha = static_cast<float>(segmentEndIdx - 1) / static_cast<float>(numSegments);
      const float endAlpha = static_cast<float>(segmentEndIdx) / static_cast<float>(numSegments);
      DrawLine(FMath::Lerp(start, end, startAlpha), FMath::Lerp(start, end, endAlpha), *segmentColor, thickness);
   }
}

void FPowerNetworkDebugDraw::DrawArrow(const FVector& start, const FVector& end, const FLinearColor& color, float thickness, float arrowLength, float arrowWidth)
{
   DrawLine(start, end, color, thickness);
   const FTransform transform(FRotationMatrix::MakeFromX(end - start).Rotator(), end);
   const float arrowHalfWidth = FMath::Max(0.1f, arrowWidth * 0.5f);
   DrawLine(transform.GetLocation(), transform.TransformPosition(FVector(-arrowLength, +arrowHalfWidth, +arrowHalfWidth)), color, thickness);
   DrawLine(transform.GetLocation(), transform.TransformPosition(FVector(-arrowLength, +arrowHalfWidth, -arrowHalfWidth)), color, thickness);
   DrawLine(transform.GetLocation(), transform.TransformPosition(FVector(-arrowLength, -arrowHalfWidth, +arrowHalfWidth)), color, thickness);
   DrawLine(transform.GetLocation(), transform.TransformPosition(FVector(-arrowLength, -arrowHalfWidth, -arrowHalfWidth)), color, thickness);
}

void FPowerNetworkDebugDraw::DrawBox(const FVector& origin, const FVector& extent, const FLinearColor& color, float thickness, const TOptional<FTransform>& transform)
{
   static constexpr int32 numPoints = 8;
   FVector points[numPoints] = {
      // top points of the box
      origin + FVector(extent.X, extent.Y, extent.Z),
      origin + FVector(extent.X, -extent.Y, extent.Z),
      origin + FVector(-extent.X, -extent.Y, extent.Z),
      origin + FVector(-extent.X, extent.Y, extent.Z),
      // bottom points of the box
      origin + FVector(extent.X, extent.Y, -extent.Z),
      origin + FVector(extent.X, -extent.Y, -extent.Z),
      origin + FVector(-extent.X, -extent.Y, -extent.Z),
      origin + FVector(-extent.X, extent.Y, -extent.Z),
   };

   if (transform)
   {
      for (int32 i = 0; i < numPoints; i++)
      {
         points[i] = transform->TransformPosition(points[i]);
      }
   }

   auto line = [&](int32 idxA, int32 idxB)
   {
      DrawLine(points[idxA], points[idxB], color, thickness);
   };

   // top square
   line(0, 1);
   line(1, 2);
   line(2, 3);
   line(3, 0);

   // bottom square
   line(4, 5);
   line(5, 6);
   line(6, 7);
   line(7, 4);

   // edges connecting the top and bottom squares
   line(0, 4);
   line(1, 5);
   line(2, 6);
   line(3, 7);
}

void FPowerNetworkDebugDraw::DrawPowerLink(AActor* srcActor, AActor* dstActor, const FLinearColor& color, float thickness, const FVector& lineOffset, float arrowLength, float arrowWidth)
{
   if (srcActor == nullptr || dstActor == nullptr)
   {
      return;
   }
   static constexpr bool forDebugVis = true;
   DrawArrow(
      GetPowerLinkLocation(srcActor, dstActor, forDebugVis) + lineOffset,
      GetPowerLinkLocation(dstActor, srcActor, forDebugVis) + lineOffset,
      color,
      thickness,
      arrowLength,
      arrowWidth);
}

void FPowerNetworkDebugDraw::DrawSplineComponent(const USplineComponent* splineComponent, const FLinearColor& color, float thickness, const FVector& lineOffset, const FDashedLine& dashed)
{
   check(splineComponent != nullptr);

   const float totalLength = splineComponent->GetSplineLength();
   if (totalLength <= 0.0f)
   {
      return;
   }

   const EOSEInterpMode lineOffsetCurve = EOSEInterpMode::SquareRoot;

   auto mapNormalizedValueToCurvedFadeInOut = [](float alpha, EOSEInterpMode mode) -> float
   {
      if (alpha > 0.5f)
      {
         alpha = 1.0f - alpha;
      }
      return UOSEMathFunctionLibrary::InterpolateNormalized(FMath::Clamp(alpha * 2.0f, 0.0f, 1.0f), mode);
   };

   const int32 numSegments = GetDashedLineSegmentCount(dashed, totalLength);
   for (int32 segmentEndIdx = 1; segmentEndIdx <= numSegments; ++segmentEndIdx)
   {
      const TOptional<FLinearColor> segmentColor = GetDashedLineSegmentColor(dashed, color, segmentEndIdx, numSegments);
      if (!segmentColor)
      {
         continue;
      }
      const int32 segmentStartIdx = segmentEndIdx - 1;
      const float startAlpha = static_cast<float>(segmentStartIdx) / static_cast<float>(numSegments);
      const float endAlpha = static_cast<float>(segmentEndIdx) / static_cast<float>(numSegments);
      FVector startLoc = splineComponent->GetWorldLocationAtDistanceAlongSpline(startAlpha * totalLength);
      FVector endLoc = splineComponent->GetWorldLocationAtDistanceAlongSpline(endAlpha * totalLength);
      if (lineOffset != FVector::ZeroVector)
      {
         startLoc += lineOffset * mapNormalizedValueToCurvedFadeInOut(startAlpha, lineOffsetCurve);
         endLoc += lineOffset * mapNormalizedValueToCurvedFadeInOut(endAlpha, lineOffsetCurve);
      }
      DrawLine(startLoc, endLoc, *segmentColor, thickness);
   }
}

void FPowerNetworkDebugDraw::DrawActorBoundingBox(AActor* actor, const FLinearColor& color, float thickness, float boundsOffset)
{
   if (actor == nullptr)
   {
      return;
   }
   FVector origin, extent;
   GetActorBoundsInLocalSpace(actor, origin, extent);
   DrawBox(origin, extent + FVector(boundsOffset * 0.5f), color, thickness, actor->GetActorTransform());
}

void FPowerNetworkDebugDraw::DrawActorSplineComponent(AActor* actor, const FLinearColor& color, float thickness, const FDashedLine& dashed, float extraLineOffset)
{
   if (actor == nullptr)
   {
      return;
   }

   ITATPowerNetworkInterface* powerInterface = Cast<ITATPowerNetworkInterface>(actor);
   USplineComponent* splineComponent = (powerInterface != nullptr)
      ? powerInterface->GetPowerNetworkConnectorSplineComponent()
      : Cast<USplineComponent>(actor->GetRootComponent());

   if (splineComponent == nullptr)
   {
      return;
   }

#if WITH_EDITORONLY_DATA
   UTATPowerNetworkComponent* comp = UTATPowerNetworkComponent::GetPowerNetworkComponent(actor);
   float lineOffset = (comp != nullptr) ? comp->ConnectorEditorVisualizationOffset : 0.0f;
#else
   float lineOffset = 50.0f;
#endif

   DrawSplineComponent(splineComponent, color, thickness, FVector::UpVector * (lineOffset + extraLineOffset), dashed);
}

} // namespace TATPowerNetworkUtils
