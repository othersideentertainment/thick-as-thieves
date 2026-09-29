// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UI/TATCompassWidget.h"

// tat
#include "Compass/TATCardinalDirectionIndicator.h"
#include "Compass/TATCompassRegistrySubsystem.h"
#include "Compass/TATGenericIndicator.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "UI/TATCompassPOIWidget.h"
#include "UI/TATHUD.h"

// ue4
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCompassWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATCompass, Log, All);

#define LOCTEXT_NAMESPACE "Cardinals"

void UTATCompassWidget::NativeConstruct()
{
   Super::NativeConstruct();

   ATATWorldSettings* worldSettings = ATATWorldSettings::GetTATWorldSettings(this);
   check(IsValid(worldSettings));
   _worldCardinalAxes = worldSettings->WorldCardinalAxes;

   // Initialize cardinal indicators
   // Note: In our in game maps, North is pointed towards the -Y Axis (Global Left Vector)
   const FVector south = _worldCardinalAxes.GetRightVector();
   const FVector north = south * -1;
   const FVector east = _worldCardinalAxes.GetForwardVector();
   const FVector west = east * -1;
   const FVector northEast = north + east;
   const FVector northWest = north + west;
   const FVector southEast = south + east;
   const FVector southWest = south + west;

   // TODO: move text to a string table for localization
   ATATCardinalDirectionIndicator* northIndicator = _SpawnCardinalDirectionIndicator(north, INVTEXT("N"));
   ATATCardinalDirectionIndicator* southIndicator = _SpawnCardinalDirectionIndicator(south, INVTEXT("S"));
   ATATCardinalDirectionIndicator* eastIndicator = _SpawnCardinalDirectionIndicator(east, INVTEXT("E"));
   ATATCardinalDirectionIndicator* westIndicator = _SpawnCardinalDirectionIndicator(west, INVTEXT("W"));
   ATATCardinalDirectionIndicator* northEastIndicator = _SpawnCardinalDirectionIndicator(northEast, INVTEXT("NE"));
   ATATCardinalDirectionIndicator* northWestIndicator = _SpawnCardinalDirectionIndicator(northWest, INVTEXT("NW"));
   ATATCardinalDirectionIndicator* southEastIndicator = _SpawnCardinalDirectionIndicator(southEast, INVTEXT("SE"));
   ATATCardinalDirectionIndicator* southWestIndicator = _SpawnCardinalDirectionIndicator(southWest, INVTEXT("SW"));

   if (UTATCompassRegistrySubsystem* registry = GetWorld()->GetSubsystem<UTATCompassRegistrySubsystem>())
   {
      registry->OnIndicatorAdded.AddUObject(this, &ThisClass::_RegisterIndicator);
      registry->OnIndicatorRemoved.AddUObject(this, &ThisClass::_DeregisterIndicator);
      for (TWeakObjectPtr<ATATGenericIndicator> weakIndicator : registry->GetIndicators())
      {
         if (ATATGenericIndicator* indicator = weakIndicator.Get())
         {
            _RegisterIndicator(indicator);
         }
      }
   }
}

void UTATCompassWidget::NativeDestruct()
{
   // Actually, seems safest not to unregister listeners here, and just lean on the listeners failing when fully gone. Lifetimes should be similar
   Super::NativeDestruct();
}

void UTATCompassWidget::NativeTick(const FGeometry& myGeometry, float inDeltaTime)
{
   Super::NativeTick(myGeometry, inDeltaTime);

   _UpdatePOIs();
   _UpdateScrollingCompassBG();
}

void UTATCompassWidget::_SetCompassPOIPosition(UTATCompassPOIWidget* poi, const float position)
{
   check(poi);
   if(UCanvasPanelSlot* canvasSlot = Cast<UCanvasPanelSlot>(poi->Slot))
   {
      if(!FMath::IsNearlyEqual(canvasSlot->GetPosition().X, position, 0.1f))
      {
         canvasSlot->SetPosition(FVector2D(position, 0));
      }
   }
}

