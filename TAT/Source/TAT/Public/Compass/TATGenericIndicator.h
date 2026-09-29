// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// tat
#include "UI/TATCompassPOIWidget.h"

#include "TATGenericIndicator.generated.h"

// ue4
class UPaperSprite;

// tat
class ATATCharacter;
class ATATPlayerState;
class ATATPlayerController;
class UTATAreaIndicatorComponent;
class UTATCompassWidget;

DEFINE_LOG_CATEGORY_STATIC(LogTATGenericIndicator, Log, All);

// Controls which strategy is used to display an indicator within close range of the player
UENUM(BlueprintType)
enum class ETATIndicatorCloseRangeDisplayMode : uint8
{
   Hidden = 0,
   ShowOnCompass,
   ShowOnCompassFixed,
   ShowInWorldSpace
};

UENUM(BlueprintType)
enum class ETATIndicatorCloseRangeDetectionMode : uint8
{
   Distance = 0,
   AreaVolume
};

UENUM(BlueprintType)
enum class ETATIndicatorOutOfVerticalRangeBehavior : uint8
{
   // Display an arrow pointing towards their vertical position on the associated compass POI
   DisplayDirectionalArrow = 0,
   // Remove the associated POI from the compass
   RemoveFromCompass
};

// Data that controls instance-specific appearance/visuals
USTRUCT(Blueprintable, BlueprintType)
struct FTATIndicatorAppearanceData
{
   GENERATED_USTRUCT_BODY()

   // Icon to display in the UI
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings", meta = (DisplayThumbnail = "true"))
   UPaperSprite* IndicatorSprite = nullptr;

   // Color used to tint the POI / world space widget sprite
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings", meta = (DisplayName = "Indicator Sprite Color"))
   FLinearColor IndicatorColor = FLinearColor::White;

   // Color used for the world-space glow effect when the player is nearby
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings", meta = (EditCondition = "CloseRangeDisplayMode == ETATIndicatorCloseRangeDisplayMode::ShowInWorldSpace", EditConditionHides))
   FLinearColor IndicatorGlowColor = FLinearColor::White;

   // Shows the glow effect when the player enters close range if true. Only applies if CloseRangeDisplayMode is set to ShowInWorldSpace
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   bool ShowIndicatorGlowAtCloseRange = true;

   // Should we run the OutOfVerticalRangeBehavior when the indicator is far above/below the player?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   bool UseVerticalDisplacement = true;

   // Should we show left/right arrows for off-screen indicators?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   bool ShowOffScreen = true;

   // Should we show how far this indicator is from the player?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   bool ShowDistance = true;

   // Controls whether drop shadow renders behind the POI
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   bool ShowDropShadow = true;

   // If true, a generic sprite is displayed until the player enters a close range (specified by ConcealedIndicatorRange)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   bool ConcealSpriteUntilCloseRange = false;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   bool HideWhenOutOfCloseRange = false;

   // What kind of behavior to run if the indicator exceeds the VerticalDisplacementThreshold
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   ETATIndicatorOutOfVerticalRangeBehavior IndicatorOutOfVerticalRangeBehavior = ETATIndicatorOutOfVerticalRangeBehavior::DisplayDirectionalArrow;

   // How close should the player get before we show the indicator's true sprite?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings", meta = (EditCondition = "ConcealSpriteUntilCloseRange", EditConditionHides))
   float ConcealedIndicatorRange = 3000.f;

   // Shown when the player is outside ConcealedIndicatorRange, if ConcealSpriteUntilCloseRange = true
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings", meta = (EditCondition = "ConcealSpriteUntilCloseRange", EditConditionHides, DisplayThumbnail = "true"))
   UPaperSprite* ConcealedIndicatorSprite = nullptr;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   EPOIDisplayMode POIDisplayMode = EPOIDisplayMode::Image;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   EPOICategory POICategory = EPOICategory::Generic;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings")
   ETATIndicatorCloseRangeDisplayMode CloseRangeDisplayMode = ETATIndicatorCloseRangeDisplayMode::ShowInWorldSpace;

   // Text to display on the POI in place of an image. Only used if POIDisplayMode == Text
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Compass Settings", meta = (EditCondition = "POIDisplayMode == EPOIDisplayMode::Text", EditConditionHides))
   FText DisplayText;

   // Used to determine relative z-sort order for various types of POI icons on the compass
   int CompassZSortPriority = 0;
};

UCLASS()
class TAT_API ATATGenericIndicator : public AActor
{
   GENERATED_BODY()
   
public:   
   ATATGenericIndicator();

   // From AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
   virtual void Destroyed() override;

   // Should be used by compass in place of GetActorLocation(), so we can represent N/S/E/W with indicators that fake their location relative to the player.
   UFUNCTION(BlueprintCallable, BlueprintPure)
   virtual FVector GetIndicatorLocation() const { return GetActorLocation(); }

