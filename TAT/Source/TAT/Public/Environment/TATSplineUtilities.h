// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATSplineUtilities.generated.h"

class USplineComponent;
class USplineMeshComponent;

UENUM(BlueprintType)
enum class ETATSplineSegmentLengthMode : uint8
{
   Auto UMETA(ToolTip = "Automatically compute the length by taking the available space and dividing it equally for each segment set to auto"),
   World UMETA(ToolTip = "Length is specified in world units"),
   Relative UMETA(ToolTip = "Length is specified as a percentage of the total spline length (0..1)"),
};

/// Represents a mesh or series of meshes stretched across a section of spline segment
USTRUCT(BlueprintType)
struct TAT_API FTATSplineMeshSegment
{
   GENERATED_BODY()

   /// Mesh to use for this segment. Leave empty to use the default mesh.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment")
   TObjectPtr<UStaticMesh> Mesh;

   /// The forward axis for the spline mesh orientation
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment")
   TEnumAsByte<ESplineMeshAxis::Type> SplineMeshAxis = ESplineMeshAxis::X;

   /// Determine how the Length property is interpreted
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment")
   ETATSplineSegmentLengthMode LengthMode = ETATSplineSegmentLengthMode::Auto;

   /// Maximum distance along the spline this mesh can take up.
   /// Set to zero to auto-calculate (the segment will expand to fit available space).
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment", Meta = (ClampMin = 0, UIMin = 0, EditCondition = "LengthMode != ETATSplineSegmentLengthMode::Auto"))
   float Length = 0.0f;

   /// How many spline mesh components to stretch across this segment.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment", Meta = (ClampMin = 1, UIMin = 1))
   int32 NumSubdivisions = 1;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment", Meta = (InlineEditConditionToggle))
   bool UseTangentScale = false;

   /// Custom multiplier for tangents applied to spline mesh components in this segment.
   /// If disabled, the value is auto-computed as 1.0/NumSubdivisions
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment", Meta = (EditCondition = "UseTangentScale"))
   float TangentScale = 1.0f;

   /// The starting scale for meshes in this segment
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment")
   FVector2D StartScale = FVector2D(1, 1);

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment", Meta = (InlineEditConditionToggle))
   bool UseEndScale = false;

   /// An explicit scale value for the end of this segment.
   /// If disabled, the start scale value from the next segment will be used (or, if this is the last segment, the start scale value from this segment).
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment", Meta = (EditCondition = "UseEndScale"))
   FVector2D EndScale = FVector2D(1, 1);

   /// The starting roll for meshes in this segment
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment", Meta = (ForceUnits = "degrees"))
   float StartRoll = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment", Meta = (InlineEditConditionToggle))
   bool UseEndRoll = false;

   /// An explicit roll value for the end of this segment.
   /// If disabled, the start roll value from the next segment will be used (or, if this is the last segment, the start roll value from this segment).
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment", Meta = (EditCondition = "UseEndRoll", ForceUnits = "degrees"))
   float EndRoll = 0.0f;

   float GetSegmentLength(float totalSplineLength, float autoSegmentLength) const
   {
      if (LengthMode == ETATSplineSegmentLengthMode::World)
      {
         return FMath::Max(0.1f, Length);
      }
      if (LengthMode == ETATSplineSegmentLengthMode::Relative)
      {
         return FMath::Max(0.1f, Length * totalSplineLength);
      }
      check(LengthMode == ETATSplineSegmentLengthMode::Auto);
      return autoSegmentLength;
   }

   int32 GetNumSegments() const
   {
      return FMath::Max(1, NumSubdivisions);
   }
};


/// Spline mesh component configuration for AutoGenerateSplineMeshComponents
USTRUCT(BlueprintType)
struct TAT_API FTATSplineMeshConfig
{
   GENERATED_BODY()

   /// Default mesh to use for any segment that does not specify a mesh.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment")
   TObjectPtr<UStaticMesh> Mesh;

   /// Default spline mesh axis to use when the default mesh is used.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Segment")
   TEnumAsByte<ESplineMeshAxis::Type> SplineMeshAxis = ESplineMeshAxis::X;

   /// Customize how spline mesh components are generated across the spline. You can use different meshes for different segments.
   /// For best results, you generally want the same number of segments as you have in your spline (in other words, one less than the number of control points).
   ///
   /// If this array is empty, segments are automatically generated.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Mesh Config")
   TArray<FTATSplineMeshSegment> Segments;

   /// Component mobility to apply to all spline mesh components
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Mesh Config")
   TEnumAsByte<EComponentMobility::Type> SegmentMobility = EComponentMobility::Static;

   /// Enable spline mesh component collision?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Mesh Config")
   TEnumAsByte<ECollisionEnabled::Type> SegmentCollision = ECollisionEnabled::QueryAndPhysics;

   /// Collision profile to apply to spline mesh components when collision is enabled
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Mesh Config")
   FCollisionProfileName SegmentCollisionProfile = UCollisionProfile::BlockAll_ProfileName;
};


USTRUCT(BlueprintType)
struct TAT_API FTATCablePoint
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cable Point")
   FVector Position = FVector::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cable Point")
   FVector Velocity = FVector::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cable Point")
   FVector Acceleration = FVector::ZeroVector;

   /// The mass of this point in the cable.
   /// If set to zero, this point will not be fixed in place and not simulated.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cable Point", Meta = (UIMin = 0, ClampMin = 0))
   float Mass = 0.0f;

   /// The resting length of the cable from this point to the next point (the length this cable segment would be if gravity was zero).
   /// This value is ignored for the last point in the cable.
   /// Set to zero to compute automatically.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cable Point")
   float SegmentRestLength = 0.0f;

   FTATCablePoint() = default;
   explicit FTATCablePoint(const FVector& pos, float mass, float segmentRestLength)
      : Position(pos)
      , Velocity()
      , Acceleration()
      , Mass(mass)
      , SegmentRestLength(segmentRestLength)
   {
   }

   FORCEINLINE bool IsSimulated() const
   {
      return Mass > 0;
   }
};


