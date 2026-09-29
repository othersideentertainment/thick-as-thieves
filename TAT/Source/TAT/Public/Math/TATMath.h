// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "CoreMinimal.h"

template<typename T>
struct TTATRandomWeight
{
   T Value;
   float Weight = 1.0f;
};

namespace TATMath
{
   template<typename T, typename Lambda>
   const T* SelectRandomItemWeighted(TConstArrayView<T> items, Lambda&& getWeightFunc, TFunctionRef<float(float, float)> randRangeFunc)
   {
      if (items.Num() <= 0)
      {
         return nullptr;
      }

      float totalWeight = 0.0f;
      for (const T& item : items)
      {
         totalWeight += FMath::Max(0.0f, getWeightFunc(item));
      }

      if (totalWeight <= 0)
      {
         return nullptr;
      }

      float cur = 0.0f;
      const float selection = randRangeFunc(0.0f, totalWeight);
      for (const T& item : items)
      {
         cur += getWeightFunc(item);
         if (selection <= cur)
         {
            return &item;
         }
      }

      return nullptr;
   }

   template<typename T, typename WeightLambda>
   int32 SelectRandomUniqueItemsWeighted(TConstArrayView<T> items, int32 count, WeightLambda&& getWeightFunc, TFunctionRef<void(const T&)> selectItemFunc, TFunctionRef<float(float, float)> randRangeFunc)
   {
      if (count <= 0 || items.Num() <= 0)
      {
         return 0;
      }

      float totalWeight = 0.0f;
      for (const T& item : items)
      {
         totalWeight += FMath::Max(0.0f, static_cast<float>(getWeightFunc(item)));
      }

      TSet<int32, DefaultKeyFuncs<int32>, TInlineSetAllocator<32>> selectedIndices;

      while (selectedIndices.Num() < count && totalWeight > 0.0f)
      {
         float cur = 0.0f;
         const float selection = randRangeFunc(0.0f, totalWeight);
         for (int32 i = 0; i < items.Num(); i++)
         {
            if (selectedIndices.Contains(i))
            {
               continue;
            }

            const T& item = items[i];
            const float itemWeight = static_cast<float>(getWeightFunc(item));
            cur += itemWeight;
            if (selection <= cur)
            {
               selectedIndices.Add(i);
               totalWeight -= itemWeight;
               selectItemFunc(item);
            }
         }
      }

      return selectedIndices.Num();
   }

   template<typename T, typename Lambda>
   const T* SelectRandomItemWeighted(TConstArrayView<T> items, Lambda&& getWeightFunc)
   {
      return SelectRandomItemWeighted(items, Forward<Lambda>(getWeightFunc), [](float minVal, float maxVal) { return FMath::FRandRange(minVal, maxVal); });
   }

   template<typename T>
   TOptional<T> WeightedRandom(TConstArrayView<TTATRandomWeight<T>> items)
   {
      if (const TTATRandomWeight<T>* result = SelectRandomItemWeighted(items, [](const TTATRandomWeight<T>& item) { return item.Weight; }))
      {
         return result->Value;
      }
      return NullOpt;
   }
}
