// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// Based on Xoshiro256**
// Xoshiro random website: https://prng.di.unimi.it/
// Original source for Xoshiro256**: https://prng.di.unimi.it/xoshiro256starstar.c
//
// Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
//
// To the extent possible under law, the author has dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
// IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "Math/TATXoshiroRandomStream.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATXoshiroRandomStream)


namespace Xoshiro
{
   FORCEINLINE uint64 Rotl(uint64 x, int k)
   {
      return (x << k) | (x >> (64 - k));
   }

   /// This is the base PRNG function for Xoshiro**256.
   /// It takes 256 bits of state (as an array of four uint64 values) and generates 64 random bits as a uint64 while mutating the input state.
   uint64 Next(uint64* s)
   {
      const uint64 result = Rotl(s[1] * 5, 7) * 9;

      const uint64 t = s[1] << 17;

      s[2] ^= s[0];
      s[3] ^= s[1];
      s[1] ^= s[2];
      s[0] ^= s[3];

      s[2] ^= t;

      s[3] = Rotl(s[3], 45);

      return result;
   }

   void Jump(uint64* s)
   {
      static constexpr int jumpStateSize = 4;
      static constexpr uint64 jumpValues[jumpStateSize] = {
         0x180ec6d33cfd0aba,
         0xd5a61266f0c9392c,
         0xa9582618e03fc9aa,
         0x39abdc4529b1661c,
      };

      uint64 s0 = 0;
      uint64 s1 = 0;
      uint64 s2 = 0;
      uint64 s3 = 0;
      for (int i = 0; i < jumpStateSize; i++)
      {
         for (int b = 0; b < 64; b++)
         {
            if (jumpValues[i] & UINT64_C(1) << b)
            {
               s0 ^= s[0];
               s1 ^= s[1];
               s2 ^= s[2];
               s3 ^= s[3];
            }
            Next(s);
         }
      }

      s[0] = s0;
      s[1] = s1;
      s[2] = s2;
      s[3] = s3;
   }

   void LongJump(uint64* s)
   {
      static constexpr int longJumpStateSize = 4;
      static constexpr uint64 longJumpState[longJumpStateSize] = {
         0x76e15d3efefdcbbf,
         0xc5004e441c522fb3,
         0x77710069854ee241,
         0x39109bb02acbe635,
      };

      uint64 s0 = 0;
      uint64 s1 = 0;
      uint64 s2 = 0;
      uint64 s3 = 0;
      for (int i = 0; i < longJumpStateSize; i++)
      {
         for (int b = 0; b < 64; b++)
         {
            if (longJumpState[i] & UINT64_C(1) << b)
            {
               s0 ^= s[0];
               s1 ^= s[1];
               s2 ^= s[2];
               s3 ^= s[3];
            }
            Next(s);
         }
      }

      s[0] = s0;
      s[1] = s1;
      s[2] = s2;
      s[3] = s3;
   }

   /// Very simple PRNG. Used for generating 256 bits of initial state for a Xoshiro random stream from a 64-bit base seed.
   uint64 SplitMix64(uint64& inOutState)
   {
      uint64 z = (inOutState += 0x9e3779b97f4a7c15);
      z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
      z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
      return z ^ (z >> 31);
   }

   /// Converts between numeric types by simply using the same bytes and reinterpreting them
   template<typename DstType, typename SrcType>
   FORCEINLINE DstType ReinterpretNumericValue(SrcType value)
   {
      static_assert(sizeof(DstType) == sizeof(SrcType), "ReinterpretNumericValue requires two types that are the same size");
      static_assert(std::is_floating_point_v<SrcType> || std::is_integral_v<SrcType>, "ReinterpretNumericValue requires a numeric source type");
      static_assert(std::is_floating_point_v<DstType> || std::is_integral_v<DstType>, "ReinterpretNumericValue requires a numeric dest type");

      if constexpr (std::is_same_v<DstType, SrcType>)
      {
         return value;
      }
      else
      {
         // Memcpy is the only type punning method in C++ that doesn't run into strict aliasing issues.
         // This generates nearly the same assembly code as a union or reinterpret_cast and is technically more "correct".
         // Note that the compiler will _not_ emit a function call here (even in debug builds) - it will inline the whole thing.
         DstType result;
         FMemory::Memcpy(&result, &value, sizeof(DstType));
         return result;
      }
   }