void UTATCompassWidget::_UpdateIndicatorCloseToPlayer(ATATGenericIndicator* genericIndicator, bool isInitializing)
{
   if (genericIndicator->GetCloseRangeDetectionMode() == ETATIndicatorCloseRangeDetectionMode::Distance)
   {
      const UTATProjectSettings& tatSettings = UTATProjectSettings::Get();
      const bool isInCloseRange = _IsPlayerWithinRange(genericIndicator, GetOwningPlayer(), tatSettings.MaxDistanceToVisibleMapAndCompassActors);
      if (isInCloseRange != genericIndicator->GetPlayerInCloseRange() || isInitializing)
      {
         genericIndicator->SetPlayerInCloseRange(isInCloseRange, isInitializing);
         HandlePlayerInCloseRange(genericIndicator, isInCloseRange);
      }
   }
}

void UTATCompassWidget::_UpdatePOIs()
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATCompass_UpdatePOIs);
   for (const TPair<TObjectPtr<ATATGenericIndicator>, TObjectPtr<UTATCompassPOIWidget>>& pair : _indicatorToPOIMap)
   {
      ATATGenericIndicator* genericIndicator = pair.Key;
      UTATCompassPOIWidget* compassPOI = pair.Value;

      check(IsValid(genericIndicator));

      const FTATIndicatorAppearanceData appearanceData = genericIndicator->GetIndicatorAppearanceData();
      FTATCompassPOIZSortPriorityBucket* priorityBucket = &_poiZSortPriorityMap[appearanceData.CompassZSortPriority];
      check(priorityBucket != nullptr);

      if (_poiCategoriesToOmit.Contains(appearanceData.POICategory))
      {
         genericIndicator->SetIndicatorEnabled(false);
      }
      else
      {
         // Only handle player in/out of close range behavior if they are not being omitted.
         // Handle player in/out of close range behavior before checking if POI is enabled.
         // This is because we may have some indicators which enable/disable themselves depending on whether they are in close range or not.
         _UpdateIndicatorCloseToPlayer(genericIndicator);
      }

      if (!genericIndicator->GetIndicatorEnabled())
      {
         // There seem to be some circumstances where the indicator is disabled, but the widget is still visible
         // This function only changes the slate widget if visibility has changed, so shouldn't dirty anything
         compassPOI->SetPOIVisible(false);

         // Remove from z-sorting bucket if indicator is disabled to speed up subsequent map sort
         if (priorityBucket->POIToDistanceMap.Contains(compassPOI))
         {
            priorityBucket->POIToDistanceMap.Remove(compassPOI);
         }
         continue;
      }

      const FVector playerToIndicator = _GetPlayerToIndicatorVector(genericIndicator, GetOwningPlayer());
      const float distanceFromPlayer = playerToIndicator.Size();

      if (compassPOI->IsEnabled())
      {
         _RefreshPOIAppearance(compassPOI, genericIndicator, playerToIndicator, distanceFromPlayer);
      }
      else
      {
         compassPOI->SetPOIVisible(false);
      }

      // Add/update sort priority map entry with distance from player
      float* distance = priorityBucket->POIToDistanceMap.Find(compassPOI);
      if (distance == nullptr)
      {
         priorityBucket->POIToDistanceMap.Add(compassPOI, distanceFromPlayer);
      }
      else
      {
         *distance = distanceFromPlayer;
      }
   }

   _SortPOIZOrder();
}

