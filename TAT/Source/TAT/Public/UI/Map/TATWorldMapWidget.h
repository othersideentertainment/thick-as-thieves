// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "UI/TATUserWidget.h"
#include "WorldMap/TATWorldMapTypes.h"

// ue
#include "UObject/WeakObjectPtrTemplates.h"


#include "TATWorldMapWidget.generated.h"

class UTATWorldMapActorWidget;
class UTATWorldMapSubsystem;
class UTATMapActorComponent;

// Widget located on the Management Screen that displays a map of the level, overlaid with map-represented actors.
// To display a new actor on the map, give it a UTATMapActorComponent so it can register with the UTATWorldMapSubsystem.
UCLASS(Blueprintable)
class TAT_API UTATWorldMapWidget : public UTATUserWidget
{
   GENERATED_BODY()

   // From UUserWidget
   virtual void NativeTick(const FGeometry& myGeometry, float inDeltaTime) override;
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;

public:
   // Toggles whether the map will keep map actors continuously updated
   UFUNCTION(BlueprintCallable, Category = "Map Screen")
   void SetMapAutoUpdate(bool newAutoUpdate);
   
protected:
   UFUNCTION(BlueprintImplementableEvent, Category = "Map Screen")
   void AddMapActorWidgetToScreen(UTATWorldMapActorWidget* mapActorWidget);

   UFUNCTION(BlueprintImplementableEvent, Category = "Map Screen")
   void RemoveMapActorWidgetFromScreen(UTATWorldMapActorWidget* mapActorWidget);

   UFUNCTION(BlueprintImplementableEvent, Category = "Map Screen")
   void UpdateMapActorWidgetLocation(UTATWorldMapActorWidget* mapActorWidget, FVector2D mapLocation, bool visibleOnMap);

   UFUNCTION(BlueprintImplementableEvent, Category = "Map Screen")
   FVector2D GetMapWidgetDimensions(FVector2D& Offset) const;

private:
   void _SetMapDimensions(const FVector2D& mapDimensions, const FVector2D& mapOffset);

   // Refreshes the appearance of a given widget from cached state
   void _RefreshMapActorWidgetAppearance(UTATWorldMapActorWidget* mapActorWidget, const UTATMapActorComponent* mapActor);

   // Constructs a widget to represent the given actor, adding it to the screen and initializing its appearance
   UTATWorldMapActorWidget* _ConstructMapActorWidget(const UTATMapActorComponent* mapActor);
   
   // Removes all map actor widgets from the screen
   void _ClearMapActorWidgets();

   void _BindToWorldMapSubsystem(UTATWorldMapSubsystem* worldMapSubsystem);
   void _UnbindFromWorldMapSubsystem(UTATWorldMapSubsystem* worldMapSubsystem);

   // Returns the map location that the given actor should be displayed at
   TOptional<FVector2D> _GetMapLocationForActor(const UTATMapActorComponent* mapActor) const;

   // Returns the widget representing the given map actor
   UTATWorldMapActorWidget* _GetMapActorWidget(const UTATMapActorComponent* mapActor) const;

   // Enables or disables this widget's tick function
   void _SetTickEnabled(bool enableWidgetTick);

   UFUNCTION()
   void _OnMapActorChanged(const UTATMapActorComponent* mapActor);
   UFUNCTION()
   void _OnMapActorRegistered(const UTATMapActorComponent* mapActor);
   UFUNCTION()
   void _OnMapActorUnregistered(const UTATMapActorComponent* mapActor);

   UPROPERTY(Transient)
   TMap<TWeakObjectPtr<const UTATMapActorComponent>, UTATWorldMapActorWidget*> _mapActorWidgetTable;

   // Class used for map actor widget construction
   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UTATWorldMapActorWidget> _mapActorWidgetClass;

   // Cached offset and dimensions of the map image used for converting actor world-location to map-location
   FVector2D _cachedMapOffset = FVector2D::ZeroVector;
   FVector2D _cachedMapDimensions = FVector2D::ZeroVector;

   // SetMapAutoUpdate always sets this to true so we know if we should let blueprints control tick or not
   bool _mapAutoUpdateCalled = false;
};
