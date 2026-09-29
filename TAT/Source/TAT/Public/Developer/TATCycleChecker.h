// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "Misc/UObjectToken.h"


#if WITH_EDITOR
namespace TATCycleChecker
{
   template <typename T>
   inline void AddCycleError(FTokenizedMessage& message, TConstArrayView<const T*> foundCycle, TFunctionRef<TSharedRef<FUObjectToken>(const T*)> makeToken = [](const T* object) { return FUObjectToken::Create(object); })
   {
      for (int i = 0; i < foundCycle.Num(); ++i)
      {
         if (i > 0)
         {
            message.AddToken(FTextToken::Create(INVTEXT("->")));
         }
         message.AddToken(makeToken(foundCycle[i]));
      }
   }
}

// Generic utility class for finding cycles of uobjects
//
// Note that use of TFunction{Ref} is mostly to avoid fighting template deduction
// of lambdas, since this is a template anyways.
template <typename T>
struct TTATObjectCycleChecker
{
   using TGenerateFunc = TFunction<void (const T*, TFunctionRef<void (const T*)>)>;
   
   TTATObjectCycleChecker(TGenerateFunc&& generateFunc)
      : _generateFunc(MoveTemp(generateFunc))
   {}
   
   void DetectCycles(const T* object, const TFunctionRef<void(const TConstArrayView<const T*>&)>& callback)
   {
      _stack.Reset();
      _DetectCyclesInternal(object, callback);
   }

protected:
   void _DetectCyclesInternal(const T* object, const TFunctionRef<void(const TConstArrayView<const T*>&)>& callback)
   {
      if (_completed.Contains(object) || object == nullptr)
      {
         return;
      }

      // basic algorithm:
      // Does a depth-first search of the graph, keeping track of which nodes
      // are currently being visited and which ones have had all their children visited.
      // If it finds a node that is still being visited, then that must be a cycle.
      _stack.Push(object);

      _generateFunc(object, [this, &callback] (const T* dependency)
      {
         if (_stack.Contains(dependency))
         {
            // A cycle
            _stack.Push(dependency);

            // slice array to only include cycle
            const int32 firstIndex = _stack.IndexOfByKey(dependency);
            check(_stack.IsValidIndex(firstIndex));
            TConstArrayView<const T*> slice = TConstArrayView<const T*>(_stack).Slice(firstIndex, _stack.Num() - firstIndex);
            callback(slice);
            _stack.Pop();
         }
         else if (!_completed.Contains(dependency))
         {
            _DetectCyclesInternal(dependency, callback);
         }
      });
      _completed.Add(object);
      _stack.Pop();
   }
   
private:
   TGenerateFunc _generateFunc;
   TArray<const T*, TInlineAllocator<8>> _stack;
   TSet<const T*, DefaultKeyFuncs<const T*>, TInlineSetAllocator<8>> _completed;
};

#endif
