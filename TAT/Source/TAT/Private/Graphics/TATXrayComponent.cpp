// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/TATXrayComponent.h"

// tat
#include "Indicators/TATThiefVisionSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATXrayComponent)

// #TODO: Separate friendly/enemy materials?

void UTATXrayComponent::BeginPlay()
{
   Super::BeginPlay();

   UWorld* world = GetWorld();
   if (VisibilityRequiresThiefVision && ensure(world))
   {
      if (UTATThiefVisionSubsystem* thiefVisionSubsystem = world->GetSubsystem<UTATThiefVisionSubsystem>())
      {
         _thiefVisionEnabled = thiefVisionSubsystem->IsThiefVisionEnabledForLocalPlayer();
         thiefVisionSubsystem->OnLocalPlayerThiefVisionStatusChanged.AddDynamic(this, &UTATXrayComponent::_OnThiefVisionStatusChanged);
      }
   }
}

void UTATXrayComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   UWorld* world = GetWorld();
   if (VisibilityRequiresThiefVision && ensure(world))
   {
      if (UTATThiefVisionSubsystem* thiefVisionSubsystem = world->GetSubsystem<UTATThiefVisionSubsystem>())
      {
         thiefVisionSubsystem->OnLocalPlayerThiefVisionStatusChanged.RemoveAll(this);
      }
   }

   Super::EndPlay(endPlayReason);
}

bool UTATXrayComponent::IsEnabled() const
{
   return _isEnabled && (!VisibilityRequiresThiefVision || _thiefVisionEnabled);
}

UMaterialInterface* UTATXrayComponent::GetMaterial() const
{
   return _material;
}

float UTATXrayComponent::GetMaxDrawDistance() const
{
   return _maxDrawDistance;
}

bool UTATXrayComponent::ShouldRenderHiddenPrimitives() const
{
   return _renderHiddenPrimitives;
}

bool UTATXrayComponent::ShouldRenderOccludedPrimitives() const
{
   return _renderOccludedPrimitives;
}

bool UTATXrayComponent::ShouldRenderNonOccludedPrimitives() const
{
   return _renderNonOccludedPrimitives;
}

void UTATXrayComponent::SetEnabled(bool enabled)
{
   _isEnabled = enabled;
}

void UTATXrayComponent::_OnThiefVisionStatusChanged(APlayerController* controller, bool thiefVisionEnabled)
{
   _thiefVisionEnabled = thiefVisionEnabled;
}
