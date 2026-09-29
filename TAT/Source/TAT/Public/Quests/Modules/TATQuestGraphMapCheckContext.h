// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
class UTATQuestGraphNode;

struct TAT_API FTATQuestGraphMapCheckContext
{
   static FTATQuestGraphMapCheckContext Create(const UWorld* world, TUniqueFunction<void(TSharedRef<FTokenizedMessage>)> reporter)
   {
      return FTATQuestGraphMapCheckContext(world, MoveTemp(reporter));
   }
   
   TWeakObjectPtr<const UWorld> World;

   // returns true if not visited yet
   bool TryVisit(const UObject* graph);
   void Report(const UTATQuestGraphNode* node, const FText& message) const;
   const TSet<FName>& GetLockCombinationNames()  const { return _lockCombinationNames; }

private:
   FTATQuestGraphMapCheckContext(const UWorld* world, TUniqueFunction<void(TSharedRef<FTokenizedMessage>)> reporter);
   // CONDIDER: populate lazily?
   TSet<FName> _lockCombinationNames;
   TUniqueFunction<void(TSharedRef<FTokenizedMessage>)> _reporter;
   TArray<FWeakObjectPtr> _visitedGraphs;
};
#endif
