// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Compass/TATCardinalDirectionIndicator.h"

// tat
#include "Player/TATPlayerController.h"
#include "UI/TATCompassPOIWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCardinalDirectionIndicator)

ATATCardinalDirectionIndicator::ATATCardinalDirectionIndicator()
{
   _indicatorAppearanceData.ShowDistance = false;
   _indicatorAppearanceData.UseVerticalDisplacement = false;
   _indicatorAppearanceData.ShowOffScreen = false;
   _indicatorAppearanceData.ShowDropShadow = false;
   _indicatorAppearanceData.POIDisplayMode = EPOIDisplayMode::Text;
   _indicatorAppearanceData.POICategory = EPOICategory::Generic;
   _indicatorAppearanceData.CloseRangeDisplayMode = ETATIndicatorCloseRangeDisplayMode::ShowOnCompass;

   ForceIndicatorEnabled = true;
}

FVector ATATCardinalDirectionIndicator::GetIndicatorLocation() const
{
   const ATATPlayerController* playerController = ATATPlayerController::GetLocalTATPlayerController(this);
   if (!IsValid(playerController))
   {
      return FVector();
   }

   FVector outPlayerLocation;
   FRotator outPlayerRotation;
   playerController->GetPlayerViewPoint(outPlayerLocation, outPlayerRotation);

   return outPlayerLocation + (_relativePlayerOffset * _offsetDistance);
}

void ATATCardinalDirectionIndicator::SetCardinality(const FVector relativePlayerOffset, const FText displayText)
{
   _relativePlayerOffset = relativePlayerOffset;
   _indicatorAppearanceData.DisplayText = displayText;
}

