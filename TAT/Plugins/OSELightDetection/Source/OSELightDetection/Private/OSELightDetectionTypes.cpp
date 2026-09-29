// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSELightDetectionTypes.h"

const FLightEmitterHandle FLightEmitterHandle::Invalid;

FLightEmitterOctreeElement::FLightEmitterOctreeElement(const FBoxCenterAndExtent& bounds,
                                                       const FLightEmitterHandle lightEmitterHandle,
                                                       const FLightEmitterOctreeIDSharedRef& sharedOctreeID)
   : Bounds(bounds)
   , LightEmitterHandle(lightEmitterHandle)
   , SharedOctreeID(sharedOctreeID)
{
}

FLightEmitterOctree::FLightEmitterOctree()
   : FLightEmitterOctree(FVector::ZeroVector, 0)
{

}

FLightEmitterOctree::FLightEmitterOctree(const FVector& origin, const float radius)
   : TOctree2<FLightEmitterOctreeElement, FLightEmitterOctreeSemantics>(origin, radius)
{
}

FLightEmitterOctree::~FLightEmitterOctree()
{
}

void FLightEmitterOctree::AddNode(const FBoxCenterAndExtent& bounds, const FLightEmitterHandle lightEmitterHandle,
                                  const FLightEmitterOctreeIDSharedRef& sharedOctreeID)
{
   AddElement(FLightEmitterOctreeElement(bounds, lightEmitterHandle, sharedOctreeID));
}

void FLightEmitterOctree::UpdateNode(const FOctreeElementId2& id, const FBox& newBounds)
{
   FLightEmitterOctreeElement ElementCopy = GetElementById(id);
   RemoveElement(id);
   ElementCopy.Bounds = newBounds;
   AddElement(ElementCopy);
}

void FLightEmitterOctree::RemoveNode(const FOctreeElementId2& id)
{
   RemoveElement(id);
}

void FLightEmitterOctreeSemantics::SetElementId(const FLightEmitterOctreeElement& element, const FOctreeElementId2 id)
{
   // You may be wondering, _why_ is the ID set here? How does that work.
   // FLightEmitterOctreeElement is returned as the "runtime data" for the light emitter component
   // This has both the ID for the octree AND a reference to the light emitter component itself.
   // So when we come to remove an element from the octree, we only need to pass in the FLightEmitterHandle to find the
   // ID that the Octree is using to find the element.
   // Yes, it's convoluted, but that's just how the TOctree2 is setup to work. WWise doesn't do it this way, and instead
   // has a map alongside the octree that is stashing off the ID against the component unique ID (FAkEnvironmentOctreeSemantics::SetElementId)
   // I don't like that way because it's relying on the component being alive at the time of removal.
   // As uniqueness is only guaranteed while the UObject is alive.
   element.SharedOctreeID->ID = id;
}
