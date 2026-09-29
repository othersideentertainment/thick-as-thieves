// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "TATWorldSpaceUserWidget.h"

// ue5
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWorldSpaceUserWidget)
DEFINE_LOG_CATEGORY_STATIC(LogTATWorldSpaceUserWidget, Log, All);

void UTATWorldSpaceUserWidget::NativeOnInitialized()
{
   _localPlayerController = GetOwningPlayer();
   check(_localPlayerController);   
   Super::NativeOnInitialized();
}

void UTATWorldSpaceUserWidget::SetIsEnabled(bool bInIsEnabled)
{
   Super::SetIsEnabled(bInIsEnabled);
   OnShouldBeVisible.Broadcast(GetIsEnabled());
}

void UTATWorldSpaceUserWidget::SetIsEnabledAndShown(bool isEnabledAndShown)
{
   if (isEnabledAndShown)
   {
      SetVisibility(ESlateVisibility::HitTestInvisible);
   }
   else
   {
      SetIsEnabled(false); 
   }
}

FVector2D UTATWorldSpaceUserWidget::GetWidgetSize_Implementation() const
{
   return FVector2D::Zero();
}

void UTATWorldSpaceUserWidget::TickScreenPosition_Implementation()
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_UTATWorldSpaceUserWidget_TickScreenPosition_Implementation);
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATWorldSpaceUserWidget::TickScreenPosition_Implementation)
   if(_sceneComponentToTrack == nullptr)
   {
      return;
   }
   if(_localPlayerController == nullptr)
   {
      UE_LOG(LogTATWorldSpaceUserWidget, Error, TEXT("playerController for %s is null, trying to set now"), *GetName());
      _localPlayerController = GetOwningPlayer();
      return;
   }
   
   const FVector actorWorldPosition = _sceneComponentToTrack->GetComponentLocation();

   // cache this off?
   int screenX, screenY;
   {
      TRACE_CPUPROFILER_EVENT_SCOPE(UTATWorldSpaceUserWidget::TickScreenPosition_Implementation_GetViewportSize)
      _localPlayerController->GetViewportSize(screenX, screenY);
   }
   const FVector2D screenSize = FVector2D(screenX, screenY);
   FVector2D screenPosition = GetScreenPositionFromWorldLocation(_localPlayerController,
                                                                 actorWorldPosition,
                                                               _bIsProjectedBackwards,
                                                                 _allowReverseProjectionOfWorldPosition);
   const FVector2D widgetSize = GetWidgetSize();
   
   const bool bIsNowClamped = _ClampLocation(screenPosition, widgetSize, screenSize);
   
   // The widget rotation should be set before we compensate for the pivot point being 0,0, so we can check exactly 
   // what edge we are along.
   _RotateWidget(screenPosition, screenSize, widgetSize);

   // We want to offset the screen position to have it centered based on the widget size (the pivot point is set to 0,0)
   screenPosition.X = screenPosition.X - (widgetSize.X * 0.5f);
   screenPosition.Y = screenPosition.Y - (widgetSize.Y * 0.5f);
   
   SetPositionInViewport(screenPosition, true);
   
   if(bIsNowClamped != _bIsWidgetClamped)
   {
      _bIsWidgetClamped = bIsNowClamped;
      OnWidgetClampStateChanged(bIsNowClamped);
   }
}

void UTATWorldSpaceUserWidget::OnWidgetClampStateChanged_Implementation(bool bIsClamped)
{
}

UWidget* UTATWorldSpaceUserWidget::GetWidgetToRotate_Implementation()
{
   return this;
}

