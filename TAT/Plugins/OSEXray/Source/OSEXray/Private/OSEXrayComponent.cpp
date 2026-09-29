// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEXrayComponent.h"

// OSE
#include "OSEXraySubsystem.h"

// UE
#include <Engine/World.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEXrayComponent)

void UOSEXrayComponent::OnRegister()
{
   Super::OnRegister();

   const UOSEXraySubsystem* xraySubsystem = UWorld::GetSubsystem<UOSEXraySubsystem>(GetWorld());
   if (xraySubsystem)
   {
      xraySubsystem->RegisterComponent(this);
   }
}

void UOSEXrayComponent::OnUnregister()
{
   Super::OnUnregister();

   const UOSEXraySubsystem* xraySubsystem = UWorld::GetSubsystem<UOSEXraySubsystem>(GetWorld());
   if (xraySubsystem)
   {
      xraySubsystem->UnregisterComponent(this);
   }
}

bool UOSEXrayComponent::IsEnabled() const
{
   return true;
}

UMaterialInterface* UOSEXrayComponent::GetMaterial() const
{
   return nullptr;
}

float UOSEXrayComponent::GetMaxDrawDistance() const
{
   return 0.0f;
}

bool UOSEXrayComponent::ShouldRenderHiddenPrimitives() const
{
   return false;
}

bool UOSEXrayComponent::ShouldRenderOccludedPrimitives() const
{
   return true;
}

bool UOSEXrayComponent::ShouldRenderNonOccludedPrimitives() const
{
   return true;
}
