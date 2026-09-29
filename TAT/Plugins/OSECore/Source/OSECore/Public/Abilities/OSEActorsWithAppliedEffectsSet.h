// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "ActiveGameplayEffectHandle.h"

#include "OSEActorsWithAppliedEffectsSet.generated.h"

class AActor;

//////////////////////////////////////////////////////////////////////////
///            FOSEActorsWithAppliedEffectsSet
//////////////////////////////////////////////////////////////////////////

// keeping this non-blueprint exposed, so it is easy to change if needed
USTRUCT()
struct FOSEActorWithAppliedEffectEntry
{
   GENERATED_BODY()

   FOSEActorWithAppliedEffectEntry() {}

   FOSEActorWithAppliedEffectEntry(AActor* actor, FActiveGameplayEffectHandle handle)
      : Actor(actor), AppliedEffectHandle(handle)
   {}

   UPROPERTY()
   AActor* Actor = nullptr;

   UPROPERTY()
   FActiveGameplayEffectHandle AppliedEffectHandle;
};

// A simple set of actors with the effects applied to them.
// If you need something more complicated, use something else.
// It isn't doing so much that can't be done another way.
USTRUCT(BlueprintType)
struct OSECORE_API FOSEActorsWithAppliedEffectsSet
{
   GENERATED_BODY()

   // deliberately not blueprint exposed
   UPROPERTY(Transient)
   TArray<FOSEActorWithAppliedEffectEntry> EffectEntries;

   void Add(AActor* actor, FActiveGameplayEffectHandle effectHandle);
   void AddMultiple(AActor* actor, TConstArrayView<FActiveGameplayEffectHandle> effectHandles);

   // -1 stacks removes all of them
   // The default is 1 on the assumption was that, if the effect could stack from multiple sources, then it still wanted to only remove the stacks that it added (probably one), rather than all of them.
   bool CancelByActor(AActor* actor, int32 stacksToRemove = 1);
   void CancelAll(int32 stacksToRemove = 1);
};


