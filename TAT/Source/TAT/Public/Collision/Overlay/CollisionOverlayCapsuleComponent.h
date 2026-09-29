// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Collision/Overlay/CollisionOverlaySet.h"
#include "Collision/Overlay/CollisionOverlayInterface.h"

// ue
#include "Components/CapsuleComponent.h"

#include "CollisionOverlayCapsuleComponent.generated.h"

/// A capsule component that implements the ICollisionOverlayInterface to allow collision overlays to be added or 
/// removed without worrying about them fighting each other or ordering.
///
/// Normal PrimimitiveComponent methods to set the collision response will set the "base" response that the overlays
/// are applied to.
UCLASS()
class TAT_API UCollisionOverlayCapsuleComponent : public UCapsuleComponent, public ICollisionOverlayInterface
{
	GENERATED_BODY()

   virtual void BeginPlay() override;

   // ICollisionOverlayInterface
   virtual void AddCollisionOverlay(const FCollisionOverlayKey& key, const FCollisionResponseContainer& responseMask) override;
   virtual void RemoveCollisionOverlayByKey(const FCollisionOverlayKey& key) override;

   // UPrimimitiveComponent
   virtual void SetCollisionProfileName(FName inCollisionProfileName, bool updateOverlaps = true) override;
   virtual void SetCollisionResponseToChannel(ECollisionChannel channel, ECollisionResponse newResponse) override;
   virtual void SetCollisionResponseToAllChannels(ECollisionResponse newResponse) override;
   virtual void SetCollisionResponseToChannels(const FCollisionResponseContainer& newResponses) override;
private:
   void _TryApplyOverlayResponses();

private:
   FCollisionOverlaySet _collisionOverlay;
};
