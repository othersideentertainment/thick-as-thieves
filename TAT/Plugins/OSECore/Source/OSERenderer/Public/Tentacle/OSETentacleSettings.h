// (c) 2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSETentacleSettings.generated.h"


//---------------------------------------------------------------------------------------
/// Structs to encapsulate properties in logical groups corresponding to categories.
/// This makes it easier for content creators to understand related properties, in
/// particular the advanced properties for some settings groups.
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSERENDERER_API FOSETentacleSettings_Size
{
   GENERATED_BODY()

public:

   /// Radius of the max tentacle "thickness"; used for traces, distribution of the points, and minimum lengths
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, meta = (ClampMin = 1))
   float TentacleRadius = 10;

   /// Radius of main body
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, meta = (ClampMin = 1))
   float BodyRadius = 50;
};


USTRUCT(BlueprintType)
struct OSERENDERER_API FOSETentacleSettings_Speed
{
   GENERATED_BODY()

public:

   /// Speed that the tentacles extend towards the target
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, meta = (ClampMin = 1))
   float ExtendSpeed = 200;

   /// Speed that the tentacles retract back the body
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, meta = (ClampMin = 1))
   float RetractSpeed = 300;
};


USTRUCT(BlueprintType)
struct OSERENDERER_API FOSETentacleSettings_Spawn
{
   GENERATED_BODY()

public:

   /// Min and max time an extended entry will stay alive
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, meta = (ClampMin = 0.1))
   FFloatInterval LifeTime = FFloatInterval(5, 10);

   /// When enabled, the minimum lifetime for a new tentacle is at least the remaining time of current tentacles.
   /// NOTE: This will tend to cause tentacles to retract in order, and spawn less frequently.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay)
   bool WaitForLongestTimeRemaining = false;

   /// When enabled, the tentacle will begin aging immediately, as opposed to waiting until it has been fully extended.
   /// NOTE: Depending on speed and distance, tentacles that start aging immediately may never reach their target.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay)
   bool BeginAgingImmediately = false;

   /// When enabled, the location on the sphere is adjusted based on closest surface point.
   /// When disabled, the initial sphere location is always used.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay)
   bool AdjustSphereLocation = false;

   /// The maximum number of times per second a new tentacle can be extended
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay, meta = (ClampMin = 0, ClampMax = 144))
   int32 NewTentacleExtendRate = 30;

   /// Excludes any rotations within this cone angle at the positive forward axis
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay, meta = (ClampMin = 0, ClampMax = 180, Units = deg))
   float ExcludeConeAngle = 0;
};


USTRUCT(BlueprintType)
struct OSERENDERER_API FOSETentacleSettings_Length
{
   GENERATED_BODY()

public:

   /// Minimum and maximum length for tentacles.
   /// Consider using "Enable Separate Axis Length" if you want to specify the lengths per axis instead.
   /// \see EnableSeparateAxisLengths
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, meta = (EditCondition = "!EnableSeparateAxisLengths", ClampMin = 0))
   FFloatInterval Length = FFloatInterval(50, 300);

   /// The total length is decreased by this amount when querying for new locations (not used for updating existing locations).
   /// The effective minimum length is _increased_ by half this amount, while the maximum length is _decreased_ by half this amount.
   /// This helps to prevent situations where new entries are shortly removed due to failing the length check.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay, meta = (ClampMin = 0))
   float QueryLengthDecrease = 100;

   /// When enabled, the min and max length can be specified in specific axis directions
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay)
   bool EnableSeparateAxisLengths = false;

   /// Min/max length (world space) in the local space +X direction
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay, meta = (EditCondition = "EnableSeparateAxisLengths", ClampMin = 0))
   FFloatInterval LengthFront = FFloatInterval(0, 0);

   /// Min/max length (world space) in the local space -X direction
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay, meta = (EditCondition = "EnableSeparateAxisLengths", ClampMin = 0))
   FFloatInterval LengthBack = FFloatInterval(0, 0);

   /// Min/max length (world space) in the local space +Y direction
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay, meta = (EditCondition = "EnableSeparateAxisLengths", ClampMin = 0))
   FFloatInterval LengthRight = FFloatInterval(0, 0);

   /// Min/max length (world space) in the local space -Y direction
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay, meta = (EditCondition = "EnableSeparateAxisLengths", ClampMin = 0))
   FFloatInterval LengthLeft = FFloatInterval(0, 0);

   /// Min/max length (world space) in the local space +Z direction
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay, meta = (EditCondition = "EnableSeparateAxisLengths", ClampMin = 0))
   FFloatInterval LengthTop = FFloatInterval(0, 0);

   /// Min/max length (world space) in the local space -Z direction
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, AdvancedDisplay, meta = (EditCondition = "EnableSeparateAxisLengths", ClampMin = 0))
   FFloatInterval LengthBottom = FFloatInterval(0, 0);
};


//---------------------------------------------------------------------------------------
/// NOTE: This struct would typically be the top-level settings struct, with nested
/// struct properties for each of the settings structs defined above. As of UE 5.1
/// however, advanced properties don't even show up in the editor _at all_ unless they
/// are in a top-level category. In order to be able to keep reasonable semantics and
/// ergonomics when accessing settings, to avoid needless copying of struct data when
/// only a few members are needed, and to provide a simple upgrade path if this bug
/// is fixed, this struct instead keeps const references to existing struct properties.
//---------------------------------------------------------------------------------------

struct OSERENDERER_API FOSETentacleSettings
{
public:

   FORCEINLINE FOSETentacleSettings(
      const FOSETentacleSettings_Size& size,
      const FOSETentacleSettings_Speed& speed,
      const FOSETentacleSettings_Spawn& spawn,
      const FOSETentacleSettings_Length& length)
      : Size(size)
      , Speed(speed)
      , Spawn(spawn)
      , Length(length)
   { }

   const FOSETentacleSettings_Size& Size;
   const FOSETentacleSettings_Speed& Speed;
   const FOSETentacleSettings_Spawn& Spawn;
   const FOSETentacleSettings_Length& Length;
};
