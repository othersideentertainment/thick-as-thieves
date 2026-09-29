// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Detection/TATLightDetectionCharacterComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLightDetectionCharacterComponent)

float UTATLightDetectionCharacterComponent::GetMinimumLightIntensity() const
{
     if(_MovementSpeedToMinimumLightIntensity == nullptr)
     {
        return 0.f;
     }
     return _MovementSpeedToMinimumLightIntensity->GetFloatValue(GetOwner()->GetVelocity().Size());
}
