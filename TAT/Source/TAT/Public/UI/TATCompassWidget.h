// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

// tat
#include "TATUserWidget.h"

#include "TATCompassWidget.generated.h"

// tat
class ATATCardinalDirectionIndicator;
class ATATGenericIndicator;
class UTATCompassPOIWidget;
enum class EPOICategory : uint8;
class USizeBox;

// Bucket of POIs with the same z-sort priority, mapped to their distance from the player (recalculated each tick in _UpdatePOIs())
USTRUCT()
struct FTATCompassPOIZSortPriorityBucket
{
   GENERATED_BODY()

   FTATCompassPOIZSortPriorityBucket() = default;

   FTATCompassPOIZSortPriorityBucket(UTATCompassPOIWidget* compassPOI, float distance)
   {
      POIToDistanceMap.Add(compassPOI, distance);
   }

   // Maps each POI to its distance from the player
   UPROPERTY()
   TMap<TObjectPtr<UTATCompassPOIWidget>, float> POIToDistanceMap;
};

UCLASS()
class TAT_API UTATCompassWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;
   virtual void NativeTick(const FGeometry& myGeometry, float inDeltaTime) override;

   // Enables/disables compass POI associated with the indicator. Returns false if no associated POI is found.
   bool SetPOIEnabledForIndicator(const ATATGenericIndicator* genericIndicator, bool isEnabled);

   void AddCategoryToOmittedPOIs(EPOICategory category) { _poiCategoriesToOmit.AddUnique(category); }

   // Called when the owning player enters/exits close range of an indicator
   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
   void HandlePlayerInCloseRange(ATATGenericIndicator* genericIndicator, bool isInCloseRange);

   UFUNCTION(BlueprintCallable, BlueprintPure)
   int GetPOICount() const { return _indicatorToPOIMap.Num(); }

   UFUNCTION(BlueprintCallable, BlueprintPure)
   int GetActivePOICount() const;

   // Attempts to retrieve the compass from TATHUD. Will return nullptr until TATHUD::BeginPlay() initializes the compass instance.
   static UTATCompassWidget* TryGetCompass(const UObject* contextObj);

protected:
   UFUNCTION(BlueprintImplementableEvent)
   UTATCompassPOIWidget* _ConstructIndicatorPOI(const ATATGenericIndicator* genericIndicator);

   UFUNCTION(BlueprintImplementableEvent)
   void _DestructIndicatorPOI(UTATCompassPOIWidget* poiWidget);

   UFUNCTION(BlueprintCallable, BlueprintPure)
   UTATCompassPOIWidget* _GetPOIWidgetForIndicator(const ATATGenericIndicator* genericIndicator) const;

   UFUNCTION(BlueprintCallable, BlueprintPure)
   float _CalculatePOICompassPosition(const ATATGenericIndicator* genericIndicator, bool& outIsOnScreen) const;

   // Returns the compass widget's screen width in local-space (unscaled) or absolute space (DPI-scaled to viewport dimensions)
   UFUNCTION(BlueprintPure)
   float _GetCompassScreenWidth(bool absolute = false) const;

   // Returns a value between 0 and 1 indicating how much of the screen width the compass occupies
   UFUNCTION(BlueprintCallable, BlueprintPure)
   float _GetCompassScreenWidthNormalized() const;

   // Called each frame the indicator is off-screen
   UFUNCTION(BlueprintCallable)
   void _HandleIndicatorOffScreen(UTATCompassPOIWidget* POI, const ATATGenericIndicator* genericIndicator, bool isLeftOfScreen);
   void _HandleIndicatorAboveBelowPlayer(UTATCompassPOIWidget* POI, const ATATGenericIndicator* genericIndicator, float verticalDisplacement);

   // Called each frame to scoll the compass cardinal axis based on the player's orientation relative to _worldCardinalAxes' forward vector
   void _HandleUpdateCompassBackground(const float playerCardinalSignedAngle);

   UFUNCTION(BlueprintCallable)
   void _SetCompassPOIPosition(UTATCompassPOIWidget* POI, const float position);

   void _UpdateIndicatorCloseToPlayer(ATATGenericIndicator* genericIndicator, bool isInitializing = false);

protected:
   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<ATATCardinalDirectionIndicator> _cardinalIndicatorClass;

   UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
   TObjectPtr<USizeBox> CompassSizeBox = nullptr;

   UPROPERTY(Transient, BlueprintReadWrite)
   TObjectPtr<UMaterialInstanceDynamic> CompassBGMaterialInstance = nullptr;

   UPROPERTY(EditDefaultsOnly, Category=Material)
   FName UVScrollPropertyName;

   UPROPERTY(EditDefaultsOnly, Category=Material)
   FName FOVPropertyName;

private:

   // Adds the indicator to the compass if not already present, creating a corresponding POI and updating mappings
   void _RegisterIndicator(ATATGenericIndicator* genericIndicator);

   // Removes the indicator from the compass, updating mappings and destroying the associated POI
   void _DeregisterIndicator(ATATGenericIndicator* genericIndicator);
   
   // Updates POI positions, calling BP-implemented events for handling various visual effects
   void _UpdatePOIs();

   // Updates the POI's position on the compass, along with other visual state
   void _RefreshPOIAppearance(UTATCompassPOIWidget* compassPOI, const ATATGenericIndicator* genericIndicator, const FVector playerToIndicator, const float distanceFromPlayer);

   // Calculates player's orientation relative to _worldCardinalAxes and passes data to blueprint for the scrolling material
   void _UpdateScrollingCompassBG();

   // Updates each POI's z-order according to their priority and distance from the player
   void _SortPOIZOrder();

   void _AddPOIToZSortPriorityMap(UTATCompassPOIWidget* poi, const int zSortPriority);

   // Returns signed angle from camera-forward vector (i.e. center screen) to player-to-indicator vector
   float _GetCameraForwardToIndicatorSignedAngle(const ATATGenericIndicator* genericIndicator, const APlayerController* controller) const;

   // Returns signed angle from camera-forward to the "north" direction according to _worldCardinalAxes
   float _GetCameraForwardToWorldNorthSignedAngle(const APlayerController* controller) const;

   FQuat _GetCameraOrientation(const APlayerController* controller) const;

   FVector _GetPlayerToIndicatorVector(const ATATGenericIndicator* genericIndicator, const APlayerController* playerController) const;

   bool _IsPlayerWithinRange(const ATATGenericIndicator* genericIndicator, const APlayerController* playerController, const float range) const;

   // Creates a cardinal direction indicator with specified settings
   ATATCardinalDirectionIndicator* _SpawnCardinalDirectionIndicator(const FVector relativeOffset, const FText text) const;

   // 1-to-1 mapping of indicators to compass POIs
   UPROPERTY(Transient)
   TMap<TObjectPtr<ATATGenericIndicator>, TObjectPtr<UTATCompassPOIWidget>> _indicatorToPOIMap;

   // Used to determine Z-sorting priority for POI icons
   UPROPERTY(Transient)
   TMap<int32, FTATCompassPOIZSortPriorityBucket> _poiZSortPriorityMap;

   // Drives the cardinal orientation of the compass.
   // North = positive X, East = Negative Y, etc.
   FQuat _worldCardinalAxes;

   // At certain phases of the game, we will want to stop showing POIs of a certain category.
   // When we want to do so, we add those categories here.
   TArray<EPOICategory> _poiCategoriesToOmit;
};