void UTATCompassWidget::_RefreshPOIAppearance(UTATCompassPOIWidget* compassPOI, const ATATGenericIndicator* genericIndicator, const FVector playerToIndicator, const float distanceFromPlayer)
{
   // Calculate compass POI position based on direction relative to player camera orientation
   bool outIsOnScreen;
   const float poiCompassPosition = _CalculatePOICompassPosition(genericIndicator, outIsOnScreen);

   // Make sure not to override fixed compass position setting when player is in close range
   const FTATIndicatorAppearanceData appearanceData = genericIndicator->GetIndicatorAppearanceData();
   const bool fixedCompassPosition = appearanceData.CloseRangeDisplayMode == ETATIndicatorCloseRangeDisplayMode::ShowOnCompassFixed && genericIndicator->GetPlayerInCloseRange();
   if (!fixedCompassPosition)
   {
      _SetCompassPOIPosition(compassPOI, poiCompassPosition);
   }
   else
   {
      // Manually enable this flag, since we're not relying on _CalculatePOICompassPosition's results for fixed-position close range display
      outIsOnScreen = true;
   }

   bool shouldBeVisible = true;
   // Handle off-screen behavior for visible POIs
   if (!outIsOnScreen)
   {
      _HandleIndicatorOffScreen(compassPOI, genericIndicator, poiCompassPosition < 0.f);
      shouldBeVisible = genericIndicator->GetIndicatorAppearanceData().ShowOffScreen;
   }
   else
   {
      shouldBeVisible = true;
      compassPOI->ClearPOIOffScreenIndicator();
   }

   // Show vertical displacement if exceeding threshold
   if (appearanceData.UseVerticalDisplacement)
   {
      const float verticalDisplacement = playerToIndicator.Z;
      const UTATProjectSettings& tatSettings = UTATProjectSettings::Get();
      if (FMath::Abs(verticalDisplacement) > tatSettings.VerticalDisplacementThreshold)
      {
         ETATIndicatorOutOfVerticalRangeBehavior OutOfVerticalRangeBehavior = genericIndicator->GetIndicatorOutOfVerticalRangeBehavior();
         if (OutOfVerticalRangeBehavior == ETATIndicatorOutOfVerticalRangeBehavior::DisplayDirectionalArrow)
         {
            // Sets the verticality arrow in BP
            _HandleIndicatorAboveBelowPlayer(compassPOI, genericIndicator, verticalDisplacement);
         }
         else if (OutOfVerticalRangeBehavior == ETATIndicatorOutOfVerticalRangeBehavior::RemoveFromCompass)
         {
            shouldBeVisible = false;
         }
      }
      else
      {
         compassPOI->ClearPOIVerticalityIndicator();
         shouldBeVisible = true;
      }
   }

   if (appearanceData.HideWhenOutOfCloseRange && !genericIndicator->GetPlayerInCloseRange())
   {
      shouldBeVisible = false;
   }
   compassPOI->SetPOIVisible(shouldBeVisible);

   // Update POI sprite for "special intel" indicators
   if (appearanceData.ConcealSpriteUntilCloseRange && genericIndicator->GetPlayerInCloseRange())
   {
      const bool isInRevealSpriteRange = _IsPlayerWithinRange(genericIndicator, GetOwningPlayer(), appearanceData.ConcealedIndicatorRange);
      compassPOI->SetPOISprite(isInRevealSpriteRange ? appearanceData.IndicatorSprite : appearanceData.ConcealedIndicatorSprite);
   }

   // Show distance from player
   const bool showDistance = outIsOnScreen && appearanceData.ShowDistance;
   compassPOI->SetDistanceVisible(showDistance);
   if (showDistance)
   {
      compassPOI->UpdateDistance(distanceFromPlayer);
   }
}

void UTATCompassWidget::_UpdateScrollingCompassBG()
{
   // Update scrolling compass BG
   const APlayerController* playerController = GetOwningPlayer();
   check(playerController && playerController->IsLocalController());

   // Get signed angle between camera forward and camera -> indicator
   const float playerCardinalSignedAngle = _GetCameraForwardToWorldNorthSignedAngle(playerController);
   _HandleUpdateCompassBackground(playerCardinalSignedAngle);
}

void UTATCompassWidget::_SortPOIZOrder()
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATCompassWidget_SortPOIZOrder);

   // Sort buckets by priority (low -> high)
   _poiZSortPriorityMap.KeySort([](int priorityA, int priorityB) { return priorityA < priorityB; });

   // Iterate over sort-priority map and update Z-sorting accordingly
   uint32 zOrder = 0;
   for (TPair<int32, FTATCompassPOIZSortPriorityBucket>& priorityToBucket : _poiZSortPriorityMap)
   {
      FTATCompassPOIZSortPriorityBucket* sortPriorityBucket = &priorityToBucket.Value;

      // Sort POIs within same priority bucket by distance (far -> close)
      sortPriorityBucket->POIToDistanceMap.ValueSort([](const float distA, const float distB) { return distA > distB; });
      for (auto& poiToDistance : sortPriorityBucket->POIToDistanceMap)
      {
         UTATCompassPOIWidget* poiWidget = poiToDistance.Key;

         UCanvasPanelSlot* panelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(poiWidget);
         check(IsValid(panelSlot));

         // Asisgn z-order and increment
         if(panelSlot->GetZOrder() != zOrder)
         {
            panelSlot->SetZOrder(zOrder);
         }
         zOrder++;
      }
   }
}

