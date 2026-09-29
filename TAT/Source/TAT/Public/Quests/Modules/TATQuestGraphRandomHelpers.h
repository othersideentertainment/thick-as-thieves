// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Math/TATMath.h"
#include "Math/TATXoshiroRandomStream.h"

class UTATQuestGraphNode;

// NOTE: Sticking with Xoshiro for now, but should keep an eye on whether that creates confusion with
//       the rest of map randomization, which used FRandomStream
namespace TATQuestGraphRandomHelpers
{
   // Creates random stream to use in quest graph execution of a given node
   FTATXoshiroRandomStream CreateStreamForNode(int32 mapSeed, const UTATQuestGraphNode* node);
   // Creates random stream to use when choosing the next node to select
   FTATXoshiroRandomStream CreateStreamForPath(int32 mapSeed, const UTATQuestGraphNode* node);
   
   /// Selects and returns a (weighted) random pointer to an item in the array view.
   /// Accepts an array of any struct type that contains a float field named "Weight".
   template<typename T>
   const T* EvalRandomWeighted(FTATXoshiroRandomStream& randomStream, TConstArrayView<T> itemView)
   {
      // Don't change the RandomStream state unless we have more than one item to choose from
      if (itemView.Num() == 0)
      {
         return nullptr;
      }
      if (itemView.Num() == 1)
      {
         return &itemView[0];
      }
      return TATMath::SelectRandomItemWeighted<T>(
         itemView,
         [](const T& item) { return item.Weight; },
         [&randomStream](float minVal, float maxVal) -> float { return randomStream.NextFloatInRange(minVal, maxVal); });
   }

   /// Selects a number of unique, weighted random items from an array and calls the callback for each selected item.
   /// Accepts an array of any struct type that contains a float field named "Weight".
   /// Returns the number of items selected.
   template<typename T>
   int32 EvalRandomWeightedMulti(FTATXoshiroRandomStream& randomStream, int32 count, TConstArrayView<T> itemView, TFunctionRef<void(const T&)> itemFunc)
   {
      // Don't change the RandomStream state unless we have more than one item to choose from
      if (count <= 0 || itemView.Num() == 0)
      {
         return 0;
      }
      if (itemView.Num() == 1)
      {
         itemFunc(itemView[0]);
         return 1;
      }
      return TATMath::SelectRandomUniqueItemsWeighted<T>(
         itemView,
         count,
         [](const T& item) { return item.Weight; },
         itemFunc,
         [&randomStream](float minVal, float maxVal) -> float { return randomStream.NextFloatInRange(minVal, maxVal); });
   }

   /// Selects a random number (in countRange, inclusive) of unique, weighted random items from an array and calls the callback for each selected item.
   /// Accepts an array of any struct type that contains a float field named "Weight".
   /// Returns the number of items selected.
   template<typename T>
   int32 EvalRandomWeightedMulti(FTATXoshiroRandomStream& randomStream, const FInt32Interval& countRange, TConstArrayView<T> itemView, TFunctionRef<void(const T&)> itemFunc)
   {
      const int32 count = FMath::Max(0, (countRange.Min == countRange.Max) ? countRange.Min : randomStream.NextInt32InRange(countRange.Min, countRange.Max));
      return EvalRandomWeightedMulti<T>(randomStream, count, itemView, itemFunc);
   }
}
