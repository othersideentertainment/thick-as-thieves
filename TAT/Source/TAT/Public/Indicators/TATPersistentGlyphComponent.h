// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Indicators/TATGlyphComponent.h"

#include "TATPersistentGlyphComponent.generated.h"

class UMaterialInstanceDynamic;

///
/// Persistent glyph indicator that manages its visibility based on its distance from the player.
/// This is intended to be used in conjunction with UTATGlyphIndicatorSubsystem to handle visibility checks.
///
UCLASS(BlueprintType, Blueprintable, DontCollapseCategories, HideCategories=(Replication, ComponentReplication, Streaming, Mobile, LOD, HLOD, Cooking, ComponentTick), Meta = (BlueprintSpawnableComponent))
class TAT_API UTATPersistentGlyphComponent : public UTATGlyphComponent
{
   GENERATED_BODY()

public:
   UTATPersistentGlyphComponent(const FObjectInitializer& objectInitializer);

   //// From UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
   UFUNCTION(BlueprintCallable, Category = "TAT|Indicator")
   void RegisterIndicator();

   UFUNCTION(BlueprintCallable, Category = "TAT|Indicator")
   void UnregisterIndicator();

   // Updates the range within which the glyph is visible for the local player
   UFUNCTION(BlueprintCallable, Category = "TAT|Indicator")
   void SetVisibleRange(float visibleRange) { _visibleRange = visibleRange; }

   // Updates whether or not to skip player-in-range checks when evaluating glyph visibility for the local player
   UFUNCTION(BlueprintCallable, Category = "TAT|Indicator")
   void SetVisibleAtInfiniteRange(bool visibleAtInfiniteRange) { _visibleAtInfiniteRange = visibleAtInfiniteRange; }

   void RefreshVisibilityForLocalPlayer(const FVector& playerLocation);

   /// Returns the glyph's visible range, or zero if it is visible at infinite range
   UFUNCTION(BlueprintPure, Category = "TAT|Indicator")
   FORCEINLINE float GetVisibleRange() const { return !_visibleAtInfiniteRange ? _visibleRange : 0.0f; }

   /// Is this glyph indicator always visible regardless of distance to a player?
   UFUNCTION(BlueprintPure, Category = "TAT|Indicator")
   FORCEINLINE bool IsVisibleAtInfiniteRange() const { return _visibleAtInfiniteRange; }

   /// Set the glyph visibility directly, overriding automatic range checks
   virtual void SetGlyphVisibility(bool newVisible) override;

   /// If the glyph visibility was overridden with a call to SetGlyphVisibility, clear the override and let normal visibility checks kick in
   UFUNCTION(BlueprintCallable, Category = "TAT|Indicator")
   void SetAutomaticGlyphIndicatorVisibility();

public:
   // Will automatically register with the world subsystem if true. Set this to false if you want the owner to evaluate some condition before registering its indicator.
   UPROPERTY(EditAnywhere, Category = "TAT|Indicator")
   bool AutoRegisterWithSubsystem = true;

protected:

   // When true, glyph visibility is determined automatically based on distance to the player.
   bool _enableRangeBasedVisibility = true;

   // Distance within which the glyph should be visible (ignored when _visibleAtInfiniteRange = true)
   UPROPERTY(EditAnywhere, Category = "TAT|Indicator", meta = (UIMin = "0", ClampMin = "0", EditCondition = "!_visibleAtInfiniteRange"))
   float _visibleRange = 2000.f;

   // When enabled, the player <-> indicator distance checks are skipped and the glyph is visible at any distance
   UPROPERTY(EditAnywhere, Category = "TAT|Indicator")
   bool _visibleAtInfiniteRange = false;
   
   // Hysteresis distance incorporated into "is player in visibility range" checks to avoid visibility flickering at range boundaries. When player within visibility range, the range is increased by this distance.
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Indicator|Glyph", meta = (UIMin = "0", ClampMin = "0"))
   float _visibleRangeHysteresis = 100.f;
};
