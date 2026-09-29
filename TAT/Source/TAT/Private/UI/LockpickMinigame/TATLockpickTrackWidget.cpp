// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/LockpickMinigame/TATLockpickTrackWidget.h"

// tat
#include "Lockpicking/LockpickableInterface.h"
#include "Lockpicking/TATLockpickingSettings.h"

// ue
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLockpickTrackWidget)
DEFINE_LOG_CATEGORY_STATIC(LogTATLockpickTrackWidget, Log, All);

UTATLockpickTrackWidget::UTATLockpickTrackWidget()
{
   // Should never be focusable - we want to restrict input to the ability system
   // (This widget should never be on a screen (which focus is restricted to), but best to be explicit)
   SetIsFocusable(false);
}

void UTATLockpickTrackWidget::RemoveFromParent()
{
   if (_trackEndWidget != nullptr)
   {
      _trackEndWidget->RemoveFromParent();
      _trackEndWidget = nullptr;
   }
   Super::RemoveFromParent();
}

int32 UTATLockpickTrackWidget::NativePaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const
{
   // This function should return the maximum LayerID painted on
   layerId = Super::NativePaint(args, allottedGeometry, cullingRect, outDrawElements, layerId, widgetStyle, parentEnabled);

   // the paint context keeps a _reference_ to the layer id, so it will stay up to date
   FOSERadialPaintContext paintCtx{ layerId, allottedGeometry, cullingRect, outDrawElements, widgetStyle, parentEnabled };

   const float angleOffset = GetNormalizedTrackProgress() * _trackMaxAngle - _trackStartingAngle;
   for (TArray<FOSERadialDiscShape>::TConstIterator itShape = _trackShapes.CreateConstIterator(); itShape; ++itShape)
   {
      const ESlateDrawEffect drawEffect = itShape->PremultipliedAlpha ? ESlateDrawEffect::PreMultipliedAlpha : ESlateDrawEffect::None;
      const FFloatInterval ArcRangeDegrees = { itShape->ArcRangeDegrees.Min - angleOffset, itShape->ArcRangeDegrees.Max - angleOffset };
      paintCtx.DrawDisc(FVector2f(itShape->Origin), itShape->Radius, ArcRangeDegrees, itShape->Brush, itShape->Resolution, NullOpt, itShape->UVMode, drawEffect);
   }

   return layerId;
}

