// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Math/OSEMathFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEMathFunctionLibrary)

namespace CompareHelper
{
   template <typename T>
   bool PerformComparison(const T& lhs, const T& rhs, EOSEComparisonMethod comparison)
   {
      switch (comparison)
      {
      case EOSEComparisonMethod::EqualTo:
         return lhs == rhs;
      case EOSEComparisonMethod::NotEqualTo:
         return lhs != rhs;
      case EOSEComparisonMethod::GreaterThanOrEqualTo:
         return lhs >= rhs;
      case EOSEComparisonMethod::LessThanOrEqualTo:
         return lhs <= rhs;
      case EOSEComparisonMethod::GreaterThan:
         return lhs > rhs;
      case EOSEComparisonMethod::LessThan:
         return lhs < rhs;
      }

      unimplemented();
      return false;
   }
}

namespace OSEMath
{
   /// Regular log function with a templated log base
   template<int32 LogBase, typename T>
   inline T Log(T value)
   {
      static_assert(std::is_floating_point_v<T>, "Expected floating point type");
      static_assert(LogBase >= 2, "Expected LogBase to be >= 2");
      static const T multiplier = 1.0f / FMath::Loge(static_cast<T>(LogBase));
      return FMath::Loge(value) * multiplier;
   }

   /// Scaled log function that fits the normal log curve such that it goes through both (0, 0) and (1, 1)
   /// The input value is clamped to the range 0..1
   template<int32 LogBase, typename T>
   inline T InterpLogOut(T alpha)
   {
      static_assert(std::is_floating_point_v<T>, "Expected floating point type");
      static_assert(LogBase >= 2, "Expected LogBase to be >= 2");
      if (alpha < 0)
      {
         return 0;
      }
      if (alpha > 1)
      {
         return 1;
      }
      return Log<LogBase>((static_cast<T>(LogBase) - static_cast<T>(1)) * alpha + static_cast<T>(1));
   }

   template<int32 LogBase, typename T>
   inline T InterpLogIn(T alpha)
   {
      return static_cast<T>(1) - InterpLogOut<LogBase, T>(static_cast<T>(1) - alpha);
   }

   template<int32 LogBase, typename T>
   inline T InterpLogInOut(T alpha)
   {
      return (alpha < static_cast<T>(0.5))
         ? (InterpLogIn<LogBase, T>(alpha * static_cast<T>(2)) * static_cast<T>(0.5))
         : (InterpLogOut<LogBase, T>(alpha * static_cast<T>(2) - static_cast<T>(1)) * static_cast<T>(0.5) + static_cast<T>(0.5));
   }

   template<typename T>
   inline T InterpPowIn(T alpha, T power)
   {
      static_assert(std::is_floating_point_v<T>, "Expected floating point type");
      return FMath::Pow(FMath::Clamp<T>(alpha, 0, 1), power);
   }

   template<typename T>
   inline T InterpPowOut(T alpha, T power)
   {
      return static_cast<T>(1) - FMath::Pow(static_cast<T>(1) - FMath::Clamp<T>(alpha, 0, 1), power);
   }

   template<typename T>
   inline T InterpPowInOut(T alpha, T power)
   {
      return (alpha < static_cast<T>(0.5))
         ? (InterpPowIn<T>(alpha * static_cast<T>(2), power) * static_cast<T>(0.5))
         : (InterpPowOut<T>(alpha * static_cast<T>(2) - static_cast<T>(1), power) * static_cast<T>(0.5) + static_cast<T>(0.5));
   }

   template<typename T>
   inline T InterpCircularIn(T alpha)
   {
      return static_cast<T>(-1) * (FMath::Sqrt(static_cast<T>(1) - alpha * alpha) - static_cast<T>(1));
   }

   template<typename T>
   inline T InterpCircularOut(T alpha)
   {
      alpha -= static_cast<T>(1);
      return FMath::Sqrt(static_cast<T>(1) - alpha * alpha);
   }

   template<typename T>
   inline T InterpCircularInOut(T alpha)
   {
      return (alpha < static_cast<T>(0.5))
         ? InterpCircularIn(alpha * static_cast<T>(2)) * static_cast<T>(0.5)
         : InterpCircularOut(alpha * static_cast<T>(2) - static_cast<T>(1)) * static_cast<T>(0.5) + static_cast<T>(0.5);
   }
}

FString UOSEMathFunctionLibrary::GetComparisonMethodDescription(const EOSEComparisonMethod& comparisonMethod)
{
   switch(comparisonMethod)
   {
   case EOSEComparisonMethod::EqualTo:
      return TEXT("Equal To");
   case EOSEComparisonMethod::NotEqualTo:
      return TEXT("Not Equal To");
   case EOSEComparisonMethod::GreaterThanOrEqualTo:
      return TEXT("Greater Than Or Equal To");
   case EOSEComparisonMethod::LessThanOrEqualTo:
      return TEXT("Less Than Or Equal To");
   case EOSEComparisonMethod::GreaterThan:
      return TEXT("Greater Than");
   case EOSEComparisonMethod::LessThan:
      return TEXT("Less Than");
   }
   return TEXT("Invalid");
}

