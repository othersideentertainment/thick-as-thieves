// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// OSE
#include <OSEXrayComponent.h>

#include "TATXrayComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATXrayComponent : public UOSEXrayComponent
{
   GENERATED_BODY()

public:
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   virtual bool IsEnabled() const override;
   virtual UMaterialInterface* GetMaterial() const override;
   virtual float GetMaxDrawDistance() const override;
   virtual bool ShouldRenderHiddenPrimitives() const override;
   virtual bool ShouldRenderOccludedPrimitives() const override;
   virtual bool ShouldRenderNonOccludedPrimitives() const override;

   void SetEnabled(bool enabled);

   UPROPERTY(EditDefaultsOnly, Category = "X-Ray Component")
   bool VisibilityRequiresThiefVision = true;

private:
   UFUNCTION()
   void _OnThiefVisionStatusChanged(APlayerController* controller, bool thiefVisionEnabled);

   UPROPERTY(VisibleAnywhere)
   bool _isEnabled = true;

   UPROPERTY(EditAnywhere)
   TObjectPtr<UMaterialInterface> _material;

   UPROPERTY(EditAnywhere)
   float _maxDrawDistance = 0.0f;

   UPROPERTY(EditAnywhere)
   bool _renderHiddenPrimitives = false;

   UPROPERTY(EditAnywhere)
   bool _renderOccludedPrimitives = true;

   UPROPERTY(EditAnywhere)
   bool _renderNonOccludedPrimitives = true;

   bool _thiefVisionEnabled = false;
};
