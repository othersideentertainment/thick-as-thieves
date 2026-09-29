// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATXoshiroRandomStream.generated.h"

enum class ETATXoshiroInit : uint8
{
   Zero = 0,
   RandomSeed,
};

// Not exposing the blueprint bindings for now. We can add these later if/when needed.
// USTRUCT(BlueprintType, Meta = (HasNativeMake="/Script/TAT.TATXoshiroRandomLibrary.XoshiroRandom_MakeWithSeed", HasNativeBreak="/Script/TAT.TATXoshiroRandomLibrary.XoshiroRandom_GetInitialSeed"))

/// Zero-dependency, extremely fast, portable PRNG implementation
USTRUCT()
struct TAT_API FTATXoshiroRandomStream
{
   GENERATED_BODY()

   static constexpr int32 StateSize = 4;

   UPROPERTY()
   int64 InitialSeed = 0;

   UPROPERTY()
   uint64 State[StateSize] = { 0, 0, 0, 0 };

   FTATXoshiroRandomStream() = default;
   explicit FTATXoshiroRandomStream(int64 seed) { Reset(seed); }
   explicit FTATXoshiroRandomStream(ETATXoshiroInit init) { Reset(init); }
   explicit FTATXoshiroRandomStream(FStringView stringSeed) { Reset(MakeSeed(stringSeed)); }

   FTATXoshiroRandomStream(const FTATXoshiroRandomStream&) = default;
   FTATXoshiroRandomStream& operator=(const FTATXoshiroRandomStream&) = default;

   /// Converts two 32-bit signed integers to a uint64 value suitable for use as a seed.
   /// This version is useful for combining the output of two calls to a PRNG that only outputs 32-bit integers (like FMath::Rand32)
   static int64 MakeSeed(int32 seedA, int32 seedB);

   /// Uses the FNV64-a hash algorithm to generate a uint64 value suitable for use as a seed
   static int64 MakeSeed(FStringView stringSeed);

   /// Resets all state to the initial seed
   void Reset() { Reset(InitialSeed); }

   /// Resets all state to one generated from a specific seed
   void Reset(int64 seed);

   /// Resets all state
   void Reset(ETATXoshiroInit init);

   /// Resets all state to one generated from a specific seed
   FORCEINLINE void Reset(FStringView stringSeed) { Reset(MakeSeed(stringSeed)); }

   /// Checks if the state is initialized
   FORCEINLINE bool IsValid() const { return !(State[0] == 0 && State[1] == 0 && State[2] == 0 && State[3] == 0); }

   FORCEINLINE explicit operator bool() const { return IsValid(); }

public:
   /// Advances the random number generator state by one iteration.
   /// Equivalent to calling NextUint64() and discarding the result.
   void Advance();

   /// Returns a random unsigned 64-bit integer in the range [0..UINT64_MAX]
   uint64 NextUint64();

   /// Returns a random signed 64-bit integer in the range [INT64_MIN..INT64_MAX]
   int64 NextInt64();

   /// Returns a random signed 64-bit integer in the range [minValue..maxValue]
   int64 NextInt64InRange(int64 minValue, int64 maxValue);

   /// Returns a random unsigned 32-bit integer in the range [0..UINT32_MAX]
   uint32 NextUint32();

   /// Returns a random signed 32-bit integer in the range [INT32_MIN..INT32_MAX]
   int32 NextInt32();

   /// Returns a random signed 32-bit integer in the range [minValue..maxValue]
   int32 NextInt32InRange(int32 minValue, int32 maxValue);

   /// Returns a single-precision floating point value in the range [0..1]
   float NextFloat();

   /// Returns a single-precision floating point value in the specified range
   float NextFloatInRange(float minVal, float maxVal);

   /// Returns a double-precision floating point value in the range [0..1]
   double NextDouble();

   /// Returns a double-precision floating point value in the specified range
   double NextDoubleInRange(double minVal, double maxVal);

   /// This is the jump function for the generator. It is equivalent to 2^128 calls to Advance();
   /// it can be used to generate 2^128 non-overlapping subsequences for parallel computations.
   ///
   /// Note that this function produces a new random stream but does not mutate its own state.
   /// If you want to call this multiple times, you will need to call Advance() between calls.
   FTATXoshiroRandomStream Jump() const;

   /// This is the long-jump function for the generator. It is equivalent to 2^192 calls to Advance();
   /// it can be used to generate 2^64 starting points, from each of which Jump() will generate 2^64
   /// non-overlapping subsequences for parallel distributed computations.
   ///
   /// Note that this function produces a new random stream but does not mutate its own state.
   /// If you want to call this multiple times, you will need to call Advance() between calls.
   FTATXoshiroRandomStream LongJump() const;
};


// Not exposing the blueprint bindings for now. We can add these later if/when needed.
#if 0

