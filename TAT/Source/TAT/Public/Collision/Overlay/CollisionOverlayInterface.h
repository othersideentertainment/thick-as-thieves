// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Collision/Overlay/CollisionOverlayTypes.h"

// ue5
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "CollisionOverlayInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UCollisionOverlayInterface : public UInterface
{
	GENERATED_BODY()
};

/// A simple interface for an object that can have collision overlays applied to them.
/// These overlays only reduce the collision response, so there is no ordering to care about.
/// Kept c++-only in case the interface changes.
class TAT_API ICollisionOverlayInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   virtual void AddCollisionOverlay(const FCollisionOverlayKey& key, const FCollisionResponseContainer& responseMask) = 0;
   virtual void RemoveCollisionOverlayByKey(const FCollisionOverlayKey& key) = 0;
};