void UTATCompassWidget::_AddPOIToZSortPriorityMap(UTATCompassPOIWidget* poi, const int zSortPriority)
{
   check(IsValid(poi));

   // Check if entry already exists in a different priority bucket, remove if present
   for (auto& priorityToBucket : _poiZSortPriorityMap)
   {
      FTATCompassPOIZSortPriorityBucket* bucket = &priorityToBucket.Value;

      if (bucket->POIToDistanceMap.Contains(poi))
      {
         // Exit if POI is already in the correct bucket
         if (priorityToBucket.Key == zSortPriority)
         {
            return;
         }
         bucket->POIToDistanceMap.Remove(poi);
      }
   }

   // Init distance to 0, since it'll be recalculated in _UpdatePOIs()
   const float distance = 0.f;

   if (FTATCompassPOIZSortPriorityBucket* sortPriorityBucket = _poiZSortPriorityMap.Find(zSortPriority))
   {
      // Add to matching priority bucket if one exists
      sortPriorityBucket->POIToDistanceMap.Add(poi, distance);
   }
   else
   {
      // Otherwise create and add to new priority bucket
      FTATCompassPOIZSortPriorityBucket newSortPriorityBucket(poi, distance);
      _poiZSortPriorityMap.Add(zSortPriority, newSortPriorityBucket);
   }

   UE_LOG(LogTATCompass, Verbose, TEXT("Added POI %s to z-sort priority bucket %d")
      , *poi->GetName()
      , zSortPriority);
}

void UTATCompassWidget::_RegisterIndicator(ATATGenericIndicator* genericIndicator)
{
   UE_LOG(LogTATCompass, Verbose, TEXT("TATCompassWidget - Initializing generic indicator %s..."), *genericIndicator->GetName());

   UTATCompassPOIWidget* compassPOI = _GetPOIWidgetForIndicator(genericIndicator);
   if (!IsValid(compassPOI))
   {
      compassPOI = _ConstructIndicatorPOI(genericIndicator);
      check(IsValid(compassPOI));
      
      UE_LOG(LogTATCompass, Verbose, TEXT("Created compassPOI (%s) for indicator (%s) and added to map"), *compassPOI->GetName(), *genericIndicator->GetName());

      // Add POI to map and disable indicator (should be enabled manually, or via objective activation)
      _indicatorToPOIMap.Add(genericIndicator, compassPOI);
      compassPOI->SetPOIVisible(genericIndicator->GetIndicatorEnabled());
      _UpdateIndicatorCloseToPlayer(genericIndicator, true);
   }
   else
   {
      UE_LOG(LogTATCompass, Verbose, TEXT("compassPOI already exists for indicator %s"), *genericIndicator->GetName());
   }

   // Add/update entry in z-sort priority map
   const int zSortPriority = genericIndicator->GetIndicatorAppearanceData().CompassZSortPriority;
   _AddPOIToZSortPriorityMap(compassPOI, zSortPriority);
}

void UTATCompassWidget::_DeregisterIndicator(ATATGenericIndicator* genericIndicator)
{
   UTATCompassPOIWidget* compassPOI = _GetPOIWidgetForIndicator(genericIndicator);
   if (!IsValid(compassPOI))
   {
      return;
   }

   // Remove POI from z-sort priority bucket
   check(IsValid(genericIndicator));
   if (FTATCompassPOIZSortPriorityBucket* sortPriorityBucket = _poiZSortPriorityMap.Find(genericIndicator->GetIndicatorAppearanceData().CompassZSortPriority))
   {
      sortPriorityBucket->POIToDistanceMap.Remove(compassPOI);
   }

   // Update indicator -> POI mapping
   _indicatorToPOIMap.Remove(genericIndicator);

   // Destroy POI if not used by any other indicators
   TArray<TObjectPtr<UTATCompassPOIWidget>> compassPOIs;
   _indicatorToPOIMap.GenerateValueArray(compassPOIs);
   if (!compassPOIs.Contains(compassPOI))
   {
      // Let bp remove the widget from it's container
      _DestructIndicatorPOI(compassPOI);
   }

   return;
}

bool UTATCompassWidget::SetPOIEnabledForIndicator(const ATATGenericIndicator* genericIndicator, bool isEnabled)
{
   if (UTATCompassPOIWidget* compassPOI = _GetPOIWidgetForIndicator(genericIndicator))
   {
      compassPOI->SetPOIVisible(isEnabled);
      return true;
   }
   return false;
}

// static
UTATCompassWidget* UTATCompassWidget::TryGetCompass(const UObject* contextObj)
{
   const ATATHUD* hud = ATATHUD::TryGetLocalTATHUD(contextObj);
   if (IsValid(hud) && hud->HasActorBegunPlay())
   {
      return hud->GetTATCompass();
   }
   return nullptr;
}

