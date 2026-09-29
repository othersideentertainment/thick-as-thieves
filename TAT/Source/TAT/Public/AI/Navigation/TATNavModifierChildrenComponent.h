// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "NavModifierComponent.h"
#include "Math/NumericLimits.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "TATNavModifierChildrenComponent.generated.h"

// Nav modifier component meant to target specific child components of the owning actor
UCLASS(ClassGroup = (Navigation), meta = (BlueprintSpawnableComponent), hidecategories = (Activation), config = Engine, defaultconfig)
class TAT_API UTATNavModifierChildrenComponent : public UNavModifierComponent
{
   GENERATED_BODY()

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
	
public:
   // How often nav recalculations are allowed to occur for 'NavModifiableComponentTag' scene components
   UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", Units = "seconds"))
   float RefreshInterval = 0.25f;

   // from UObject
   virtual void BeginDestroy() override;

   // from UNavRelevantComponent
   virtual void CalcAndCacheBounds() const override;
   virtual void CalculateBounds() const override;
   virtual void GetNavigationData(FNavigationRelevantData& data) const override;

protected:
   // Calculates the bounds of the collidable bodies within 'primitive' and adds them to 
   // '_staticModifierBounds' which is supplied to the navmesh as nav areas
   void AppendStaticComponentBounds(UPrimitiveComponent* primitive, const FTransform& offset, TArray<FRotatedBox>& bounds) const;

   void OnDynamicComponentTransformUpdated(USceneComponent* component, EUpdateTransformFlags updateTransformFlags, ETeleportType teleport);

private:
   mutable bool _hasDynamicModifier = false;
   mutable TArray<FRotatedBox> _staticModifierBounds;

   float _lastRefreshTime = MIN_flt;

   FTimerHandle _lastRefreshTimeout;
   TWeakObjectPtr<USceneComponent> _lastRefreshComponent = nullptr;

   void _OnLastRefreshTimeoutElapsed();
};