FVector2D UTATWorldSpaceUserWidget::GetScreenPositionFromWorldLocation(APlayerController* playerController, FVector worldLocation, bool& bDidProjectBackwards, bool bAllowReverseProjection) const
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATWorldSpaceUserWidget::GetScreenPositionFromWorldLocation)
   if(playerController == nullptr)
   {
      UE_LOG(LogTATWorldSpaceUserWidget, Error, TEXT("playerController is null, maybe %s is being used outside of the hud?"), *GetName());
      return FVector2D::ZeroVector;
   }
   // Built in projection system doesn't work with points _behind_ the camera
   // https://forums.unrealengine.com/t/project-world-to-screen-location-stops-returning-a-value/57135/3
   FVector projected = FVector::ZeroVector;
   bDidProjectBackwards = false;
   const ULocalPlayer* const localPlayer = playerController->GetLocalPlayer();
   if (localPlayer && localPlayer->ViewportClient)
   {
      FSceneViewProjectionData projectionData;
      if (localPlayer->GetProjectionData(localPlayer->ViewportClient->Viewport, projectionData))
      {
         const FMatrix viewProjectionMatrix = projectionData.ComputeViewProjectionMatrix();
         const FIntRect viewRectangle = projectionData.GetConstrainedViewRect();

         FPlane result = viewProjectionMatrix.TransformFVector4(FVector4(worldLocation, 1.f));
         if (FMath::IsNearlyZero(result.W)) { result.W = 1.f; } // Prevent Divide By Zero
         if (result.W < 0.f) {
            bDidProjectBackwards = true;
            if(bAllowReverseProjection == false)
            {
               return FVector2D::ZeroVector;
            }
         }

         const float rhw = 1.f / FMath::Abs(result.W);
         projected = FVector(result.X, result.Y, result.Z) * rhw;
	
         // Normalize to 0..1 UI Space
         const float normX = (projected.X / 2.f) + 0.5f;
         const float normY = 1.f - (projected.Y / 2.f) - 0.5f;
 
         projected.X = static_cast<float>(viewRectangle.Min.X) + (normX * static_cast<float>(viewRectangle.Width()));
         projected.Y = static_cast<float>(viewRectangle.Min.Y) + (normY * static_cast<float>(viewRectangle.Height()));
      }
   }
   const FVector2D screenPosition = FVector2D(projected.X, projected.Y);
   return screenPosition;
}

bool UTATWorldSpaceUserWidget::_ClampLocation(FVector2D& inOutLocation, const FVector2D& widgetSize, const FVector2D& screenSize) const
{
   switch(_clampingType)
   {
      case EWorldWidgetClampingType::None:
      {
         return false;
      }
      case EWorldWidgetClampingType::ClampToCenterOfScreen:
      {
         return _ClampToCenterOfScreen(inOutLocation, widgetSize, screenSize);
      }
      case EWorldWidgetClampingType::ClampToEdgeOfScreen:
      {
         return _ClampToEdgeOfScreen(inOutLocation, widgetSize, screenSize);
      }
      default:
         checkNoEntry();
   }
   return false;
}

bool UTATWorldSpaceUserWidget::_ClampToEdgeOfScreen(FVector2D& inOutLocation, const FVector2D& widgetSize, const FVector2D& screenSize) const
{
   const FVector2D halfWidget = widgetSize * 0.5f;
   const FBox2D bounds = FBox2D(
      FVector2D(halfWidget.X, halfWidget.Y),
      FVector2D(screenSize.X - halfWidget.X, screenSize.Y - halfWidget.Y)
   );

   if(_bIsProjectedBackwards)
   {
      // I don't like this - but it's placeholder. Ideally we have a different visual treatment when the NPC's are _behind_ you.
      // Perhaps showing in the center of screen?
      const FVector2D center = screenSize * 0.5f;
      const FVector2D normal = (center - inOutLocation).GetSafeNormal();
      inOutLocation = inOutLocation * normal * 100000.f;
   }
   const bool bClamped = bounds.IsInsideOrOn(inOutLocation) == false;

   inOutLocation.X = FMath::Clamp(inOutLocation.X, bounds.Min.X, bounds.Max.X);
   inOutLocation.Y = FMath::Clamp(inOutLocation.Y, bounds.Min.Y, bounds.Max.Y);

   return bClamped;
}

bool UTATWorldSpaceUserWidget::_ClampToCenterOfScreen(FVector2D& inOutLocation, const FVector2D& widgetSize, const FVector2D& screenSize) const
{
   if(FMath::IsNearlyZero(screenSize.X) || FMath::IsNearlyZero(screenSize.Y))
   {
      //Early out to avoid divide by zero, consider it "clamped"
      return true;
   }

   const FVector2D midPoint = FVector2D(screenSize.X * 0.5f, screenSize.Y * 0.5f);
   const FVector2D halfWidget = widgetSize * 0.5f;
   const FBox2D bounds = FBox2D(
      FVector2D(halfWidget.X, halfWidget.Y),
      FVector2D(screenSize.X - halfWidget.X, screenSize.Y - halfWidget.Y)
   );

   const bool bClamped = bounds.IsInsideOrOn(inOutLocation) == false;
   
   if(bClamped == false && _bIsProjectedBackwards == false)
   {
      return false;
   }
   if (_clampToLowerHalfOfScreen)
   {
      if (inOutLocation.Y < midPoint.Y)
      {
         inOutLocation.Y = midPoint.Y;
      }
   }
   const FVector2D unclampedDirection = inOutLocation - midPoint;
   const float clampedDistance = _maxDistanceFromCenterOfScreen * screenSize.Y;

   const FVector2D unclampedNormalizedDirection = unclampedDirection.GetSafeNormal();
   const FVector2D clampedLocation = midPoint + unclampedNormalizedDirection * (clampedDistance);
   inOutLocation =  clampedLocation;
   return true;
}