ATATCardinalDirectionIndicator* UTATCompassWidget::_SpawnCardinalDirectionIndicator(const FVector relativeOffset, const FText text) const
{
   ATATCardinalDirectionIndicator* cardinalIndicator = GetWorld()->SpawnActorDeferred<ATATCardinalDirectionIndicator>(_cardinalIndicatorClass, FTransform());
   cardinalIndicator->SetCardinality(relativeOffset, text);

   cardinalIndicator->FinishSpawning(FTransform());
   return cardinalIndicator;
}

float UTATCompassWidget::_CalculatePOICompassPosition(const ATATGenericIndicator* genericIndicator, bool& outIsOnScreen) const
{
   const APlayerController* playerController = GetOwningPlayer();
   check(playerController && playerController->IsLocalController());

   const APlayerCameraManager* cameraManager = playerController->PlayerCameraManager;
   check(IsValid(cameraManager));

   // Get signed angle (-180 <-> 180 degrees) between camera forward and camera -> indicator
   const float angle = _GetCameraForwardToIndicatorSignedAngle(genericIndicator, playerController);

   // Get angle that encompasses the compass screen boundaries
   const float compassBoundaryAngle = _GetCompassScreenWidthNormalized() * cameraManager->GetFOVAngle();

   // Get left/right edges of compass widget
   const float maxCompassPosition = _GetCompassScreenWidth() / 2.f;
   const float minCompassPosition = maxCompassPosition * -1.f;

   // Check if angle to indicator falls within compass bounds
   outIsOnScreen = FMath::Abs(angle) < compassBoundaryAngle / 2;
   if (!outIsOnScreen)
   {
      // Shortcut to left/right compass edges
      return angle < 0 ? minCompassPosition : maxCompassPosition;
   }

   // Inverse lerp angle against compass half-angles, generating a position between 0 and 1 (normalized to compass edges)
   const float normalizedCompassPosition = UKismetMathLibrary::NormalizeToRange(angle, -compassBoundaryAngle / 2, compassBoundaryAngle / 2);
   
   // Lerp + clamp resulting position to compass widget edges
   const float compassPosition = FMath::Lerp(minCompassPosition, maxCompassPosition, normalizedCompassPosition);
   return FMath::Clamp(compassPosition, minCompassPosition, maxCompassPosition);
}

float UTATCompassWidget::_GetCompassScreenWidth(bool absolute) const
{
   check(CompassSizeBox);
   const FVector2d size = absolute
      ? CompassSizeBox->GetCachedGeometry().GetAbsoluteSize()
      : CompassSizeBox->GetCachedGeometry().GetLocalSize();
   return size.X;
}

int UTATCompassWidget::GetActivePOICount() const
{
   int activePOIs = 0;
   for (const TPair<TObjectPtr<ATATGenericIndicator>, TObjectPtr<UTATCompassPOIWidget>>& pair : _indicatorToPOIMap)
   {
      if (pair.Key && pair.Key->GetIndicatorEnabled())
      {
         activePOIs++;
      }
   }
   return activePOIs;
}

UTATCompassPOIWidget* UTATCompassWidget::_GetPOIWidgetForIndicator(const ATATGenericIndicator* genericIndicator) const
{
   if (_indicatorToPOIMap.Contains(genericIndicator))
   {
      return _indicatorToPOIMap[genericIndicator];
   }
   UE_LOG(LogTATCompass, Verbose, TEXT("_GetPOIWidgetForIndicator() - Could not find compassPOI associated with indicator (%s)"), *genericIndicator->GetName());
   return nullptr;
}

float UTATCompassWidget::_GetCompassScreenWidthNormalized() const
{
   const APlayerController* playerController = GetOwningPlayer();
   check(playerController->IsLocalController());

   int outScreenWidth, outScreenHeight;
   playerController->GetViewportSize(outScreenWidth, outScreenHeight);

   // Return ratio between viewport width and (DPI-scaled) compass width
   const bool isAbsolute = true;
   return _GetCompassScreenWidth(isAbsolute) / outScreenWidth;
}

