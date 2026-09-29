// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/LockpickMinigame/TATLockpickMinigameWidget.h"

// tat
#include "Lockpicking/TATLockpickingSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLockpickMinigameWidget)
DEFINE_LOG_CATEGORY_STATIC(LogTATLockpickMinigameWidget, Log, All);

struct FInputModeLockpickMinigame : public FInputModeDataBase
{
   /** Widget to focus */
   FInputModeLockpickMinigame& SetWidgetToFocus(TSharedPtr<SWidget> InWidgetToFocus) { WidgetToFocus = InWidgetToFocus; return *this; }

protected:
   TSharedPtr<SWidget> WidgetToFocus = nullptr;

   virtual void ApplyInputMode(FReply& SlateOperations, class UGameViewportClient& GameViewportClient) const override
   {
      // Mimics FInputModeUIOnly::ApplyInputMode, but without the ReleaseMouseCaptor call which enables visibility of the mouse
      // for at least a frame (presumably until the cursor can be pulled from the hovered widget on the next frame). 
      // Since we ignore input in the game viewport, retaining the captor shouldn't matter.

      TSharedPtr<SViewport> ViewportWidget = GameViewportClient.GetGameViewportWidget();
      if (ViewportWidget.IsValid())
      {
         const bool bLockMouseToViewport = false;
         SetFocusAndLocking(SlateOperations, WidgetToFocus, bLockMouseToViewport, ViewportWidget.ToSharedRef());

         GameViewportClient.SetMouseLockMode(EMouseLockMode::DoNotLock);
         GameViewportClient.SetIgnoreInput(true);
         GameViewportClient.SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
      }
   }
};

UTATLockpickMinigameWidget::UTATLockpickMinigameWidget()
{
   // Should never be focusable - we want to restrict input to the ability system
   // (This widget should never be on a screen (which focus is restricted to), but best to be explicit)
   SetIsFocusable(false);
}

int32 UTATLockpickMinigameWidget::NativePaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const
{
   // This function should return the maximum LayerID painted on
   layerId = Super::NativePaint(args, allottedGeometry, cullingRect, outDrawElements, layerId, widgetStyle, parentEnabled);

   if (IsDesignTime() && _previewLockpickTrack)
   {
      // the paint context keeps a _reference_ to the layer id, so it will stay up to date
      FOSERadialPaintContext paintCtx{ layerId, allottedGeometry, cullingRect, outDrawElements, widgetStyle, parentEnabled };

      auto pointOnCircleDegrees = [](const FVector2f& circleOrigin, float circleRadius, float angleDegrees) -> FVector2f
      {
         return {
            circleOrigin.X + (FMath::Cos(FMath::DegreesToRadians(angleDegrees)) * circleRadius),
            circleOrigin.Y + (FMath::Sin(FMath::DegreesToRadians(angleDegrees)) * circleRadius),
         };
      };

      auto drawSection = [&](const FGameplayTag& tag, FOSERadialDiscShape shape, const TOptional<FFloatInterval>& arcRange)
      {
         const FVector2f originOffset = paintCtx.GetLocalSize() / 2.0f;

         shape.Radius.Min = _trackBuildParams.TrackStartingRadius - _trackBuildParams.TrackWidth / 2.0f;
         shape.Radius.Max = _trackBuildParams.TrackStartingRadius + _trackBuildParams.TrackWidth / 2.0f;
         if (arcRange)
         {
            shape.ArcRangeDegrees = *arcRange;
         }

         paintCtx.DrawDisc(shape, originOffset);

         if (_previewTrackSectionNames)
         {
            const FVector2f textOrigin = pointOnCircleDegrees(
               FVector2f(shape.Origin) + originOffset,
               FMath::Lerp(shape.Radius.Min, shape.Radius.Max, 0.0f),
               FMath::Lerp(shape.ArcRangeDegrees.Min, shape.ArcRangeDegrees.Max, 0.5f));
            paintCtx.DrawText(textOrigin, tag.ToString());
         }
      };

      if (_previewSingleTrackSection)
      {
         if (const FOSERadialDiscShape* previewDiscShape = _trackBuildParams.SectionTypeToShape.Find(_previewSingleTrackSectionType))
         {
            drawSection(_previewSingleTrackSectionType, *previewDiscShape, NullOpt);
         }
      }
      else
      {
         const float angleInterval = 180.0f / static_cast<float>(_trackBuildParams.SectionTypeToShape.Num());
         FFloatInterval arcRange{ 0.0f, angleInterval };
         for (const auto& pair : _trackBuildParams.SectionTypeToShape)
         {
            drawSection(pair.Key, pair.Value, arcRange);
            arcRange.Min += angleInterval;
            arcRange.Max += angleInterval;
         }
      }
   }

   return layerId;
}

