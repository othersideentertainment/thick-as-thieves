// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include "Math/GenericOctreePublic.h"
#include "Math/GenericOctree.h"

#include "OSELightDetectionTypes.generated.h"

class UOSELightEmittingComponent;
struct FInstancedStruct;
struct FStructView;

typedef TSharedRef<struct FLightEmitterOctreeID, ESPMode::ThreadSafe> FLightEmitterOctreeIDSharedRef;

/**
 * Handle to a registered LightEmitter.
 * Internal IDs are assigned in editor by the collection and then serialized for runtime.
 */
USTRUCT(BlueprintType)
struct OSELIGHTDETECTION_API FLightEmitterHandle
{
   GENERATED_BODY()

public:
   FLightEmitterHandle() {}

   /**
    * Indicates that the handle was properly assigned but doesn't guarantee that the associated object is still accessible.
    * This information requires a call to `ULightEmitterSubsystem::IsObjectValid` using the handle.
    */
   bool IsValid() const { return *this != Invalid; }
   void Invalidate() { *this = Invalid; }

   friend FString LexToString(const FLightEmitterHandle Handle)
   {
      return LexToString(Handle.ID);
   }

   bool operator==(const FLightEmitterHandle Other) const { return ID == Other.ID; }
   bool operator!=(const FLightEmitterHandle Other) const { return !(*this == Other); }

   friend uint32 GetTypeHash(const FLightEmitterHandle Handle)
   {
      return Handle.ID;
   }

private:
   friend struct FLightEmitterHandleFactory;
   explicit FLightEmitterHandle(const uint32 InID) : ID(InID) {}

   UPROPERTY(VisibleAnywhere, Category = LightEmitter)
   uint32 ID = INDEX_NONE;
public:
   static const FLightEmitterHandle Invalid;
};

struct OSELIGHTDETECTION_API FLightEmitterOctreeID : public TSharedFromThis<FLightEmitterOctreeID, ESPMode::ThreadSafe>
{
	FOctreeElementId2 ID;
};

struct OSELIGHTDETECTION_API FLightEmitterOctreeElement
{
	FBoxCenterAndExtent Bounds;
	FLightEmitterHandle LightEmitterHandle;
	FLightEmitterOctreeIDSharedRef SharedOctreeID;

	FLightEmitterOctreeElement(const FBoxCenterAndExtent& bounds, const FLightEmitterHandle lightEmitterHandle, const FLightEmitterOctreeIDSharedRef& sharedOctreeID);
};

struct FLightEmitterOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static const FBoxCenterAndExtent& GetBoundingBox(const FLightEmitterOctreeElement& Element)
	{
		return Element.Bounds;
	}

	FORCEINLINE static bool AreElementsEqual(const FLightEmitterOctreeElement& a, const FLightEmitterOctreeElement& b)
	{
		return a.LightEmitterHandle == b.LightEmitterHandle;
	}

	static void SetElementId(const FLightEmitterOctreeElement& element, FOctreeElementId2 id);
};

struct FLightEmitterOctree : TOctree2<FLightEmitterOctreeElement, FLightEmitterOctreeSemantics>
{
public:
	FLightEmitterOctree();
	FLightEmitterOctree(const FVector& origin, float radius);
	virtual ~FLightEmitterOctree();

	/** Add new node and initialize using LightEmitter runtime data */
	void AddNode(const FBoxCenterAndExtent& bounds, const FLightEmitterHandle lightEmitterHandle, const FLightEmitterOctreeIDSharedRef& sharedOctreeID);
	
	/** Updates element bounds remove/add operation */
	void UpdateNode(const FOctreeElementId2& id, const FBox& newBounds);

	/** Remove node */
	void RemoveNode(const FOctreeElementId2& id);
};

USTRUCT()
struct OSELIGHTDETECTION_API FLightEmitterOctreeEntryData
{
	GENERATED_BODY()

	FLightEmitterOctreeEntryData() : SharedOctreeID(MakeShareable(new FLightEmitterOctreeID())) {}

	FLightEmitterOctreeIDSharedRef SharedOctreeID;
};


USTRUCT()
struct OSELIGHTDETECTION_API FLightEmitterRuntimeData
{
   GENERATED_BODY();
   
   UPROPERTY()
   TWeakObjectPtr<UOSELightEmittingComponent> OwnerComponent;
   
   /** RegisteredHandle != FSmartObjectHandle::Invalid when registered with SmartObjectSubsystem */
   FLightEmitterHandle RegisteredHandle;

   /** Data stored within the octree */
   FLightEmitterOctreeEntryData SpatialEntryData;
};