void UTATCompassWidget::_HandleIndicatorOffScreen(UTATCompassPOIWidget* poi, const ATATGenericIndicator* genericIndicator, bool isLeftOfScreen)
{
   check(poi);
   check(genericIndicator);
   if(genericIndicator->GetIndicatorAppearanceData().ShowOffScreen)
   {
      poi->SetPOIOffScreenIndicator(isLeftOfScreen);
   }
   else
   {
      if(poi->GetVisibility() != ESlateVisibility::Hidden)
      {
         poi->SetVisibility(ESlateVisibility::Hidden);
      }
   }
}

void UTATCompassWidget::_HandleIndicatorAboveBelowPlayer(UTATCompassPOIWidget* poi, const ATATGenericIndicator* genericIndicator,
   float verticalDisplacement)
{
   check(poi);
   check(genericIndicator);
   const UTATProjectSettings& tatSettings = UTATProjectSettings::Get();
   if(tatSettings.VerticalDisplacementThreshold < FMath::Abs(verticalDisplacement))
   {
      poi->SetPOIVerticalityIndicator(verticalDisplacement > 0);
   }
   else
   {
      poi->ClearPOIVerticalityIndicator();
   }
}

void UTATCompassWidget::_HandleUpdateCompassBackground(const float playerCardinalSignedAngle)
{
   if(CompassBGMaterialInstance)
   {
      CompassBGMaterialInstance->SetScalarParameterValue(UVScrollPropertyName, playerCardinalSignedAngle / -360.f);
      CompassBGMaterialInstance->SetScalarParameterValue(FOVPropertyName, GetOwningPlayerCameraManager()->GetFOVAngle());
   }
}

bool UTATCompassWidget::_IsPlayerWithinRange(const ATATGenericIndicator* genericIndicator, const APlayerController* playerController, const float range) const
{
   const float sqrDistance = _GetPlayerToIndicatorVector(genericIndicator, playerController).SizeSquared();
   return sqrDistance < range* range;
}

float UTATCompassWidget::_GetCameraForwardToIndicatorSignedAngle(const ATATGenericIndicator* genericIndicator, const APlayerController* controller) const
{
   // Forward vector from camera
   check(IsValid(controller));
   FVector eyePos;
   FRotator eyeRot; 
   controller->GetActorEyesViewPoint(eyePos, eyeRot);

   const FVector cameraForward = eyeRot.Quaternion().GetForwardVector();
   const FVector cameraRight = eyeRot.Quaternion().GetRightVector();

   // Camera -> indicator vector
   const FVector cameraToIndicator = genericIndicator->GetIndicatorLocation() - eyePos;

   // Angle between two vectors, excluding camera pitch
   float angle = FMath::Acos(cameraForward.CosineAngle2D(cameraToIndicator));
   angle = FMath::RadiansToDegrees(angle);

   // Apply directionality of angle - is it to the left/right of camera forward?
   const float sign = FMath::Sign(FVector::DotProduct(cameraRight, cameraToIndicator));
   return angle * sign;
}

float UTATCompassWidget::_GetCameraForwardToWorldNorthSignedAngle(const APlayerController* controller) const
{
   // Get camera forward/right vectors
   check(IsValid(controller));
   const FQuat cameraOrientation = _GetCameraOrientation(controller);
   const FVector cameraForward = cameraOrientation.GetForwardVector();
   const FVector cameraRight = cameraOrientation.GetRightVector();

   // Get angle from camera-forward to north (flattened to X/Y plane)
   const FVector worldNorth = _worldCardinalAxes.GetForwardVector();
   float angle = FMath::Acos(cameraForward.CosineAngle2D(worldNorth));
   angle = FMath::RadiansToDegrees(angle);

   // Apply directionality of angle - is it to the left/right of camera forward?
   const float sign = FMath::Sign(FVector::DotProduct(cameraRight, worldNorth));
   return angle * sign;
}

FQuat UTATCompassWidget::_GetCameraOrientation(const APlayerController* controller) const
{
   // Forward vector from camera
   check(IsValid(controller));
   FVector eyePos;
   FRotator eyeRot;
   controller->GetActorEyesViewPoint(eyePos, eyeRot);

   return eyeRot.Quaternion();
}

FVector UTATCompassWidget::_GetPlayerToIndicatorVector(const ATATGenericIndicator* genericIndicator, const APlayerController* playerController) const
{
   FVector outPlayerLocation;
   FRotator outPlayerRotation;
   playerController->GetPlayerViewPoint(outPlayerLocation, outPlayerRotation);

   return genericIndicator->GetIndicatorLocation() - outPlayerLocation;
}

#undef LOCTEXT_NAMESPACE
