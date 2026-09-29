// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Collision/Overlay/CollisionOverlayTypes.h"

struct FCollisionOverlaySetEntry
{
   FCollisionOverlaySetEntry(FCollisionOverlayKey key, const FCollisionResponseContainer& mask)
      : Mask(mask), OwnerKey(key)
   {}

   FCollisionResponseContainer Mask;
   FCollisionOverlayKey OwnerKey;
};

/// A simple struct to track collision overlays applied to them
/// These overlays only reduce the collision response, so there is no ordering to care about.
/// Not a USTRUCT
struct FCollisionOverlaySet
{
public:
   void SetBase(const FCollisionResponseContainer& base);
   void SetBaseResponseToChannel(ECollisionChannel channel, ECollisionResponse newResponse);

   void AddOverlay(const FCollisionOverlayKey& ownerKey, const FCollisionResponseContainer& responseMask);
   bool RemoveOverlayByKey(const FCollisionOverlayKey& ownerKey);

   FCollisionResponseContainer GetCombinedResponse() const;

private:
   FCollisionResponseContainer _baseResponse;
   TArray<FCollisionOverlaySetEntry> _overlays;
};
