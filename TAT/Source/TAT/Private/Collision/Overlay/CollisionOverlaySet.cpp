// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Collision/Overlay/CollisionOverlaySet.h"

void FCollisionOverlaySet::SetBase(const FCollisionResponseContainer& base)
{
   _baseResponse = base;
}

void FCollisionOverlaySet::SetBaseResponseToChannel(ECollisionChannel channel, ECollisionResponse newResponse)
{
   _baseResponse.SetResponse(channel, newResponse);
}

void FCollisionOverlaySet::AddOverlay(const FCollisionOverlayKey& ownerKey, const FCollisionResponseContainer& responseMask)
{
   _overlays.Emplace(ownerKey, responseMask);
}

bool FCollisionOverlaySet::RemoveOverlayByKey(const FCollisionOverlayKey& ownerKey)
{
   return _overlays.RemoveAllSwap([ownerKey](const FCollisionOverlaySetEntry& entry) { return entry.OwnerKey == ownerKey; }) > 0;
}

FCollisionResponseContainer FCollisionOverlaySet::GetCombinedResponse() const
{
   // computing on the fly until there is a reason not to
   FCollisionResponseContainer result = _baseResponse;
   for (const FCollisionOverlaySetEntry& entry : _overlays)
   {
      // do we care enough to do this in-place?
      result = FCollisionResponseContainer::CreateMinContainer(result, entry.Mask);
   }
   return result;
}
