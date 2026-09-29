// (c) 2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// ose
#include "OSECommon.h"
#include "OSECompression.h"

// ue4
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"

#if WITH_DEV_AUTOMATION_TESTS

template <typename T>
static T RoundTripSerialize(T value, TArray<uint8>& buffer)
{
   bool okay;
   buffer.Reset();
   FMemoryWriter writer(buffer);

   value.NetSerialize(writer, nullptr, okay);
   
   T result;
   FMemoryReader reader(buffer);
   result.NetSerialize(reader, nullptr, okay);
   return result;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOSECommon_PreQuantizeTest, "OSE.OSECommon.PreQuantize", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FOSECommon_PreQuantizeTest::RunTest(const FString& Parameters)
{

#if UE_BUILD_DEBUG
   // DebugGame Editor compiles our game library in Debug but the engine in Development so comparing
   // these values isn't precise enough to test against; so loosen up our threshold.
   static const float kThreshold = 0.01;
#else
   static const float kThreshold = KINDA_SMALL_NUMBER;
#endif 

#define TEST_PREQUANTIZE_VECTOR(value) do { auto v = value; TestEqual(TEXT(#value), (FVector)UOSECommon::PreQuantize(v), (FVector)RoundTripSerialize(v, buffer), kThreshold); } while(0)
   TArray<uint8> buffer;

   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize(0, 0, 0));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize(1037.34, 123.233444, 3772.92527));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize(-1037.34, -123.233444, 73772.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize(-1037.34, -123.233444, 2232373772.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize(-1037.34, -123.233444, -2232373772.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize(-4503599627370497.363627, -4503599627370497.363627, 4503599627370497.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize(-4503599627370497.363627, -4503599627370497.363627, -4503599627370497.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize(-1037.34, -123.233444, 4.6116860184273888e+18));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize(-1037.34, -123.233444, -4.6116860184273888e+18));

   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize10(0, 0, 0));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize10(1037.34, 123.233444, 3772.92527));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize10(-1037.34, -123.233444, 73772.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize10(-1037.34, -123.233444, 2232373772.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize10(-1037.34, -123.233444, -2232373772.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize10(-4503599627370497.363627, -4503599627370497.363627, 4503599627370497.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize10(-4503599627370497.363627, -4503599627370497.363627, -4503599627370497.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize10(-1037.34, -123.233444, 4.6116860184273888e+18));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize10(-1037.34, -123.233444, -4.6116860184273888e+18));

   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize100(0, 0, 0));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize100(1037.34, 123.233444, 3772.92527));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize100(-1037.34, -123.233444, 73772.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize100(-1037.34, -123.233444, 232373772.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize100(-1037.34, -123.233444, -232373772.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize100(-4503599627370497.363627, -4503599627370497.363627, 4503599627370497.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize100(-4503599627370497.363627, -4503599627370497.363627, -4503599627370497.363627));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize100(-1037.34, -123.233444, 4.6116860184273888e+18));
   TEST_PREQUANTIZE_VECTOR(FVector_NetQuantize100(-1037.34, -123.233444, -4.6116860184273888e+18));

#undef TEST_PREQUANTIZE_VECTOR
   return true;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOSECommon_CompressionTestFLE, "OSE.OSECommon.CompressionTestFLE", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

template <typename T_SaveCompressionStruct, typename T_BaseType>
bool TestCompressionFLE(const TArray<T_BaseType>& sourceArray, uint32& compressedSize)
{
   TArray< T_SaveCompressionStruct> compressedData;
   OSECompression::CompressArray_RLE(sourceArray, compressedData);

   compressedSize = compressedData.Num() * sizeof(T_SaveCompressionStruct);

   TArray<T_BaseType> fromCompression;
   OSECompression::UncompressArray_RLE(fromCompression, compressedData);

   bool isSame = sourceArray == fromCompression;

   return isSame;
}

bool FOSECommon_CompressionTestFLE::RunTest(const FString& Parameters)
{
   uint32 compressedSize = 0;
   uint32 uncompressedSize = 0;
   uint32 uniformTestingAmount = 9000;
   {
      //uniform uint8 testing
      TArray<uint8> testArray;
      testArray.Init(123, uniformTestingAmount);

      AddInfo(FString::Printf(TEXT("RLE Compression uint8: Amount entries: %u"), uniformTestingAmount));

      uncompressedSize = testArray.Num() * sizeof(uint8);
      bool didPass = TestCompressionFLE<FOSESaveCompressedRLEUintDataEntry>(testArray, compressedSize);
      if (!didPass)
      {
         AddError(FString::Printf(TEXT("RLE Compression failed - uint8")));
      }
      else
      {
         AddInfo(FString::Printf(TEXT("RLE Compression uint8: Uncompressed size %u bytes; compressed size %u bytes"), uncompressedSize, compressedSize));
      }

      //random(ish) uint8 testing
      testArray.Reset();
      FRandomStream randomStream("Testing");
      uint32 amount = 4096 * 4096;
      uint32 amountAtATime = 8;

      AddInfo(FString::Printf(TEXT("RLE Compression uint8 random: Amount entries: %u; amount of same in a row: %u"), amount, amountAtATime));

      testArray.Reserve(amount);
      uncompressedSize = amount * sizeof(uint8);
      while(amount != 0)
      {
         uint32 amountToDecrease = FMath::Min(amount, amountAtATime);
         amount -= amountToDecrease;
         uint8 value = randomStream.RandRange(0, 255);
         while (amountToDecrease != 0)
         {
            testArray.Add(value);
            amountToDecrease--;
         }
      }
      
      didPass = TestCompressionFLE<FOSESaveCompressedRLEUintDataEntry>(testArray, compressedSize);
      if (!didPass)
      {
         AddError(FString::Printf(TEXT("RLE Compression failed - uint8 random")));
      }
      else
      {
         AddInfo(FString::Printf(TEXT("RLE Compression uint8 random: Uncompressed size %u bytes; compressed size %u bytes"), uncompressedSize, compressedSize));
      }


   }

   {
      //uniform color testing
      TArray<FColor> testArray;
      testArray.Init(FColor::Red, uniformTestingAmount);
      uncompressedSize = testArray.Num() * sizeof(FColor);

      AddInfo(FString::Printf(TEXT("RLE Compression Color: Amount entries: %u"), uniformTestingAmount));

      bool didPass = TestCompressionFLE<FOSESaveCompressedRLEColorDataEntry>(testArray, compressedSize);
      if (!didPass)
      {
         AddError(FString::Printf(TEXT("RLE Compression failed - color")));
      }
      else
      {
         AddInfo(FString::Printf(TEXT("RLE Compression Color: Uncompressed size %u bytes; compressed size %u bytes"), uncompressedSize, compressedSize));
      }

      //random(ish) color testing
      testArray.Reset();
      FRandomStream randomStream("Testing");
      uint32 amount = 4096 * 4096;
      uint32 amountAtATime = 8;

      AddInfo(FString::Printf(TEXT("RLE Compression Color random: Amount entries: %u; amount of same in a row: %u"), amount, amountAtATime));

      testArray.Reserve(amount);
      uncompressedSize = amount * sizeof(FColor);
      while (amount != 0)
      {
         uint32 amountToDecrease = FMath::Min(amount, amountAtATime);
         amount -= amountToDecrease;
         uint8 value = randomStream.RandRange(0, 255);
         while (amountToDecrease != 0)
         {
            testArray.Add(FColor(value));
            amountToDecrease--;
         }
      }

      didPass = TestCompressionFLE<FOSESaveCompressedRLEColorDataEntry>(testArray, compressedSize);
      if (!didPass)
      {
         AddError(FString::Printf(TEXT("RLE Compression failed - Color random")));
      }
      else
      {
         AddInfo(FString::Printf(TEXT("RLE Compression Color random: Uncompressed size %u bytes; compressed size %u bytes"), uncompressedSize, compressedSize));
      }


   }


   return true;
}


#endif //WITH_DEV_AUTOMATION_TESTS
