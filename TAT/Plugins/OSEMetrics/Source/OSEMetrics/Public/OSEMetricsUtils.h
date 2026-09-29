// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Json.h"
#include "Misc/DefaultValueHelper.h"

// std
#include <type_traits>

/// Callback that takes a string view representing the portion of the path after the endpoint and returns
/// a JSON object that should be serialized and sent as metrics data.
using FOSEMetricsValueCallback = TFunction<TSharedPtr<FJsonObject>(const TMap<FString, FString>& queryParams)>;

DECLARE_STATS_GROUP(TEXT("OSEMetrics"), STATGROUP_OSEMetrics, STATCAT_Advanced);

namespace OSEMetricsUtils
{

template<typename T>
static TSharedPtr<FJsonValue> EnumToJsonString(T value)
{
   return MakeShared<FJsonValueString>(StaticEnum<T>()->GetNameStringByValue(static_cast<int64>(value)));
}
   
/// Converts an integer, float, bool, or FString to an FJsonObject
template<typename T>
inline TSharedPtr<FJsonObject> MakeSimpleJsonObject(T value, const TCHAR* key = TEXT("value"))
{
   using ValueType = std::remove_cvref_t<T>;
   TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
   if constexpr (std::is_null_pointer_v<ValueType>)
   {
      result->Values.Add(key, MakeShared<FJsonValueNull>());
   }
   else if constexpr (std::is_same_v<ValueType, bool>)
   {
      result->Values.Add(key, MakeShared<FJsonValueBoolean>(value));
   }
   else if constexpr (std::is_integral_v<ValueType> || std::is_floating_point_v<ValueType>)
   {
      result->Values.Add(key, MakeShared<FJsonValueNumber>(static_cast<double>(value)));
   }
   else if constexpr (std::is_same_v<ValueType, FString>)
   {
      result->Values.Add(key, MakeShared<FJsonValueString>(Forward<ValueType>(value)));
   }
   else
   {
      ([]<bool Fail = true>{ static_assert(!Fail, "Invalid type for MakeSimpleJsonObject"); })();
   }
   return result;
}

// Internal helper function to wrap FDefaultValueHelper functions into a single templated version
template<typename T>
TOptional<T> TryParseString(const FString& value)
{
#define IF_CONSTEXPR_OSE_TRY_PARSE_STRING(CPP_TYPE, PARSE_FUNC) \
   if constexpr (std::is_same_v<T, CPP_TYPE>) \
   { \
      CPP_TYPE result{}; \
      if (PARSE_FUNC(value, result)) \
      { \
         return result; \
      } \
   }

   IF_CONSTEXPR_OSE_TRY_PARSE_STRING(int32, FDefaultValueHelper::ParseInt)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(int64, FDefaultValueHelper::ParseInt64)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(float, FDefaultValueHelper::ParseFloat)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(double, FDefaultValueHelper::ParseDouble)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FVector3f, FDefaultValueHelper::ParseVector)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FVector3d, FDefaultValueHelper::ParseVector)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FVector2f, FDefaultValueHelper::ParseVector2D)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FVector2d, FDefaultValueHelper::ParseVector2D)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FVector4f, FDefaultValueHelper::ParseVector4)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FVector4d, FDefaultValueHelper::ParseVector4)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FRotator3f, FDefaultValueHelper::ParseRotator)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FRotator3d, FDefaultValueHelper::ParseRotator)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FLinearColor, FDefaultValueHelper::ParseLinearColor)
   else IF_CONSTEXPR_OSE_TRY_PARSE_STRING(FColor, FDefaultValueHelper::ParseColor)
   else if constexpr (std::is_same_v<T, FString>) { return value; }
   else if constexpr (std::is_same_v<T, FName>) { return (value.Len() > 0) ? FName(value) : NAME_None; }
   else
   {
      // Fail at compile-time for unhandled types
      ([]<bool Fail = true>{ static_assert(!Fail, "Unsupported type for TryParseString"); })();
   }

#undef IF_CONSTEXPR_OSE_TRY_PARSE_STRING

   return NullOpt;
}

