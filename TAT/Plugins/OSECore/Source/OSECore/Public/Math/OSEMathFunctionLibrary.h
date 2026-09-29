// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSEMathFunctionLibrary.generated.h"

UENUM(BlueprintType)
enum class EOSEComparisonMethod : uint8
{
   EqualTo,
   NotEqualTo,
   GreaterThanOrEqualTo,
   LessThanOrEqualTo,
   GreaterThan,
   LessThan
};

UENUM(BlueprintType)
enum class EOSEInterpMode : uint8
{
   Linear = 0,

   Sin                 UMETA(DisplayName = "Sin(X)"),
   Pow2                UMETA(DisplayName = "Pow(X, 2)", Tooltip = "X^2"),
   Pow3                UMETA(DisplayName = "Pow(X, 3)", Tooltip = "X^3"),
   Pow4                UMETA(DisplayName = "Pow(X, 4)", Tooltip = "X^4"),
   SquareRoot          UMETA(DisplayName = "SquareRoot(X)"),
   CubeRoot            UMETA(DisplayName = "CubeRoot(X)"),
   FourthRoot          UMETA(DisplayName = "FourthRoot(X)"),
   Log2                UMETA(DisplayName = "Log2(X)"),
   Log10               UMETA(DisplayName = "Log10(X)"),

   Log2EaseIn          UMETA(DisplayName = "Log2 Ease-In"),
   Log2EaseOut         UMETA(DisplayName = "Log2 Ease-Out"),
   Log2EaseInOut       UMETA(DisplayName = "Log2 Ease-In-Out"),

   Log10EaseIn         UMETA(DisplayName = "Log10 Ease-In"),
   Log10EaseOut        UMETA(DisplayName = "Log10 Ease-Out"),
   Log10EaseInOut      UMETA(DisplayName = "Log10 Ease-In-Out"),

   SinEaseIn           UMETA(DisplayName = "Sin Ease-In"),
   SinEaseOut          UMETA(DisplayName = "Sin Ease-Out"),
   SinEaseInOut        UMETA(DisplayName = "Sin Ease-In-Out"),

   QuadraticEaseIn     UMETA(DisplayName = "Quadratic Ease-In"),
   QuadraticEaseOut    UMETA(DisplayName = "Quadratic Ease-Out"),
   QuadraticEaseInOut  UMETA(DisplayName = "Quadratic Ease-In-Out"),

   CubicEaseIn         UMETA(DisplayName = "Cubic Ease-In"),
   CubicEaseOut        UMETA(DisplayName = "Cubic Ease-Out"),
   CubicEaseInOut      UMETA(DisplayName = "Cubic Ease-In-Out"),

   CircularEaseIn      UMETA(DisplayName = "Circular Ease-In"),
   CircularEaseOut     UMETA(DisplayName = "Circular Ease-Out"),
   CircularEaseInOut   UMETA(DisplayName = "Circular Ease-In-Out"),
};

UCLASS()
class OSECORE_API UOSEMathFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   static FString GetComparisonMethodDescription(const EOSEComparisonMethod& comparisonMethod);
   
   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static bool CompareFloats(float lhs, float rhs, EOSEComparisonMethod comparisonMethod);

   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static bool CompareInts(int lhs, int rhs, EOSEComparisonMethod comparisonMethod);

   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static float GetClosestDistanceToCapsule(FVector location, FVector capsuleCenter, float capsuleRadius, float capsuleHalfHeight);

   /// Interpolates a value between 0 and 1, returning a value between 0 and 1 with the specified interpolation function applied.
   /// The alpha value is clamped to the range 0..1
   UFUNCTION(BlueprintPure, Category = "Math|OSE", DisplayName = "Interpolate (Normalized)")
   static double InterpolateNormalized(double alpha, EOSEInterpMode mode);

   /// Interpolates between A and B using the specified interpolation function
   /// The alpha value is clamped to the range 0..1
   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static double Interpolate(double a, double b = 1.0, double alpha = 0.0, EOSEInterpMode mode = EOSEInterpMode::Linear)
   {
      return FMath::Lerp(a, b, InterpolateNormalized(alpha, mode));
   }

   /// Interpolates a value using the sin function.
   /// Similar to FMath::Sin, but scaled such that an input range of 0..1 is mapped to an output range of 0..1
   template<typename T>
   static inline T InterpSin(T alpha)
   {
      static_assert(std::is_floating_point_v<T>, "Expected floating point type");
      static constexpr T twoPi = static_cast<T>(2) / static_cast<T>(UE_PI);
      return (FMath::Sin(((static_cast<T>(2) * FMath::Clamp<T>(alpha, 0, 1)) - static_cast<T>(1)) / twoPi) + static_cast<T>(1)) / static_cast<T>(2);
   }

   /// Interpolates between A and B using the sin function
   /// The alpha value is clamped to the range 0..1
   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static double InterpolateSin(double a, double b = 1.0, double alpha = 0.0)
   {
      return FMath::Lerp(a, b, InterpSin(alpha));
   }
};
