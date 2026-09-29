// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

class USplineComponent;
enum class ETATPowerNetworkRole : uint8;

namespace TATPowerNetworkUtils
{

static constexpr FLinearColor JunctionColor = FLinearColor(0.85f, 0.65f, 0.20f);
static constexpr FLinearColor ConnectorColor = FLinearColor(0.15f, 0.65f, 0.75f);
static constexpr FLinearColor ProviderColor = FLinearColor(0.30f, 0.75f, 0.25f);
static constexpr FLinearColor ConsumerColor = FLinearColor(0.75f, 0.35f, 0.55f);

TAT_API FLinearColor GetColorForRole(ETATPowerNetworkRole powerNetworkRole);

/// Adjusts the saturation and value of a color. Intended for use with functions like FPrimitiveDrawInterface::DrawLine that ignore color opacity values.
///
/// intensity == 0.0: returns black
/// intensity == 0.1: strongly dial back saturation and value
/// intensity == 0.9: slightly dial back saturation and value
/// intensity == 1.0: returns input color
TAT_API FLinearColor ApplyColorIntensity(const FLinearColor& color, float intensity);

/// Gets the world location at srcActor that represents where dstActor would "plug in" to srcActor.
TAT_API FVector GetPowerLinkLocation(AActor* srcActor, AActor* dstActor, bool forDebugVis);

/// Gets the local-space bounding box of an actor.
/// Useful for passing to FPowerNetworkDebugDraw::DrawBox if you intend to pass a local-to-world transform along with it.
TAT_API void GetActorBoundsInLocalSpace(const AActor* actor, FVector& outCenter, FVector& outExtent, bool nonColliding = false, bool includeChildActors = false);

struct FDashedLine
{
   bool Dashed = false;
   TOptional<FLinearColor> DashColor;
   int32 DashInterval = 2;
   int32 MinSegments = 8;
   int32 MaxSegments = 64;
   int32 ExtraNonDashedEndSegments = 4;

   FDashedLine() = default;
   explicit FDashedLine(bool dashed, int32 interval = 2) : Dashed(dashed), DashInterval(interval) {}
   explicit FDashedLine(const FLinearColor& color, int32 interval = 2) : Dashed(true), DashColor(color), DashInterval(interval) {}
};

class TAT_API FPowerNetworkDebugDraw
{
public:
   virtual ~FPowerNetworkDebugDraw() {}

   /// Implement this in a subclass - all other debug draw functions are based on this
   virtual void DrawLine(const FVector& start, const FVector& end, const FLinearColor& color, float thickness) = 0;

   /// Draws a dashed line
   void DrawDashedLine(const FVector& start, const FVector& end, const FLinearColor& color, float thickness, const FDashedLine& dashed);

   /// Draws an arrow from start to end.
   /// arrowLength is the length of the arrowhead lines
   /// arrowWidth is the "diameter" of the arrowhead (roughly speaking)
   void DrawArrow(const FVector& start, const FVector& end, const FLinearColor& color, float thickness, float arrowLength, float arrowWidth);

   /// Draws all edges that make up a box.
   /// Either pass in a transform and pass in the origin and extent in local space, or skip the transform and pass in the origin and extent in world-space (eg. an AABB)
   void DrawBox(const FVector& origin, const FVector& extent, const FLinearColor& color, float thickness, const TOptional<FTransform>& transform = NullOpt);

   /// Draws an arrow connecting srcActor to dstActor, using the power link locations for each.
   void DrawPowerLink(AActor* srcActor, AActor* dstActor, const FLinearColor& color, float thickness, const FVector& lineOffset = FVector::ZeroVector, float arrowLength = 16.0f, float arrowWidth = 24.0f);

   /// Draws a spline component.
   /// If lineOffset is specified, the line will "bend" away from the actual spline (useful for being able to see a spline mesh or spline editor visualization at the same time)
   void DrawSplineComponent(const USplineComponent* splineComponent, const FLinearColor& color, float thickness, const FVector& lineOffset = FVector::ZeroVector, const FDashedLine& dashed = FDashedLine{});

   void DrawActorBoundingBox(AActor* actor, const FLinearColor& color, float thickness, float boundsOffset = 0.0f);

   /// Finds the spline component for an actor and draws it with DrawSplineComponent.
   /// This will be either the one returned by UTATPowerNetworkInterface::GetPowerNetworkConnectorSplineComponent or the root component.
   void DrawActorSplineComponent(AActor* actor, const FLinearColor& color, float thickness, const FDashedLine& dashed, float extraLineOffset = 0.0f);

};

} // namespace TATPowerNetworkUtils
