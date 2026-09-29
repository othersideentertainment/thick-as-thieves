// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Lockpicking/TATLockpickingTypes.h"
#include "UI/TATUserWidget.h"

// ose
#include "UI/OSERadialPaintLibrary.h"

// ue
#include "CoreMinimal.h"

#include "TATLockpickTrackWidget.generated.h"

class ILockpickableInterface;
class UCanvasPanel;

UENUM()
enum class ETATLockpickStartingTrack : uint8
{
   InnerRing,
   OuterRing
};

namespace TATLockpickTrackHelpers
{
   // helps find the display order of a given track index in the scope of a lockpicking configuration
   static int32 FindTrackIndexOffset(ETATLockpickStartingTrack startingTrack, const int32 trackCount, const int32 trackIndex)
   {
      ensure(trackCount > 0);
      ensure(trackIndex >= 0 && trackIndex <= trackCount - 1);
      return startingTrack == ETATLockpickStartingTrack::OuterRing ? trackCount - trackIndex - 1 : trackIndex;
   }
}

USTRUCT(BlueprintType)
struct TAT_API FTATLockpickTrackBuildParams
{
   GENERATED_BODY()

public:
   // The width size of each track
   UPROPERTY(EditDefaultsOnly, Category = "Track Data")
   float TrackWidth = 30.0f;

   // Defines the radius of the first track
   UPROPERTY(EditDefaultsOnly, Category = "Track Data")
   float TrackStartingRadius = 395.0f;

   // Defines where tracks start in degrees, where the right edge on the circle is zero 
   // and positive values rotate the starting position clockwise
   UPROPERTY(EditDefaultsOnly, Category = "Track Data", Meta = (ClampMin = "0.0", ClampMax = "360.0", UIMin = "0.0", UIMax = "360.0", Units = deg))
   float TrackStartingAngle = 0.0f;

   // How much space (if any) should exist between multiple tracks
   UPROPERTY(EditDefaultsOnly, Category = "Track Data")
   float PaddingBetweenTracks = 2.0f;

   // Defines the order that tracks should be laid out in rings - should the first ring be on the outside
   // and incrementing rings within it, or should it be the innermost and increment outwards?
   UPROPERTY(EditDefaultsOnly, Category = "Track Data")
   ETATLockpickStartingTrack StartingTrack = ETATLockpickStartingTrack::OuterRing;

   // Settings pertaining to visualization of track sections
   UPROPERTY(EditDefaultsOnly, Category = "Section Data", meta = (Categories = "Lockpick.TrackSection"))
   TMap<FGameplayTag, FOSERadialDiscShape> SectionTypeToShape;

   // Optional widget to spawn at the end of each track
   UPROPERTY(EditDefaultsOnly, Category = "Track End")
   TSubclassOf<UOSEUserWidget> TrackEndWidget = nullptr;

   // Size of 'TrackEndWidget' when spawned
   UPROPERTY(EditDefaultsOnly, Category = "Track End")
   FVector2D TrackEndWidgetSize = FVector2D(30.0f, 30.0f);

   UPROPERTY(Transient)
   UCanvasPanel* CanvasPanel = nullptr;
};

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATLockpickTrackWidget : public UTATUserWidget
{
	GENERATED_BODY()
	
   UTATLockpickTrackWidget();

protected:
   // from UUserWidget
   virtual int32 NativePaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const override;

public:
   // from UUserWidget
   virtual void RemoveFromParent() override;

   void BuildTrack(const FTATLockpickTrackLayerManager& trackLayerStack, const FTATLockpickTrackBuildParams& trackBuildParams, float trackSectionRadiusOffset, float maxTrackAngle);

   void UpdateTrackProgress(float trackProgress);

   FORCEINLINE float GetNormalizedTrackProgress() const
   {
      check(!FMath::IsNearlyZero(_trackDuration));
      return _trackProgress / _trackDuration;
   }

private:
   TArray<FOSERadialDiscShape> _trackShapes;

   // Optional widget to position at the end of this track
   UPROPERTY()
   UOSEUserWidget* _trackEndWidget = nullptr;

   // Size use for spawned _trackEndWidget
   FVector2D _trackEndWidgetSize = FVector2D();

   float _trackRadius = 0.0f;
   float _trackStartingAngle = 0.0f;
   float _trackMaxAngle = 0.0f;
   float _trackProgress = 0.0f;
   float _trackDuration = 0.0f;

   void _UpdateTrackEndWidget();
};

USTRUCT()
struct TAT_API FTATLockpickMultiTrackWidgetManager
{
   GENERATED_BODY()

public:
   void BuildTracks(const FTATLockpickMinigameVariation& lockpickMinigameVariation, TScriptInterface<ILockpickableInterface> lockpickableActor, const FTATLockpickTrackBuildParams& trackBuildParams);

   void AddInteractionToTrack(int32 trackIndex, float interactionTime, const FTATLockpickMinigameVariation& lockpickMinigameVariation, const FTATLockpickTrackBuildParams& trackBuildParams);

   void UpdateTrackProgress(int32 trackIndex, float trackProgress);

   void RemoveTrack(int32 trackIndex);

   void RemoveAllTracks();

   FORCEINLINE int32 GetTrackCount() const { return _trackWidgets.Num(); }

private:
   UPROPERTY()
   TArray<UTATLockpickTrackWidget*> _trackWidgets;

   TArray<FTATLockpickTrackLayerManager> _trackLayerStacks;
};
