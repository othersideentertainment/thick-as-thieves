// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATLockCombinationName.generated.h"

// Just a wrapper around combination lock names for editor bells
// "Lock names" are just identifiers used to deterministically generate combinations from
//
// version that is used by a lock in a level, implicitly "defining" it
USTRUCT()
struct TAT_API FTATLockCombinationName
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   FName Name;

   FORCEINLINE bool IsNone() const { return Name.IsNone(); }
   
   void PostSerialize(const FArchive& ar);
   bool SerializeFromMismatchedTag(const FPropertyTag& tag, FStructuredArchive::FSlot slot);
};

template<>
struct TStructOpsTypeTraits< FTATLockCombinationName > : public TStructOpsTypeTraitsBase2< FTATLockCombinationName >
{
   enum
   {
      WithPostSerialize = true,
      WithStructuredSerializeFromMismatchedTag = true,
   };
};

// version that is a reference to a lock name in a level, but does not define it
USTRUCT()
struct TAT_API FTATLockCombinationNameRef : public FTATLockCombinationName
{
   GENERATED_BODY()
};

template<>
struct TStructOpsTypeTraits< FTATLockCombinationNameRef > : public TStructOpsTypeTraitsBase2< FTATLockCombinationNameRef >
{
   enum
   {
      WithPostSerialize = true,
      WithStructuredSerializeFromMismatchedTag = true,
   };
};