void UTATWorldSpaceUserWidget::_RotateWidget(const FVector2D& inLocation, const FVector2D& screenSize, const FVector2D& widgetSize)
{
   UWidget* WidgetToRotate = GetWidgetToRotate();
   check(WidgetToRotate);
   switch (_rotationType)
   {
   case EWorldWidgetRotationType::None:
      if(FMath::IsNearlyZero(GetRenderTransformAngle()) == false)
      {
         WidgetToRotate->SetRenderTransformAngle(0.f);
      }
      return;
   case EWorldWidgetRotationType::SpecifyRotationBasedOnEdgeDirection:
      if (_bIsWidgetClamped == false)
      {
         WidgetToRotate->SetRenderTransformAngle(0.f);
      }
      else
      {
         _RotateWidgetToSpecifiedAmountBasedOnEdgeDirection(inLocation, screenSize, widgetSize);
      }
      return;
   case EWorldWidgetRotationType::RotateToFaceAwayFromCenterWhenAtEdge:
      {
         if(_bIsWidgetClamped == false)
         {
            // There used to be logic here to only set the render transform to 0 if the angle wasn't already at 0.
            // However, the render transform angle was returning as 0 despite not being rotated correctly in game.
            // I suspect we still need to call the UpdateRenderTransform() method within the widget, but that is a protected method.
            // It gets called if we set the render transform angle to 0 though, so we'll just call it here regardless of the redundancy.
            WidgetToRotate->SetRenderTransformAngle(0.f);
            return;
         }
         //Deliberate fall through to AlwaysRotateToFaceAwayFromCenter if the widget is clamped. 
      }
   case EWorldWidgetRotationType::AlwaysRotateToFaceAwayFromCenter:
      _RotateWidgetTowardsCenterOfScreen(inLocation, screenSize);
      return;
      default:
         checkNoEntry();
   }
}

void UTATWorldSpaceUserWidget::_RotateWidgetTowardsCenterOfScreen(const FVector2D& inLocation, const FVector2D& screenSize)
{
   UWidget* WidgetToRotate = GetWidgetToRotate();
   check(WidgetToRotate);
   const FVector2D midPoint = FVector2D(screenSize.X * 0.5f, screenSize.Y * 0.5f);
   FVector2D dir = (inLocation - midPoint);
   dir.Normalize();
   const FVector2D up = FVector2D(0,1.f);
   const float dot = dir.Dot(up);
   float degrees = FMath::RadiansToDegrees(FMath::Acos(dot));
   if(dir.X > 0)
   {
      degrees = -degrees;
   }
   WidgetToRotate->SetRenderTransformAngle(degrees);
}

void UTATWorldSpaceUserWidget::_RotateWidgetToSpecifiedAmountBasedOnEdgeDirection(
   const FVector2D& inLocation, 
   const FVector2D& screenSize, 
   const FVector2D& widgetSize)
{
   const FVector2D halfWidget = widgetSize * 0.5f;
   
   UWidget* widgetToRotate = GetWidgetToRotate();
   check(widgetToRotate);
   
   float degreesToRotate = 0.f;
   if (FMath::IsNearlyEqual(inLocation.Y, halfWidget.Y))
   {
      degreesToRotate = degreesAtSouthEdgeOfScreen;
   } 
   else if (FMath::IsNearlyEqual(inLocation.Y, screenSize.Y - halfWidget.Y))
   {
      degreesToRotate = degreesAtNorthEdgeOfScreen;
   }
   else
   {
      const FVector2D midPoint = FVector2D(screenSize.X * 0.5f, screenSize.Y * 0.5f);
      const FVector2D dir = (inLocation - midPoint).GetSafeNormal();
      const FVector2D east = FVector2D(1.f,0);
      const FVector2D west = FVector2D(-1.f,0);
      const float westDot = dir.Dot(west);
      const float eastDot = dir.Dot(east);   
      
      degreesToRotate = degreesAtWestEdgeOfScreen;
      if (eastDot > westDot)
      {
         degreesToRotate = degreesAtEastEdgeOfScreen;
      }
   }
  
   widgetToRotate->SetRenderTransformAngle(degreesToRotate);
}
