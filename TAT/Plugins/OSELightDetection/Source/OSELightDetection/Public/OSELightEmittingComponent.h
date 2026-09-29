// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSELightDetectionTypes.h"

// ue
#include "Components/ActorComponent.h"

#include "OSELightEmittingComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSELightEmittingComponent, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OSELIGHTDETECTION_API UOSELightEmittingComponent : public UActorComponent
{
   GENERATED_BODY()
public:
   UOSELightEmittingComponent();

   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   
   bool AffectsDetectionComponent(const UPrimitiveComponent* component) const;
   FLinearColor GetLightColor() const;
   float GetLightAttenuationDistance() const;
   FVector GetLightEmissionLocation(const FTransform& transform) const;

   void SetRegisteredHandle(const FLightEmitterHandle& handle);
   void InvalidateRegisteredHandle();
   
   FLightEmitterHandle GetRegisteredHandle() const { return _registeredHandle; }
   FBox GetLightBounds() const;
   bool IsMovable() const;
   bool AreComponentsValid() const;
   void HandleOwnerHiddenStateChanged();
   float GetLightFudgeValue() const { return _lightFudgeValue; }
   UPROPERTY(BlueprintReadWrite)
   ULightComponent* _lightComponent { nullptr };
   
   virtual void OnActorVisibilityChanged() override;
   
private:
   bool _OwnerIsHidden { false };
   UPROPERTY(EditAnywhere)
   float _lightFudgeValue { 1.f };
   UPROPERTY(Transient)
   FLightEmitterHandle _registeredHandle { FLightEmitterHandle::Invalid }; 
};
