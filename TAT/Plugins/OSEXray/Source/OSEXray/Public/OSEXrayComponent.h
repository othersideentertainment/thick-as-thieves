// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <Components/ActorComponent.h>

#include "OSEXrayComponent.generated.h"

UCLASS(MinimalAPI, meta=(BlueprintSpawnableComponent))
class UOSEXrayComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   OSEXRAY_API virtual void OnRegister() override;
   OSEXRAY_API virtual void OnUnregister() override;

   virtual bool IsEnabled() const;
   virtual UMaterialInterface* GetMaterial() const;
   virtual float GetMaxDrawDistance() const;

   virtual bool ShouldRenderHiddenPrimitives() const;
   virtual bool ShouldRenderOccludedPrimitives() const;
   virtual bool ShouldRenderNonOccludedPrimitives() const;
};