   /// Simple, very fast string hash function that's great for short strings (as would be typical for strings used for random seeds).
   /// Having the string hash be implemented in this file helps keep the whole thing portable if/when needed.
   template<typename T>
   T Fnv64StringHashUtf8(TConstArrayView<UTF8CHAR> bytes)
   {
      static constexpr uint64 fnv64Prime = 0x100000001b3ULL;
      static constexpr uint64 fnv64Init = 0xcbf29ce484222325ULL;

      uint64 hashResult = fnv64Init;

      const UTF8CHAR* ptr = &bytes[0];

      // one past the end of the buffer
      const UTF8CHAR* endOfBuffer = ptr + bytes.NumBytes();

      // FNV-1 hash each octet of the buffer
      while (ptr < endOfBuffer)
      {
         // multiply by the 64 bit FNV magic prime mod 2^64
         hashResult *= fnv64Prime;

         // xor the bottom with the current octet
         hashResult ^= static_cast<uint64>(*ptr++);
      }

      return ReinterpretNumericValue<T>(hashResult);
   }

   template<typename T>
   FORCEINLINE T Fnv64StringHash(FStringView str)
   {
      // I don't love converting to a UTF-8 string first, but TCHAR is 16-bits on Windows and 32-bits on Linux (and Fnv64 expects uint8-sized values anyway),
      // so this is required if we want to have string seeds behave consistently across platforms.
      const auto utf8String = StringCast<UTF8CHAR>(str.GetData(), str.Len());
      return Fnv64StringHashUtf8<T>(utf8String);
   }

   /// Just a standard lerp function. Only used here instead of FMath::Lerp to keep this trivially portable to other frameworks or languages
   template<typename ValueT, typename AlphaT>
   FORCEINLINE ValueT Lerp(const ValueT& a, const ValueT& b, const AlphaT& alpha)
   {
      return static_cast<ValueT>(a + alpha * (b - a));
   }
}

// static
int64 FTATXoshiroRandomStream::MakeSeed(int32 seedA, int32 seedB)
{
   // Mash the bits together to create a single 64-bit value
   return static_cast<int64>((static_cast<uint64>(seedA) << 32) | static_cast<uint32>(seedB));
}

// static
int64 FTATXoshiroRandomStream::MakeSeed(FStringView stringSeed)
{
   return Xoshiro::Fnv64StringHash<int64>(stringSeed);
}

void FTATXoshiroRandomStream::Reset(int64 seed)
{
   InitialSeed = seed;

   uint64 splitMixState = Xoshiro::ReinterpretNumericValue<uint64>(seed);
   State[0] = Xoshiro::SplitMix64(splitMixState);
   State[1] = Xoshiro::SplitMix64(splitMixState);
   State[2] = Xoshiro::SplitMix64(splitMixState);
   State[3] = Xoshiro::SplitMix64(splitMixState);
}

void FTATXoshiroRandomStream::Reset(ETATXoshiroInit init)
{
   switch (init)
   {
   case ETATXoshiroInit::Zero:
      InitialSeed = 0;
      for (int32 i = 0; i < StateSize; i++)
      {
         State[i] = 0;
      }
      break;
   case ETATXoshiroInit::RandomSeed:
      Reset(MakeSeed(FMath::Rand32(), FMath::Rand32()));
      break;
   default:
      checkNoEntry();
   }
}

void FTATXoshiroRandomStream::Advance()
{
   Xoshiro::Next(State);
}

uint64 FTATXoshiroRandomStream::NextUint64()
{
   return Xoshiro::Next(State);
}

int64 FTATXoshiroRandomStream::NextInt64()
{
   return Xoshiro::ReinterpretNumericValue<int64>(Xoshiro::Next(State));
}

int64 FTATXoshiroRandomStream::NextInt64InRange(int64 minValue, int64 maxValue)
{
   const int64 rangeSize = (maxValue - minValue) + 1;
   return minValue + ((rangeSize > 0) ? FMath::TruncToInt64(NextDouble() * static_cast<double>(rangeSize)) : 0);
}

uint32 FTATXoshiroRandomStream::NextUint32()
{
   // Note that there's no practical difference between extracting the upper or lower 32 bits.
   // We could trivially write a function that uses both the upper and lower bits to generate a pair of 32-bit values if that's ever needed in the future.

   const uint64 val = Xoshiro::Next(State);
   // Extract just the lower 32 bits.
   return static_cast<uint32>(val & 0xffffffff);
}

int32 FTATXoshiroRandomStream::NextInt32()
{
   return Xoshiro::ReinterpretNumericValue<int32>(NextUint32());
}

