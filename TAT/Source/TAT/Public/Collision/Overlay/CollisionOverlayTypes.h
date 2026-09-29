// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "GameplayTagContainer.h"

// A key for identifying collision overlays
//
// Either:
// 1. An FObjectKey
// 2. An FName/GameplayTag
//
// This probably could have been bitpacked, as there are sentinel
// values. But it is private, so probably cleaner to just have both.
// (and did not bother with a discriminated union)
struct FCollisionOverlayKey
{
   FCollisionOverlayKey(const UObject* object)
      : _objectKey(object)
   {}

   FCollisionOverlayKey(FObjectKey key)
      : _objectKey(key)
   {}

   FCollisionOverlayKey(FGameplayTag tag)
      : _name(tag.GetTagName())
   {}

   FCollisionOverlayKey(FName name)
      : _name(name)
   {}

   bool operator==(const FCollisionOverlayKey& other) const
   {
      return _objectKey == other._objectKey && _name == other._name;
   }

private:
   FObjectKey _objectKey;
   FName _name;
};