bool UOSEMathFunctionLibrary::CompareFloats(float lhs, float rhs, EOSEComparisonMethod comparisonMethod)
{
   return CompareHelper::PerformComparison(lhs, rhs, comparisonMethod);
}

bool UOSEMathFunctionLibrary::CompareInts(int lhs, int rhs, EOSEComparisonMethod comparisonMethod)
{
   return CompareHelper::PerformComparison(lhs, rhs, comparisonMethod);
}

float UOSEMathFunctionLibrary::GetClosestDistanceToCapsule(FVector location, FVector capsuleCenter, float capsuleRadius, float capsuleHalfHeight)
{
   // find sphere in capsule closest to the target location
   const float excessHalfHeight = FMath::Max(0, capsuleHalfHeight - capsuleRadius);
   const double deltaZ = location.Z - capsuleCenter.Z;
   const float clampedDeltaZ = FMath::Clamp(static_cast<float>(deltaZ), -excessHalfHeight, excessHalfHeight);
   const FVector sphereCenter = capsuleCenter + FVector(0, 0, clampedDeltaZ);

   return FMath::Max(0, FVector::Distance(sphereCenter, location) - capsuleRadius);
}

double UOSEMathFunctionLibrary::InterpolateNormalized(double alpha, EOSEInterpMode mode)
{
   alpha = FMath::Clamp(alpha, 0.0, 1.0);
   switch (mode)
   {
   case EOSEInterpMode::Linear:
      return alpha;

   case EOSEInterpMode::Pow4:
      return OSEMath::InterpPowIn(alpha, 4.0);
   case EOSEInterpMode::SquareRoot:
      return OSEMath::InterpPowIn(alpha, 1.0 / 2.0);
   case EOSEInterpMode::CubeRoot:
      return OSEMath::InterpPowIn(alpha, 1.0 / 3.0);
   case EOSEInterpMode::FourthRoot:
      return OSEMath::InterpPowIn(alpha, 1.0 / 4.0);

   case EOSEInterpMode::Log2EaseIn:
      return OSEMath::InterpLogIn<2>(alpha);
   case EOSEInterpMode::Log2:
      // fallthrough
   case EOSEInterpMode::Log2EaseOut:
      return OSEMath::InterpLogOut<2>(alpha);
   case EOSEInterpMode::Log2EaseInOut:
      return OSEMath::InterpLogInOut<2>(alpha);

   case EOSEInterpMode::Log10EaseIn:
      return OSEMath::InterpLogIn<10>(alpha);
   case EOSEInterpMode::Log10:
      // fallthrough
   case EOSEInterpMode::Log10EaseOut:
      return OSEMath::InterpLogOut<10>(alpha);
   case EOSEInterpMode::Log10EaseInOut:
      return OSEMath::InterpLogInOut<10>(alpha);

   case EOSEInterpMode::SinEaseIn:
      return InterpSin(alpha * 0.5) * 2.0;
   case EOSEInterpMode::SinEaseOut:
      return (InterpSin((alpha * 0.5) + 0.5) - 0.5) * 2.0;
   case EOSEInterpMode::SinEaseInOut:
      // fallthrough
   case EOSEInterpMode::Sin:
      return InterpSin(alpha);

   case EOSEInterpMode::Pow2:
      // fallthrough
   case EOSEInterpMode::QuadraticEaseIn:
      return OSEMath::InterpPowIn(alpha, 2.0);
   case EOSEInterpMode::QuadraticEaseOut:
      return OSEMath::InterpPowOut(alpha, 2.0);
   case EOSEInterpMode::QuadraticEaseInOut:
      return OSEMath::InterpPowInOut(alpha, 2.0);

   case EOSEInterpMode::Pow3:
      // fallthrough
   case EOSEInterpMode::CubicEaseIn:
      return OSEMath::InterpPowIn(alpha, 3.0);
   case EOSEInterpMode::CubicEaseOut:
      return OSEMath::InterpPowOut(alpha, 3.0);
   case EOSEInterpMode::CubicEaseInOut:
      return OSEMath::InterpPowInOut(alpha, 3.0);

   case EOSEInterpMode::CircularEaseIn:
      return OSEMath::InterpCircularIn(alpha);
   case EOSEInterpMode::CircularEaseOut:
      return OSEMath::InterpCircularOut(alpha);
   case EOSEInterpMode::CircularEaseInOut:
      return OSEMath::InterpCircularInOut(alpha);

   default:
      ensureMsgf(false, TEXT("Invalid interp mode %i"), static_cast<int32>(mode));
      break;
   }

   return alpha;
}