   // Returns relative z-order in which the compass icon for this indicator should render on-screen (does not account for sorting by distance from player).
   // Higher number -> higher priority -> renders on top of other icons with lower priority.
   UFUNCTION(BlueprintNativeEvent, BlueprintPure)
   int GetCompassIconPriorityOrder() const;
   virtual int GetCompassIconPriorityOrder_Implementation() const { return 0; }

   UFUNCTION(BlueprintCallable, BlueprintPure)
   ETATIndicatorOutOfVerticalRangeBehavior GetIndicatorOutOfVerticalRangeBehavior() const { return _indicatorAppearanceData.IndicatorOutOfVerticalRangeBehavior; };

   UFUNCTION(BlueprintCallable, BlueprintPure)
   const FTATIndicatorAppearanceData& GetIndicatorAppearanceData() const { return _indicatorAppearanceData; }

   UFUNCTION(BlueprintCallable, BlueprintPure)
   FLinearColor GetIndicatorColor() const { return _indicatorAppearanceData.IndicatorColor; }

   UFUNCTION(BlueprintCallable, BlueprintPure)
   UPaperSprite* GetIndicatorSprite() const { return _indicatorAppearanceData.IndicatorSprite; }

   UFUNCTION(BlueprintCallable, BlueprintPure)
   ETATIndicatorCloseRangeDisplayMode GetCloseRangeDisplayMode() const { return _indicatorAppearanceData.CloseRangeDisplayMode; }

   UFUNCTION(BlueprintCallable, BlueprintPure)
   ETATIndicatorCloseRangeDetectionMode GetCloseRangeDetectionMode() const { return _closeRangeDetectionMode; }

   UFUNCTION(BlueprintCallable, BlueprintPure)
   bool ShouldShowWorldSpaceGlow() const { return _indicatorAppearanceData.ShowIndicatorGlowAtCloseRange && _indicatorAppearanceData.CloseRangeDisplayMode == ETATIndicatorCloseRangeDisplayMode::ShowInWorldSpace; }

   UFUNCTION(BlueprintCallable, BlueprintPure)
   bool ShouldShowCompassAreaGlow() const;

   // Returns whether this indicator should show itself on the compass / in world-space
   UFUNCTION(BlueprintCallable, BlueprintPure)
   bool GetIndicatorEnabled() const { return _enabled; }

   // Enables/disables the indicator's CompassPOI and world-space widget
   UFUNCTION(BlueprintCallable)
   void SetIndicatorEnabled(bool enabled);

   // Returns true if the locally controlled player is in close range
   UFUNCTION(BlueprintCallable, BlueprintPure)
   bool GetPlayerInCloseRange() const { return _isPlayerInCloseRange; }
   void SetPlayerInCloseRange(bool isPlayerInCloseRange, bool isInitializing = false);

   UFUNCTION(BlueprintImplementableEvent)
   void OnPlayerEnterExitsCloseRange(bool isPlayerInCloseRange);

   // Controls the visibility of the world-space widget used by this indicator
   UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
   void SetVisibleInWorldSpace(bool enabled);

   UPROPERTY(EditAnywhere, Category = "Compass Settings")
   bool ForceIndicatorEnabled = false;

#if WITH_EDITOR
   // from AActor
   virtual void CheckForErrors() override;
#endif // WITH_EDITOR

protected:
   // Registers this indicator with the TATCompass instance
   void _RegisterIndicator();
   // Un-registers this indicator with the TATCompass instance
   void _UnregisterIndicator();

   UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, BlueprintPure)
   USceneComponent* _GetWorldSpaceWidgetComponent();

   // Sets up the visuals used to display this indicator in world-space (glow, world-space widget, etc)
   UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
   void _InitializeWorldSpaceRepresentation();

   UPROPERTY(EditAnywhere, Category = "Compass Settings")
   FTATIndicatorAppearanceData _indicatorAppearanceData;

   UPROPERTY(EditAnywhere, Category = "Compass Settings")
   ETATIndicatorCloseRangeDetectionMode _closeRangeDetectionMode = ETATIndicatorCloseRangeDetectionMode::Distance;

   // Detects when the player has entered/exited the area volume, updating the compass displayed state accordingly. Only used if _closeRangeDetectionMode is set to AreaVolume.
   UPROPERTY(VisibleAnywhere, Category = "Compass Settings")
   UTATAreaIndicatorComponent* _areaIndicatorComponent;

private:
   // Controls whether this indicator should show itself on the compass
   bool _enabled = false;

   // Stores state for when the player is in/out of close range. 
   bool _isPlayerInCloseRange = false;

   FDelegateHandle _localPlayerLoadedIntoMapHandle;
};