void UTATLockpickTrackWidget::BuildTrack(const FTATLockpickTrackLayerManager& trackLayerStack, const FTATLockpickTrackBuildParams& trackBuildParams, float trackSectionRadiusOffset, float maxTrackAngle)
{
   check(trackLayerStack.GetSectionCount() > 0);

   _trackDuration = trackLayerStack.GetDurationSeconds();
   check(!FMath::IsNearlyZero(_trackDuration));
   
   check(!FMath::IsNearlyZero(maxTrackAngle));
   _trackMaxAngle = maxTrackAngle;

   _trackStartingAngle = trackBuildParams.TrackStartingAngle;

   _trackRadius = trackBuildParams.TrackStartingRadius + trackSectionRadiusOffset;

   _trackShapes.Empty(trackLayerStack.GetSectionCount());

   const UTATLockpickingSettings& lockpickSettings = UTATLockpickingSettings::GetLockpickingSettingsRef();

   FFloatInterval sectionRadius;
   sectionRadius.Min = _trackRadius - trackBuildParams.TrackWidth / 2.0f;
   sectionRadius.Max = _trackRadius + trackBuildParams.TrackWidth / 2.0f;

   for (FTATLockpickTrackLayerManager::FTATConstSectionIterator itSection = trackLayerStack.CreateConstIterator(); itSection; ++itSection)
   {
      const FTATLockpickTrackSection& section = *itSection;

      FOSERadialDiscShape sectionShape = trackBuildParams.SectionTypeToShape.FindChecked(section.SectionType);
      sectionShape.Radius = sectionRadius;

      // Map normalized times to degrees
      sectionShape.ArcRangeDegrees.Min = itSection.GetCurrentSectionStartTime() / _trackDuration * _trackMaxAngle;
      sectionShape.ArcRangeDegrees.Max = itSection.GetNextSectionStartTime() / _trackDuration * _trackMaxAngle;

      _trackShapes.Add(sectionShape);
   }

   if (_trackMaxAngle < 360.0f)
   {
      FOSERadialDiscShape sectionShape = trackBuildParams.SectionTypeToShape.FindChecked(lockpickSettings.EmptyTrackSectionType);
      sectionShape.Radius = sectionRadius;
      sectionShape.ArcRangeDegrees.Min = _trackMaxAngle;
      sectionShape.ArcRangeDegrees.Max = 360.0f;
      _trackShapes.Add(sectionShape);
   }

   // Create the track end widget if class is specified and we haven't already
   if (_trackEndWidget == nullptr && trackBuildParams.TrackEndWidget.Get())
   {
      if (UOSEUserWidget* trackWidget = CreateWidget<UOSEUserWidget>(trackBuildParams.CanvasPanel, trackBuildParams.TrackEndWidget))
      {
         // add to the canvas panel
         if (trackBuildParams.CanvasPanel)
         {
            trackBuildParams.CanvasPanel->AddChild(trackWidget);
         }
         else
         {
            UE_LOG(LogTATLockpickTrackWidget, Error, TEXT("UTATLockpickTrackWidget::BuildTrack() called with an invalid CanvasPanel!"));
         }

         if (UCanvasPanelSlot* canvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(trackWidget))
         {
            canvasPanelSlot->SetAnchors(FAnchors(0.5f));
            canvasPanelSlot->SetSize(trackBuildParams.TrackEndWidgetSize);
         }

         // Set _trackEndWidget after CanvasPanel->AddChild() to ensure RemoveFromParent() doesn't nullify it
         _trackEndWidget = trackWidget;
         _trackEndWidgetSize = trackBuildParams.TrackEndWidgetSize;
         _UpdateTrackEndWidget();
      }
   }
}

void UTATLockpickTrackWidget::UpdateTrackProgress(float trackProgress)
{
   _trackProgress = trackProgress;

   // Update the end track widget when progress changes
   _UpdateTrackEndWidget();
}

void UTATLockpickTrackWidget::_UpdateTrackEndWidget()
{
   if (_trackEndWidget != nullptr)
   {
      // Find the ending location along the ring track
      const float angleOffset = GetNormalizedTrackProgress() * _trackMaxAngle - _trackStartingAngle;
      const float radians = FMath::DegreesToRadians(_trackMaxAngle - angleOffset);
      const FVector2D translation = FVector2D(FMath::Cos(radians), FMath::Sin(radians)) * _trackRadius;
      const FVector2D offset = _trackEndWidgetSize / 2.0f;
      _trackEndWidget->SetRenderTranslation(translation - offset);
   }
}