UCLASS()
class UTATXoshiroRandomLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   /// Constructs a Xoshiro seed by combining the bits from two integer values
   UFUNCTION(BlueprintPure, DisplayName = "Make Seed From Integer Pair (Xoshiro)", Category = "Xoshiro Random")
   static int64 XoshiroRandom_MakeSeedFromInt32Pair(int32 a, int32 b) { return FTATXoshiroRandomStream::MakeSeed(a, b); }

   /// Constructs a Xoshiro seed by hashing a string
   UFUNCTION(BlueprintPure, DisplayName = "Make Seed From String (Xoshiro)", Category = "Xoshiro Random")
   static int64 XoshiroRandom_MakeSeedFromString(const FString& stringSeed) { return FTATXoshiroRandomStream::MakeSeed(stringSeed); }

   /// Creates a new random stream with a seed
   UFUNCTION(BlueprintPure, DisplayName = "Make Xoshiro Random Stream", Category = "Xoshiro Random")
   static FTATXoshiroRandomStream XoshiroRandom_MakeWithSeed(int64 seed) { return FTATXoshiroRandomStream(seed); }

   /// Creates a new random stream with a random seed
   UFUNCTION(BlueprintPure, DisplayName = "Make Xoshiro Random Stream (Random Seed)", Category = "Xoshiro Random")
   static FTATXoshiroRandomStream XoshiroRandom_MakeWithRandomSeed() { return FTATXoshiroRandomStream(ETATXoshiroInit::RandomSeed); }

   /// Gets the initial seed for a Xoshiro random stream
   UFUNCTION(BlueprintPure, DisplayName = "Get Initial Seed (Xoshiro)", Category = "Xoshiro Random")
   static int64 XoshiroRandom_GetInitialSeed(const FTATXoshiroRandomStream& stream) { return stream.InitialSeed; }

   /// Resets a random stream with a new seed
   UFUNCTION(BlueprintCallable, DisplayName = "Reset With New Seed (Xoshiro)", Category = "Xoshiro Random")
   static void XoshiroRandom_ResetWithNewSeed(UPARAM(ref) FTATXoshiroRandomStream& stream, int64 seed) { stream.Reset(seed); }

   /// Resets a random stream with its original seed
   UFUNCTION(BlueprintCallable, DisplayName = "Reset With Initial Seed (Xoshiro)", Category = "Xoshiro Random")
   static void XoshiroRandom_Reset(UPARAM(ref) FTATXoshiroRandomStream& stream) { stream.Reset(); }

   /// Advances the stream exactly once. This is equivalent to generating a single random integer and discarding the result.
   /// Useful for keeping procedural generation deterministic.
   UFUNCTION(BlueprintCallable, DisplayName = "Advance (Xoshiro)", Category = "Xoshiro Random")
   static void XoshiroRandom_Advance(UPARAM(ref) FTATXoshiroRandomStream& stream) { stream.Advance(); }

   /// Generates a random 32-bit integer between minValue and maxValue (inclusive)
   UFUNCTION(BlueprintCallable, DisplayName = "Random Integer in Range (Xoshiro)", Category = "Xoshiro Random")
   static int32 XoshiroRandom_NextInt32InRange(UPARAM(ref) FTATXoshiroRandomStream& stream, int32 minValue, int32 maxValue) { return stream.NextInt32InRange(minValue, maxValue); }

   /// Generates a random 64-bit integer between minValue and maxValue (inclusive)
   UFUNCTION(BlueprintCallable, DisplayName = "Random Integer64 in Range (Xoshiro)", Category = "Xoshiro Random")
   static int64 XoshiroRandom_NextInt64InRange(UPARAM(ref) FTATXoshiroRandomStream& stream, int64 minValue, int64 maxValue) { return stream.NextInt64InRange(minValue, maxValue); }

   /// Generates a random floating point value (double-precision) between 0.0 and 1.0
   UFUNCTION(BlueprintCallable, DisplayName = "Random Float (Xoshiro)", Category = "Xoshiro Random")
   static double XoshiroRandom_NextDouble(UPARAM(ref) FTATXoshiroRandomStream& stream) { return stream.NextDouble(); }

   /// Generates a random floating point value (double-precision) between minValue and maxValue (inclusive)
   UFUNCTION(BlueprintCallable, DisplayName = "Random Float in Range (Xoshiro)", Category = "Xoshiro Random")
   static double XoshiroRandom_NextDoubleInRange(UPARAM(ref) FTATXoshiroRandomStream& stream, double minValue, double maxValue) { return stream.NextDoubleInRange(minValue, maxValue); }

   /// Returns a new random stream based on this one, without altering the state of this stream.
   ///
   /// This is equivalent to making a copy of the stream, then calling Advance 2^128 times. It can be used to generate non-overlapping
   /// subsequences to keep procedural generation deterministic.
   ///
   /// For example, you might pass a random stream to a function to compute a variable number of computations.
   /// If you call Jump and pass the resulting stream to the function, you'll know that the next random number you generate will always be the same
   /// regardless of how many random values that function generated.
   ///
   /// Note that this function produces a new random stream but does not mutate its own state.
   /// If you want to call this multiple times, you will need to call Advance() between calls.
   UFUNCTION(BlueprintCallable, DisplayName = "Jump (Xoshiro)", Category = "Xoshiro Random")
   static FTATXoshiroRandomStream XoshiroRandom_Jump(const FTATXoshiroRandomStream& stream) { return stream.Jump(); }

   UFUNCTION(BlueprintCallable, DisplayName = "Run Xoshiro Random Tests", Category = "Xoshiro Random")
   static void XoshiroRandom_RunTests(UPARAM(ref) FTATXoshiroRandomStream& stream);

};

#endif // #if 0