using FQueryParamsKeyFuncs = TMap<FString, FString>::KeyFuncsType;

template<typename T>
TOptional<T> TryParseQueryParamValue(const TMap<FString, FString>& queryParams, FStringView key)
{
   if (const FString* value = queryParams.FindByHash(FQueryParamsKeyFuncs::GetKeyHash(key), key))
   {
      return TryParseString<T>(*value);
   }
   return NullOpt;
}

inline const FString* TryGetQueryParamValue(const TMap<FString, FString>& queryParams, FStringView key)
{
   return queryParams.FindByHash(FQueryParamsKeyFuncs::GetKeyHash(key), key);
}

/// The min, max, sum, count, and timestamp range for a rolling set of values
template<typename T>
struct TRollingValueStats
{
   using ValueType = T;
   static_assert(std::is_integral_v<ValueType> || std::is_floating_point_v<ValueType>, "TRollingValueStats requires a numeric type");

   // This is double if ValueType is a 64-bit type, otherwise float
   using FloatType = std::conditional_t<sizeof(ValueType) >= sizeof(double), double, float>;

   ValueType ValueMin = std::numeric_limits<ValueType>::max();
   ValueType ValueMax = std::numeric_limits<ValueType>::min();
   ValueType ValueSum = 0;
   int32 ValueCount = 0;
   double TimestampMin = std::numeric_limits<double>::max();
   double TimestampMax = std::numeric_limits<double>::min();

   /// Checks if this has at least one value
   FORCEINLINE bool IsValid() const
   {
      return ValueCount > 0;
   }

   FORCEINLINE explicit operator bool() const
   {
      return IsValid();
   }

   void Add(ValueType value, double timestamp)
   {
      if (value < ValueMin)
      {
         ValueMin = value;
      }
      if (value > ValueMax)
      {
         ValueMax = value;
      }
      ValueSum += value;
      ValueCount += 1;
      if (timestamp < TimestampMin)
      {
         TimestampMin = timestamp;
      }
      if (timestamp > TimestampMax)
      {
         TimestampMax = timestamp;
      }
   }

   /// Gets the average value for this time slice
   FORCEINLINE TOptional<FloatType> GetAverageValue() const
   {
      if (ValueCount > 0)
      {
         return static_cast<FloatType>(ValueSum) / static_cast<FloatType>(ValueCount);
      }
      return NullOpt;
   }

   TSharedPtr<FJsonObject> ToJsonObject() const
   {
      TSharedPtr<FJsonObject> stats = MakeShared<FJsonObject>();
      stats->Values.Add(TEXT("ValueMin"), MakeShared<FJsonValueNumber>(IsValid() ? ValueMin : 0));
      stats->Values.Add(TEXT("ValueMax"), MakeShared<FJsonValueNumber>(IsValid() ? ValueMax : 0));
      // Not sure the sum value is useful here. In any case you can get it later by doing ValueAvg * ValueCount
      //stats->Values.Add(TEXT("ValueSum"), MakeShared<FJsonValueNumber>(stats.ValueSum));
      stats->Values.Add(TEXT("ValueAvg"), MakeShared<FJsonValueNumber>(GetAverageValue().Get(0.0f)));
      stats->Values.Add(TEXT("ValueCount"), MakeShared<FJsonValueNumber>(ValueCount));
      stats->Values.Add(TEXT("TimestampMin"), MakeShared<FJsonValueNumber>(IsValid() ? TimestampMin : 0));
      stats->Values.Add(TEXT("TimestampMax"), MakeShared<FJsonValueNumber>(IsValid() ? TimestampMax : 0));
      return stats;
   }
};

/// Helper to compute time-based rolling averages and other related stats
template<typename T>
class TRollingValue
{
   using ValueType = T;
   static_assert(std::is_integral_v<ValueType> || std::is_floating_point_v<ValueType>, "TRollingValue requires a numeric type");

   struct FValueAndTimestamp
   {
      ValueType Value = 0;
      double Timestamp = 0;

      FORCEINLINE bool IsRecent(double currentTime, double maxValueAge) const
      {
         return Timestamp > 0 && currentTime - Timestamp <= maxValueAge;
      }
   };