void FTATLockpickMultiTrackWidgetManager::BuildTracks(const FTATLockpickMinigameVariation& lockpickMinigameVariation, TScriptInterface<ILockpickableInterface> lockpickableActor, const FTATLockpickTrackBuildParams& trackBuildParams)
{
   check(lockpickableActor);

   // Clear out any existing tracks
   RemoveAllTracks();

   _trackWidgets.SetNum(lockpickMinigameVariation.GetTrackCount());
   _trackLayerStacks.SetNum(_trackWidgets.Num());

   // Find the next, uncompleted track
   int32 trackIndex = lockpickableActor->GetLockpickCurrentTrack();
   lockpickMinigameVariation.ValidateTrackIndex(trackIndex);
   while (trackIndex != INDEX_NONE)
   {
      FTATLockpickTrackLayerManager& trackLayerStack = _trackLayerStacks[trackIndex];

      const FTATLockpickMinigameTrackDefinition& trackDefinition = lockpickMinigameVariation.GetTrackDefinition(trackIndex);
      trackLayerStack.Build(trackDefinition);

      const float trackMaxAngle = lockpickMinigameVariation.GetTrackArcAngle(trackIndex);

      const int32 offsetIndex = TATLockpickTrackHelpers::FindTrackIndexOffset(
         trackBuildParams.StartingTrack, lockpickMinigameVariation.GetTrackCount(), trackIndex);
      const float trackSectionRadiusOffset = offsetIndex * (trackBuildParams.TrackWidth + trackBuildParams.PaddingBetweenTracks);

      if (UTATLockpickTrackWidget* trackWidget = CreateWidget<UTATLockpickTrackWidget>(trackBuildParams.CanvasPanel))
      {
         // add to the canvas panel
         if (trackBuildParams.CanvasPanel)
         {
            trackBuildParams.CanvasPanel->AddChild(trackWidget);
         }
         else
         {
            UE_LOG(LogTATLockpickTrackWidget, Error, TEXT("FTATLockpickMultiTrackWidgetManager::BuildTracks() called with an invalid CanvasPanel!"));
         }

         if (UCanvasPanelSlot* canvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(trackWidget))
         {
            canvasPanelSlot->SetAnchors(FAnchors(0.5f));
         }

         // BuildTrack() after CanvasPanel->AddChild() to ensure RemoveFromParent() doesn't nullify the tracl's end widget
         trackWidget->BuildTrack(trackLayerStack, trackBuildParams, trackSectionRadiusOffset, trackMaxAngle);
         _trackWidgets[trackIndex] = trackWidget;
      }

      lockpickMinigameVariation.ValidateTrackIndex(++trackIndex);
   }
}

void FTATLockpickMultiTrackWidgetManager::AddInteractionToTrack(int32 trackIndex, float interactionTime, const FTATLockpickMinigameVariation& lockpickMinigameVariation, const FTATLockpickTrackBuildParams& trackBuildParams)
{
   check(_trackWidgets.IsValidIndex(trackIndex));

   if (UTATLockpickTrackWidget* trackWidget = _trackWidgets[trackIndex])
   {
      FTATLockpickTrackLayerManager& trackLayerStack = _trackLayerStacks[trackIndex];
      trackLayerStack.AddInteractionTime(interactionTime);

      const float trackMaxAngle = lockpickMinigameVariation.GetTrackArcAngle(trackIndex);
      
      const int32 offsetIndex = TATLockpickTrackHelpers::FindTrackIndexOffset(
         trackBuildParams.StartingTrack, lockpickMinigameVariation.GetTrackCount(), trackIndex);
      const float trackSectionRadiusOffset = offsetIndex * (trackBuildParams.TrackWidth + trackBuildParams.PaddingBetweenTracks);

      trackWidget->BuildTrack(trackLayerStack, trackBuildParams, trackSectionRadiusOffset, trackMaxAngle);
   }
}

void FTATLockpickMultiTrackWidgetManager::UpdateTrackProgress(int32 trackIndex, float trackProgress)
{
   check(_trackWidgets.IsValidIndex(trackIndex));

   if (UTATLockpickTrackWidget* trackWidget = _trackWidgets[trackIndex])
   {
      trackWidget->UpdateTrackProgress(trackProgress);
   }
}

void FTATLockpickMultiTrackWidgetManager::RemoveTrack(int32 trackIndex)
{
   if (_trackWidgets.IsValidIndex(trackIndex) && _trackWidgets[trackIndex] != nullptr)
   {
      _trackWidgets[trackIndex]->RemoveFromParent();
      _trackWidgets[trackIndex] = nullptr;
   }
}

void FTATLockpickMultiTrackWidgetManager::RemoveAllTracks()
{
   for (int trackIndex = 0; trackIndex < _trackWidgets.Num(); trackIndex++)
   {
      RemoveTrack(trackIndex);
   }
   _trackWidgets.Empty();
}
