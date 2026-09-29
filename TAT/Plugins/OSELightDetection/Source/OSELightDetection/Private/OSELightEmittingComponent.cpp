// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// ose
#include "OSELightEmittingComponent.h"
#include "OSELightDetectionWorldSubsystem.h"

// ue
#include "Components/LightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Engine/Light.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSELightEmittingComponent)

DEFINE_LOG_CATEGORY(LogOSELightEmittingComponent);

UOSELightEmittingComponent::UOSELightEmittingComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

void UOSELightEmittingComponent::BeginPlay()
{
   Super::BeginPlay();
   if(_lightComponent == nullptr)
   {
      _lightComponent = GetOwner()->FindComponentByClass<ULightComponent>();
      UE_LOG(
         LogOSELightEmittingComponent,
         Warning,
         TEXT("No light component set on %s, falling back and picking a light component at random."),
         *GetNameSafe(GetOwner())
      )
   }
   if(AreComponentsValid() == false)
   {
      return;
   }
   if(const UWorld* world = GetWorld())
   {
      if(UOSELightDetectionWorldSubsystem* lightDetectionWorldSubsystem = world->GetSubsystem<UOSELightDetectionWorldSubsystem>())
      {
         lightDetectionWorldSubsystem->RegisterLight(this);
         HandleOwnerHiddenStateChanged();
      }
   }
}

void UOSELightEmittingComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if(AreComponentsValid())
   {
      if(const UWorld* world = GetWorld())
      {
         if(UOSELightDetectionWorldSubsystem* lightDetectionWorldSubsystem = world->GetSubsystem<UOSELightDetectionWorldSubsystem>())
         {
            lightDetectionWorldSubsystem->UnRegisterLight(this);
         }
      }
   }
   Super::EndPlay(endPlayReason);
}

bool UOSELightEmittingComponent::AffectsDetectionComponent(const UPrimitiveComponent* component) const
{
   checkf(_lightComponent, TEXT("Light emitting component has no light (%s)"), *GetReadableName());
   if(_lightComponent == nullptr)
      return false;
   
   return _OwnerIsHidden == false && _lightComponent->IsVisible() && _lightComponent->AffectsPrimitive(component);
}


FLinearColor UOSELightEmittingComponent::GetLightColor() const
{
   return _lightComponent->GetColoredLightBrightness();
}

float UOSELightEmittingComponent::GetLightAttenuationDistance() const
{
   // Bounding Sphere W value is equal to the radius of the sphere.
   if(ULocalLightComponent* asLocalLightComponent = Cast<ULocalLightComponent>(_lightComponent))
   {
      return asLocalLightComponent->AttenuationRadius;
   }
   return _lightComponent->GetBoundingSphere().W;
}

FVector UOSELightEmittingComponent::GetLightEmissionLocation(const FTransform& transform) const
{
   if(_lightComponent->GetLightType() == LightType_Directional)
   {
      FVector actorLocation = transform.GetLocation();
      //todo: Currently using a hardcoded distance offset for the directional lights.
      //100k felt like a reasonable number, but maybe we should use something different?
      actorLocation += _lightComponent->GetDirection() * -100000.f;
      return actorLocation;
   }
   return _lightComponent->GetComponentLocation();
}

void UOSELightEmittingComponent::SetRegisteredHandle(const FLightEmitterHandle& handle)
{
   ensure(_registeredHandle.IsValid() == false);
   ensure(handle.IsValid());
   _registeredHandle = handle;
}

void UOSELightEmittingComponent::InvalidateRegisteredHandle()
{
   _registeredHandle = FLightEmitterHandle::Invalid;
}

FBox UOSELightEmittingComponent::GetLightBounds() const
{
   return _lightComponent->GetBoundingBox();
}

bool UOSELightEmittingComponent::IsMovable() const
{
   return _lightComponent->IsMovable();
}

bool UOSELightEmittingComponent::AreComponentsValid() const
{
   return _lightComponent != nullptr;
}

void UOSELightEmittingComponent::HandleOwnerHiddenStateChanged()
{
   // Instead of unregistering and registering the light when the owner becomes hidden / unhidden, just set a flag for now
   // I'm concerned with the adding / removing cost if this changes frequently.
   _OwnerIsHidden = GetOwner()->IsHidden();
}

void UOSELightEmittingComponent::OnActorVisibilityChanged()
{
   Super::OnActorVisibilityChanged();
   HandleOwnerHiddenStateChanged();
}