USTRUCT(BlueprintType)
struct TAT_API FTATCableSimulationSettings
{
   GENERATED_BODY()

   /// Acceleration due to gravity (cm/s^2)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cable Simulation Settings")
   float Gravity = 981.0f;

   /// Damping factor to reduce oscillation.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cable Simulation Settings", Meta = (UIMin = 0.9, UIMax = 1.0))
   float Damping = 0.95f;

   /// Spring constant for cable stiffness
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cable Simulation Settings")
   float SpringConstant = 10000.0f;
};


UCLASS()
class TAT_API UTATSplineUtilities : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   /// Spawns spline mesh components to cover a spline. Usable in construction scripts.
   UFUNCTION(BlueprintCallable, Category = "Spline Utilities")
   static void AutoGenerateSplineMeshComponents(USplineComponent* splineComponent, const FTATSplineMeshConfig& splineMeshConfig);

   /// Calls the callback for each SplineMeshComponent on an actor that was generated with AutoGenerateSplineMeshComponents
   static void ForEachAutoGeneratedSplineMeshComponent(USplineComponent* splineComponent, TFunctionRef<void(USplineMeshComponent*)> callback);

   /// Gets all components created with the AutoGenerateSplineMeshComponents function
   UFUNCTION(BlueprintCallable, Category = "Spline Utilities")
   static void GetAllAutoGeneratedSplineMeshComponents(USplineComponent* splineComponent, TArray<USplineMeshComponent*>& splineMeshComponents);

   /// Calls UTATHighlightStateMgrComponent::HighlightMeshesWithTag on all spline mesh components that were generated with AutoGenerateSplineMeshComponents
   static void HighlightAllAutoGeneratedSplineMeshComponents(USplineComponent* splineComponent, bool isHighlighted);

   /// Gets all components created with the AutoGenerateSplineMeshComponents function.
   UFUNCTION(BlueprintCallable, Category = "Spline Utilities")
   static void SetMaterialOnAllAutoGeneratedSplineMeshComponents(USplineComponent* splineComponent, int32 materialIndex, UMaterialInterface* material);

   /// Destroys all spline mesh components created with the AutoGenerateSplineMeshComponents function.
   /// Returns the number of components destroyed.
   UFUNCTION(BlueprintCallable, Category = "Spline Utilities")
   static int32 DestroyAllAutoGeneratedSplineMeshComponents(USplineComponent* splineComponent);

   /// Constructs a cable point array from a start point, an end point, and a number of center points (interpolated locations between start and end).
   /// The mass of each non-fixed point will be set to (cableLengthMeters * cableMassPerMeter) / numCablePoints
   ///
   /// The SegmentRestLength for each point will be computed as the length of that segment multiplied by (1.0 / cableTension)
   ///
   /// If fixedStartPoint or fixedEndPoint is true, the first (and/or last) point will have a mass of zero and will not be simulated.
   /// Note that if you need more control than this, you can take the output cable point array and set whichever points you'd like to have a mass of zero.
   UFUNCTION(BlueprintCallable, Category = "Spline Utilities")
   static void MakeCableFromStartAndEnd(FVector start, FVector end, TArray<FTATCablePoint>& cablePoints, float cableTension = 1.35f, float cableMassPerMeter = 50.0f,
      int32 numCenterPoints = 1, bool fixedStartPoint = true, bool fixedEndPoint = true);

   /// Runs a single cable simulation frame with the specified delta seconds.
   /// Generally, to simulate cable physics you need to run this repeatedly until the cable point positions settle.
   ///
   /// IMPORTANT: This function is intended for use in PRE-computing cable positions. This function is NOT intended for real-time use. Do not use this in tick!
   UFUNCTION(BlueprintCallable, Category = "Spline Utilities")
   static void RunCableSimulationTimestep(const FTATCableSimulationSettings& settings, UPARAM(ref) TArray<FTATCablePoint>& cablePoints, float deltaSeconds = 0.033f);

   /// Applies cable points to a spline component.
   /// This will set the spline point count and positions to match the input cable points.
   UFUNCTION(BlueprintCallable, Category = "Spline Utilities")
   static void ApplyCablePointsToSplineComponent(USplineComponent* splineComponent, const TArray<FTATCablePoint>& cablePoints,
      TEnumAsByte<ESplineCoordinateSpace::Type> coordSpace = ESplineCoordinateSpace::World, bool updateSpline = true);

   /// Constructs a cable from a start and end point, simulates cable physics, and applies the result directly to a spline component.
   /// This function is just a simple wrapper around MakeCableFromStartAndEnd, RunCableSimulationTimestep, and ApplyCablePointsToSplineComponent
   UFUNCTION(BlueprintCallable, Category = "Spline Utilities")
   static void SetupSplineComponentWithCableSimulation(USplineComponent* splineComponent, FVector start, FVector end,
      const FTATCableSimulationSettings& settings, TArray<FTATCablePoint>& cablePoints, TEnumAsByte<ESplineCoordinateSpace::Type> coordSpace = ESplineCoordinateSpace::World,
      float cableTension = 1.35f, float cableMassPerMeter = 50.0f, int32 numCenterPoints = 1, bool fixedStartPoint = true, bool fixedEndPoint = true,
      float simulationTimeSeconds = 5.0f, int32 simulationStepsPerSecond = 30, bool updateSpline = true);

};
