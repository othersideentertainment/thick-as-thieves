// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat
#include "WorldMap/TATWorldMapTypes.h"
#include "TATWorldMapBoundary.h"

// ue4
#include "Components/ActorComponent.h"
#include "Math/Vector2D.h"

#include "TATMapActorComponent.generated.h"

// Component used to track a world actor on the map screen by registering with UTATWorldMapSubsystem. 
// Defaults to representing its owner, but can represent a different actor via SetRepresentedActor().
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TAT_API UTATMapActorComponent : public UActorComponent
{
   GENERATED_BODY()

   UTATMapActorComponent();

public:

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   // From UActorComponent
   virtual void Activate(bool bReset=false) override;
   virtual void Deactivate() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   UFUNCTION(BlueprintPure, Category = "Map Actor")
   const FTATMapRepresentationData& GetMapRepresentationData() const { return _mapRepresentationData; }
   // Intended to be called before registration with the TATWorldMapSubsystem
   void SetMapRepresentationData(const FTATMapRepresentationData& mapRepresentationData);

   // Used to set _mapRepresentationData.IsGeneralArea without having to worry about whether the map actor is registered or active
   void SetGeneralAreaVisible(bool bGeneralAreaVisual, bool bSetActive = false);

   // Overrides the actor represented on the map screen. Useful if you want this component to represent a different actor than its owner
   void SetRepresentedActor(const AActor* mapRepresentedActor);

   TWeakObjectPtr<const AActor> GetRepresentedActor() const { return _mapRepresentedActor; }

   // Updates the sprite used to represent this actor on the map screen, with spriteTag corresponding to an entry in FTATMapRepresentationData::MapSpriteTable
   UFUNCTION(BlueprintCallable, Category = "Map Actor")
   void UpdateMapSprite(const FGameplayTag spriteTag);

   // Updates the text label displayed on the map
   UFUNCTION(BlueprintCallable, Category = "Map Actor")
   void UpdateMapLabel(const FText& newLabel);

   // Gets the actual world location of the represented actor
   UFUNCTION(BlueprintPure, Category = "Map Actor")
   FVector GetActorLocation() const;

   // Gets the location that should be shown on the map for the represented actor (takes into account offsets and delays in updates)
   UFUNCTION(BlueprintPure, Category = "Map Actor")
   FVector GetMapLocation() const;

   UFUNCTION(BlueprintPure, Category = "Map Actor")
   FVector2D GetActorFacingDirection() const;

   const FTATMapSpriteEntry* GetCurrentMapSpriteEntry() const;

   const FTATMapSpriteEntry* GetMapSpriteEntry(FGameplayTag spriteTag) const;

   const FText& GetCurrentMapLabel() const;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Map Representation")
   FText MapLabel;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Map Representation")
   bool UseMapTypeRequirement = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Map Representation", Meta = (EditCondition = "UseMapTypeRequirement"))
   ETATWorldMapBoundaryType RequiredMapType = ETATWorldMapBoundaryType::Secondary;

private:
   void _SetMapTrackingEnabled(bool enabled);

   void _UpdateMapLocationForGeneralArea();

private:
   UPROPERTY(EditAnywhere, Category = "World Map Representation")
   FTATMapRepresentationData _mapRepresentationData;

   UPROPERTY(Transient)
   TObjectPtr<UTATMapSpriteDataAsset> _mapSpriteDataAsset;

   // Optional offset for the represented location
   UPROPERTY(EditAnywhere, Category = "World Map Representation")
   FVector _mapLocationOffset;

   // Keeps track of the current sprite representation (tag corresponds to entry in FTATMapRepresentationData::MapSpriteTable)
   FGameplayTag _currentSpriteTag;

   FText _currentMapLabel;

   // Actor to be represented on the map. Defaults to the component owner, but can be manually assigned
   UPROPERTY(Transient)
   TWeakObjectPtr<const AActor> _mapRepresentedActor;

   // When using a general area, we base the radius off of this location
   // This location gets updated on a frequency defined in FTATMapRepresentationData
   FVector _savedMapLocationForGeneralArea;

   FTimerHandle _updateGeneralAreaTimerHandle;
};
