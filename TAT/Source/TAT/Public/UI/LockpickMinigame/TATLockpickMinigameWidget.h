// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Lockpicking/LockpickableInterface.h"
#include "Lockpicking/TATLockpickingTypes.h"
#include "UI/TATUserWidget.h"
#include "TATLockpickTrackWidget.h"

// ue
#include "UObject/WeakInterfacePtr.h"

#include "TATLockpickMinigameWidget.generated.h"

class UCanvasPanel;

UCLASS(Blueprintable, BlueprintType, meta = (DisableNativeTick))
class TAT_API UTATLockpickMinigameWidget : public UTATUserWidget
{
   GENERATED_BODY()

   UTATLockpickMinigameWidget();

public:
   // from UUserWidget
   virtual int32 NativePaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const override;

   UFUNCTION(BlueprintCallable)
   void StartMinigame(const FTATLockpickMinigameVariation& lockpickMinigameVariation, TScriptInterface<ILockpickableInterface> lockpickableActor, UCanvasPanel* multiTrackCanvasPanel);

   UFUNCTION(BlueprintCallable)
   void StopMinigame();

   UFUNCTION(BlueprintCallable)
   void InitializeFocus();

protected:
   UFUNCTION(BlueprintCallable)
   void UpdateTrackProgress(int32 trackIndex, float trackProgress);

   UFUNCTION(BlueprintCallable)
   FVector2D GetTrackInteractLocation(int32 trackIndex) const;

   UFUNCTION(BlueprintCallable)
   void HandleInteractionTriggered(int32 trackIndex, float interactionTime);

   UFUNCTION(BlueprintCallable)
   void HandleTrackFinished(int32 trackIndex);

   TScriptInterface<ILockpickableInterface> GetLockpickableActor() const;

private:
   // Selected lockpick minigame variation
   UPROPERTY(Transient)
   FTATLockpickMinigameVariation _lockpickMinigameVariation;

   // Lockable actor being picked
   TWeakInterfacePtr<ILockpickableInterface> _lockpickableActor;

   // Manages all track widgets
   UPROPERTY(Transient)
   FTATLockpickMultiTrackWidgetManager _multiTrackWidgetManager;

   // Defines configurable values to customize track visualization
   UPROPERTY(EditAnywhere)
   FTATLockpickTrackBuildParams _trackBuildParams;

   UPROPERTY(EditAnywhere, Category = "Lockpick Track Designer Preview")
   bool _previewLockpickTrack = false;

   UPROPERTY(EditAnywhere, Category = "Lockpick Track Designer Preview", Meta = (EditCondition = "_previewLockpickTrack"))
   bool _previewTrackSectionNames = true;

   UPROPERTY(EditAnywhere, Category = "Lockpick Track Designer Preview", Meta = (EditCondition = "_previewLockpickTrack"))
   bool _previewSingleTrackSection = false;

   UPROPERTY(EditAnywhere, Category = "Lockpick Track Designer Preview", Meta = (EditCondition = "_previewLockpickTrack && _previewSingleTrackSection", Categories = "Lockpick.TrackSection"))
   FGameplayTag _previewSingleTrackSectionType;

};