int32 FTATXoshiroRandomStream::NextInt32InRange(int32 minValue, int32 maxValue)
{
   const int32 rangeSize = (maxValue - minValue) + 1;
   return minValue + ((rangeSize > 0) ? FMath::TruncToInt32(NextFloat() * static_cast<float>(rangeSize)) : 0);
}

float FTATXoshiroRandomStream::NextFloat()
{
   // See the comment in the NextDouble() implementation. Same trick here, just different number of significant binary digits.
   const uint32 val = NextUint32();
   return (val >> 8) * 0x1.0p-24f;
}

float FTATXoshiroRandomStream::NextFloatInRange(float minVal, float maxVal)
{
   return FMath::Lerp(minVal, maxVal, NextFloat());
}

double FTATXoshiroRandomStream::NextDouble()
{
   // Quoting from the Xoshiro website:
   // | Generating uniform doubles in the unit interval
   // | A standard double (64-bit) floating-point number in IEEE floating point format has 52 bits of significand, plus an implicit bit at the left of the significand.
   // | Thus, the representation can actually store numbers with 53 significant binary digits.
   // | Because of this fact, in C99 a 64-bit unsigned integer x should be converted to a 64-bit double using the expression
   // |   (x >> 11) * 0x1.0p-53

   const uint64 val = Xoshiro::Next(State);
   return (val >> 11) * 0x1.0p-53;
}

double FTATXoshiroRandomStream::NextDoubleInRange(double minVal, double maxVal)
{
   return FMath::Lerp(minVal, maxVal, NextDouble());
}

FTATXoshiroRandomStream FTATXoshiroRandomStream::Jump() const
{
   FTATXoshiroRandomStream result = *this;
   Xoshiro::Jump(result.State);
   return result;
}

FTATXoshiroRandomStream FTATXoshiroRandomStream::LongJump() const
{
   FTATXoshiroRandomStream result = *this;
   Xoshiro::LongJump(result.State);
   return result;
}

namespace Xoshiro
{
   template<typename T>
   void RunTest(const TCHAR* label, T minValue, T maxValue, TFunctionRef<T()> cb)
   {
      UE_LOG(LogTemp, Log, TEXT("Xoshiro Random Test: %s"), label);
      for (int32 i = 0; i < 8; i++)
      {
         const T value = cb();
         const FString valueString = FString::Format(TEXT("{0}"), { value });
         if (value >= minValue && value <= maxValue)
         {
            UE_LOG(LogTemp, Log, TEXT("    [%i] %s"), i, *valueString);
         }
         else
         {
            UE_LOG(LogTemp, Error, TEXT("    [%i] %s (OUT OF RANGE)"), i, *valueString);
         }
      }
   }
}

#if 0
// static
void UTATXoshiroRandomLibrary::XoshiroRandom_RunTests(FTATXoshiroRandomStream& stream)
{
   UE_LOG(LogTemp, Log, TEXT("Xoshiro Random Test Sequence"));
   UE_LOG(LogTemp, Log, TEXT("Seed = %lli"), stream.InitialSeed);
   Xoshiro::RunTest<uint64>(TEXT("uint64"), 0, UINT64_MAX, [&stream]() { return stream.NextUint64(); });
   Xoshiro::RunTest<int64>(TEXT("int64"), INT64_MIN, INT64_MAX, [&stream]() { return stream.NextInt64(); });
   Xoshiro::RunTest<uint32>(TEXT("uint32"), 0, UINT32_MAX, [&stream]() { return stream.NextUint32(); });
   Xoshiro::RunTest<int32>(TEXT("int32"), INT32_MIN, INT32_MAX, [&stream]() { return stream.NextInt32(); });
   Xoshiro::RunTest<float>(TEXT("float"), FLT_MIN, FLT_MAX, [&stream]() { return stream.NextFloat(); });
   Xoshiro::RunTest<double>(TEXT("double"), DBL_MIN, DBL_MAX, [&stream]() { return stream.NextDouble(); });
   Xoshiro::RunTest<int64>(TEXT("int64 in range"), -1000, 1000, [&stream]() { return stream.NextInt64InRange(-1000, 1000); });
   Xoshiro::RunTest<int32>(TEXT("int32 in range"), -1000, 1000, [&stream]() { return stream.NextInt32InRange(-1000, 1000); });
   Xoshiro::RunTest<float>(TEXT("float in range"), -1000, 1000, [&stream]() { return stream.NextFloatInRange(-1000, 1000); });
   Xoshiro::RunTest<double>(TEXT("double in range"), -1000, 1000, [&stream]() { return stream.NextDoubleInRange(-1000, 1000); });
}
#endif
