// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSECompression.generated.h"

USTRUCT(BlueprintType)
struct OSECORE_API FOSESaveCompressedRLEUintDataEntry
{
   GENERATED_BODY()
public:

   UPROPERTY()
      uint32 numOfSameData = 0;

   UPROPERTY()
      uint8 data = 0;
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSESaveCompressedRLEColorDataEntry
{
   GENERATED_BODY()
public:

   UPROPERTY()
      uint32 numOfSameData = 0;

   UPROPERTY()
      FColor data = FColor::Transparent;
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSESaveCompressedRLEUintData
{
   GENERATED_BODY()
public:

   UPROPERTY()
      TArray< FOSESaveCompressedRLEUintDataEntry> data;
};
namespace OSECompression
{

#pragma region RLE

   template <typename T_ArrayType, typename T_CompressedMapEntry>
   inline void CompressArray_RLE(const TArray<T_ArrayType>& sourceArray, TArray<T_CompressedMapEntry>& compressedArrayOut)
   {
      if (sourceArray.Num() == 0)
      {
         return;
      }
      compressedArrayOut.Reserve(sourceArray.Num()); //max amount is a change at every pixel

      uint32 amountOfSame = 0;
      T_ArrayType sameValue = T_ArrayType(0);
      for (const T_ArrayType& sourceData : sourceArray)
      {
         if (amountOfSame == 0)
         {
            //first set
            sameValue = sourceData;
         }

         if (sameValue != sourceData)
         {
            //push a new entry

            T_CompressedMapEntry entry;
            entry.data = sameValue;
            entry.numOfSameData = amountOfSame;
            compressedArrayOut.Add(entry);

            //setup new value
            amountOfSame = 0;
            sameValue = sourceData;
         }

         amountOfSame++;
      }

      {
         //push the last info
         T_CompressedMapEntry entry;
         entry.data = sameValue;
         entry.numOfSameData = amountOfSame;
         compressedArrayOut.Add(entry);
      }
   }

   template <typename T_ArrayType, typename T_CompressedMapEntry>
   inline void UncompressArray_RLE(TArray<T_ArrayType>& uncompressedArrayOut, const TArray<T_CompressedMapEntry>& compressedMapIn)
   {
      if (compressedMapIn.Num() == 0)
      {
         return;
      }

      uncompressedArrayOut.Reset();

      TArray<T_ArrayType> newSection;
      for (const T_CompressedMapEntry& saveEntry : compressedMapIn)
      {
         newSection.Init(saveEntry.data, saveEntry.numOfSameData);
         uncompressedArrayOut.Append(newSection);
      }
   }



#pragma endregion Run Length Encoding

};