void UTATLockpickMinigameWidget::StartMinigame(const FTATLockpickMinigameVariation& lockpickMinigameVariation, TScriptInterface<ILockpickableInterface> lockpickableActor, UCanvasPanel* multiTrackCanvasPanel)
{
   _lockpickableActor = lockpickableActor;
   _lockpickMinigameVariation = lockpickMinigameVariation;

   _trackBuildParams.CanvasPanel = multiTrackCanvasPanel;

   if (TScriptInterface<ILockpickableInterface> lockpickActor = GetLockpickableActor())
   {
      _multiTrackWidgetManager.BuildTracks(_lockpickMinigameVariation, lockpickActor, _trackBuildParams);
   }
}

void UTATLockpickMinigameWidget::StopMinigame()
{
   _multiTrackWidgetManager.RemoveAllTracks();
}

void UTATLockpickMinigameWidget::InitializeFocus()
{
   FInputModeLockpickMinigame inputMode;
   inputMode.SetWidgetToFocus(TakeWidget());
   GetOwningPlayer()->SetInputMode(inputMode);
}

void UTATLockpickMinigameWidget::UpdateTrackProgress(int32 trackIndex, float trackProgress)
{
   _multiTrackWidgetManager.UpdateTrackProgress(trackIndex, trackProgress);
}

FVector2D UTATLockpickMinigameWidget::GetTrackInteractLocation(int32 trackIndex) const
{
   FVector2D location = FVector2D::ZeroVector;
   if (trackIndex != INDEX_NONE)
   {
      const int32 offsetIndex = TATLockpickTrackHelpers::FindTrackIndexOffset(
         _trackBuildParams.StartingTrack, _multiTrackWidgetManager.GetTrackCount(), trackIndex);
      const float trackSectionRadiusOffset = offsetIndex * (_trackBuildParams.TrackWidth + _trackBuildParams.PaddingBetweenTracks);
      const float offset = trackSectionRadiusOffset + _trackBuildParams.TrackStartingRadius;

      location.X = offset * FMath::Cos(FMath::DegreesToRadians(_trackBuildParams.TrackStartingAngle));
      location.Y = offset * FMath::Sin(FMath::DegreesToRadians(_trackBuildParams.TrackStartingAngle));
   }
   return location;
}

void UTATLockpickMinigameWidget::HandleInteractionTriggered(int32 trackIndex, float interactionTime)
{
   _multiTrackWidgetManager.AddInteractionToTrack(trackIndex, interactionTime, _lockpickMinigameVariation, _trackBuildParams);
}

void UTATLockpickMinigameWidget::HandleTrackFinished(int32 trackIndex)
{
   const UTATLockpickingSettings& lockpickSettings = UTATLockpickingSettings::GetLockpickingSettingsRef();
   if (lockpickSettings.DestroyTracksOnFinish)
   {
      _multiTrackWidgetManager.RemoveTrack(trackIndex);
   }
}

TScriptInterface<ILockpickableInterface> UTATLockpickMinigameWidget::GetLockpickableActor() const
{
   if (_lockpickableActor.IsValid())
   {
      return _lockpickableActor.ToScriptInterface();
   }

   UE_LOG(LogTATLockpickMinigameWidget, Warning, TEXT("GetLockpickableActor() called with invalid _lockpickableActor reference!"));
   return nullptr;
}