   static constexpr double kMinValueAgeSeconds = 0.000001;

   double _maxValueAgeSeconds = 1.0;
   int32 _maxNumValuesPerSecond = -1;
   int32 _nextValueIndex = -1;
   TArray<FValueAndTimestamp> _values;

   FORCEINLINE double _GetCurrentTime() const
   {
      return FPlatformTime::Seconds();
   }

   /// Returns an array index we can replace with a new value, or INDEX_NONE if no such existing array index exists.
   int32 _GetNextValueIndex(double currentTime)
   {
      if (_maxNumValuesPerSecond > 0)
      {
         check(_values.IsValidIndex(_nextValueIndex));
         const int32 result = _nextValueIndex;
         _nextValueIndex = (_nextValueIndex + 1) % _values.Num();
         return result;
      }
      else
      {
         for (int32 i = 0; i < _values.Num(); i++)
         {
            if (!_values[i].IsRecent(currentTime, _maxValueAgeSeconds))
            {
               return i;
            }
         }
      }
      return INDEX_NONE;
   }

public:
   TRollingValue() = default;

   /// If you specify max values per second, a fixed-sized buffer will be used.
   /// This is faster, but only useful if you generally know how many values per second you're expecting.
   explicit TRollingValue(double maxValueAgeSeconds, int32 maxNumValuesPerSecond = -1)
   {
      Reset(maxValueAgeSeconds, maxNumValuesPerSecond);
   }

   double GetMaxValueAgeSeconds() const
   {
      return _maxValueAgeSeconds;
   }

   void Add(ValueType value, TOptional<double> valueTimestamp = NullOpt)
   {
      const double timestamp = valueTimestamp ? *valueTimestamp : _GetCurrentTime();
      const int32 existingIndex = _GetNextValueIndex(timestamp);
      if (existingIndex == -1)
      {
         _values.Emplace(value, timestamp);
      }
      else
      {
         FValueAndTimestamp& existingValue = _values[existingIndex];
         existingValue.Value = value;
         existingValue.Timestamp = timestamp;
      }
   }

   /// Computes the stats for the time starting at currentTime (or FPlatformTime::Seconds if not specified) and including all values
   /// in the past timespanSeconds, or all recent values if not specified.
   TRollingValueStats<ValueType> GetStats(TOptional<double> currentTime = NullOpt, TOptional<double> timespanSeconds = NullOpt) const
   {
      const double now = currentTime ? *currentTime : _GetCurrentTime();
      const double maxValueAge = FMath::Clamp(timespanSeconds.Get(_maxValueAgeSeconds), kMinValueAgeSeconds, _maxValueAgeSeconds);
      TRollingValueStats<ValueType> stats{};
      for (const FValueAndTimestamp& timedValue : _values)
      {
         if (timedValue.IsRecent(now, maxValueAge))
         {
            stats.Add(timedValue.Value, timedValue.Timestamp);
         }
      }
      return stats;
   }

   void Reserve(int32 numValues)
   {
      _values.Reserve(numValues);
   }

   /// Resets the value array to either empty, or (if configured with max number of values per second) a fixed size array based on that value.
   void Reset(TOptional<double> newMaxValueAgeSeconds = NullOpt, TOptional<int32> newMaxNumValuesPerSecond = NullOpt)
   {
      if (newMaxValueAgeSeconds)
      {
         _maxValueAgeSeconds = FMath::Max(kMinValueAgeSeconds, *newMaxValueAgeSeconds);
      }
      if (newMaxNumValuesPerSecond)
      {
         _maxNumValuesPerSecond = *newMaxNumValuesPerSecond;
      }

      if (_maxNumValuesPerSecond <= 0)
      {
         _maxNumValuesPerSecond = -1;
         _values.Reset();
      }
      else
      {
         const int32 newSize = FMath::Max(2, FMath::RoundHalfFromZero(_maxValueAgeSeconds * _maxNumValuesPerSecond));
         _values.Empty(newSize);
         _values.SetNumZeroed(newSize);
         _nextValueIndex = 0;
      }
   }
};

} // namespace OSEMetricsUtils
