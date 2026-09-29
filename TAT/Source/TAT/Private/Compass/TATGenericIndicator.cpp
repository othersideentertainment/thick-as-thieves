// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Compass/TATGenericIndicator.h"

// tat
#include "Compass/TATAreaIndicatorComponent.h"
#include "Compass/TATCompassRegistrySubsystem.h"
#include "GameFramework/TATTravelMgr.h"
#include "TATGameInstance.h"
#include "UI/TATCompassWidget.h"
#include "UI/TATHUD.h"

// ose
#include "Utl/OSEShapeCollisionTrackerComponent.h"
#include "Misc/UObjectToken.h"
#include "Logging/MessageLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGenericIndicator)

ATATGenericIndicator::ATATGenericIndicator()
{
   bAlwaysRelevant = true;

   _areaIndicatorComponent = CreateDefaultSubobject<UTATAreaIndicatorComponent>(TEXT("AreaIndicatorComponent"));
}

void ATATGenericIndicator::BeginPlay()
{
   Super::BeginPlay();
   if (!IsNetMode(NM_DedicatedServer))
   {
      _InitializeWorldSpaceRepresentation();

      if (ForceIndicatorEnabled)
      {
         _enabled = true;
      }

      // Initialize indicator's visual state
      SetIndicatorEnabled(_enabled);

      _RegisterIndicator();
   }
}

void ATATGenericIndicator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   Super::EndPlay(EndPlayReason);
}

void ATATGenericIndicator::Destroyed()
{
   _UnregisterIndicator();
   Super::Destroyed();
}

void ATATGenericIndicator::_RegisterIndicator()
{
   UE_LOG(LogTATGenericIndicator, Verbose, TEXT("%s -> _InitializeIndicator()"), *GetName());

   // Cache z-sort priority before registration
   _indicatorAppearanceData.CompassZSortPriority = GetCompassIconPriorityOrder();
   
   if (UTATCompassRegistrySubsystem* registry = GetWorld()->GetSubsystem<UTATCompassRegistrySubsystem>())
   {
      registry->RegisterIndicator(this);
   }
}

void ATATGenericIndicator::_UnregisterIndicator()
{
   // Deregister indicator from compass
   if (UTATCompassRegistrySubsystem* registry = GetWorld()->GetSubsystem<UTATCompassRegistrySubsystem>())
   {
      registry->UnregisterIndicator(this);
   }
}

bool ATATGenericIndicator::ShouldShowCompassAreaGlow() const
{
   const bool isAreaIndicator = _closeRangeDetectionMode == ETATIndicatorCloseRangeDetectionMode::AreaVolume;
   const bool showOnCompassFixed = _indicatorAppearanceData.CloseRangeDisplayMode == ETATIndicatorCloseRangeDisplayMode::ShowOnCompassFixed;
   
   return GetIndicatorEnabled() && isAreaIndicator && showOnCompassFixed && _isPlayerInCloseRange;
}

void ATATGenericIndicator::SetIndicatorEnabled(bool enabled)
{
   _enabled = enabled;

   // CONSIDER: Should we register and unregister when enabling?
   //           Might be a win if there a lot of disabled indicators that will never enable
   UTATCompassWidget* compass = UTATCompassWidget::TryGetCompass(this);
   if (IsValid(compass))
   {
      compass->SetPOIEnabledForIndicator(this, enabled);
      compass->HandlePlayerInCloseRange(this, _isPlayerInCloseRange);
   }

   if (!IsNetMode(NM_DedicatedServer))
   {
      const bool visibleInWorldSpace = enabled && _isPlayerInCloseRange && GetCloseRangeDisplayMode() == ETATIndicatorCloseRangeDisplayMode::ShowInWorldSpace;
      SetVisibleInWorldSpace(visibleInWorldSpace);
   }
}

void ATATGenericIndicator::SetPlayerInCloseRange(bool isPlayerInCloseRange, bool isInitializing)
{
   if (_isPlayerInCloseRange != isPlayerInCloseRange || isInitializing)
   {
      _isPlayerInCloseRange = isPlayerInCloseRange;
      OnPlayerEnterExitsCloseRange(_isPlayerInCloseRange);
   }
}

#if WITH_EDITOR
void ATATGenericIndicator::CheckForErrors()
{
   Super::CheckForErrors();

   // if we were configured to have an area volume, but don't have one, complain about it
   if (_closeRangeDetectionMode == ETATIndicatorCloseRangeDetectionMode::AreaVolume)
   {
      if (!_areaIndicatorComponent || _areaIndicatorComponent->GetAreaVolume().IsNull())
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("%s close ranged detection mode set to AreaVolume but has no Area Volume Object assigned!"), *GetName()))));
      }
   }
}
#endif


