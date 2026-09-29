// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Map/TATWorldMapWidget.h"

// tat
#include "UI/Map/TATWorldMapActorWidget.h"
#include "WorldMap/TATMapActorComponent.h"
#include "WorldMap/TATWorldMapSubsystem.h"
#include "Developer/TATProjectSettings.h"

// ue
#include "Slate/SObjectWidget.h"
#include "Templates/SharedPointer.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWorldMapWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATMapScreenWidget, Log, All);

namespace WorldMapWidgetHelpers
{
   FString GetMapActorName(const UTATMapActorComponent* comp)
   {
      if (comp == nullptr)
      {
         return TEXT("NULL");
      }
      const AActor* owner = comp->GetOwner();
      return FString::Printf(TEXT("%s(%s)"), ((owner != nullptr) ? *owner->GetName() : TEXT("NULL")), *comp->GetName());
   }
}


void UTATWorldMapWidget::NativeTick(const FGeometry& myGeometry, float inDeltaTime)
{
   Super::NativeTick(myGeometry, inDeltaTime);

   // Drew Graham 6/22/23: UWidget::GetDesiredSize() returns a zero-vector when called in NativeConstruct() / NativeOnInitialized() (and their bp counterparts), 
   // preventing us from getting the map dimensions at widget construction. This is a temporary fix to allow us to compute map locations, as a more dedicated fix 
   // will have to change when we rework the screen to async-load the map image.
   FVector2D mapOffset = FVector2D::Zero();
   const FVector2D mapDimensions = GetMapWidgetDimensions(mapOffset);
   if (mapDimensions != FVector2D::Zero())
   {
      _SetMapDimensions(mapDimensions, mapOffset);

      // Disable tick now that we have the map dimensions, unless SetMapAutoUpdate() was called, in which case we'll let blueprints manage it
      if (!_mapAutoUpdateCalled)
      {
         _SetTickEnabled(false);
      }
   }
}

void UTATWorldMapWidget::NativeConstruct()
{
   Super::NativeConstruct();

   // Destroy any previously-existing map actor widgets
   _ClearMapActorWidgets();

   // Bind to world map subsystem
   UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>();
   check(worldMapSubsystem);
   _BindToWorldMapSubsystem(worldMapSubsystem);

   // Pull all map actors from subsystem
   const TSet<TWeakObjectPtr<UTATMapActorComponent>>& mapActors = worldMapSubsystem->GetMapRepresentedActors();

   // Construct widget representation for each
   _mapActorWidgetTable.Reserve(mapActors.Num());
   for (const TWeakObjectPtr<UTATMapActorComponent> weakMapActor : mapActors)
   {
      UTATMapActorComponent* mapActor = weakMapActor.Get();
      if (mapActor == nullptr)
      {
         continue;
      }
      UTATWorldMapActorWidget* mapActorWidget = _ConstructMapActorWidget(mapActor);
      
      check(IsValid(mapActorWidget));
      _mapActorWidgetTable.Emplace(weakMapActor, mapActorWidget);
   }

   UE_CLOG(mapActors.Num() != _mapActorWidgetTable.Num(), LogTATMapScreenWidget, Warning, TEXT("UTATWorldMapScreenWidget::NativeConstruct() expected %d map actor widgets, but detected %d instead!")
      , mapActors.Num()
      , _mapActorWidgetTable.Num());
}

void UTATWorldMapWidget::NativeDestruct()
{
   // Unbind from world map subsystem (if present)
   UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>();
   if (IsValid(worldMapSubsystem))
   {
      _UnbindFromWorldMapSubsystem(worldMapSubsystem);
   }

   _ClearMapActorWidgets();

   Super::NativeDestruct();
}

void UTATWorldMapWidget::SetMapAutoUpdate(bool newAutoUpdate)
{
   _SetTickEnabled(newAutoUpdate);
   _mapAutoUpdateCalled = true;
}

void UTATWorldMapWidget::_SetMapDimensions(const FVector2D& mapDimensions, const FVector2D& mapOffset)
{
   _cachedMapOffset = mapOffset;
   _cachedMapDimensions = mapDimensions;
      
   // Re-render all map actors on screen when map dimensions change
   for (const TPair<TWeakObjectPtr<const UTATMapActorComponent>, UTATWorldMapActorWidget*>& mapActorWidgetPair : _mapActorWidgetTable)
   {
      TWeakObjectPtr<const UTATMapActorComponent> mapActorPtr = mapActorWidgetPair.Key;
      UTATWorldMapActorWidget* mapActorWidget = mapActorWidgetPair.Value;

      check(IsValid(mapActorWidget));
      if (const UTATMapActorComponent* mapActor = mapActorPtr.Get())
      {
         _RefreshMapActorWidgetAppearance(mapActorWidget, mapActor);
      }
      else
      {
         UE_LOG(LogTATMapScreenWidget, Warning, TEXT("Null UTATMapActorComponent reference found in _mapActorWidgetTable!"));
      }
   }
}

void UTATWorldMapWidget::_RefreshMapActorWidgetAppearance(UTATWorldMapActorWidget* mapActorWidget, const UTATMapActorComponent* mapActor)
{
   check(IsValid(mapActorWidget));
   check(IsValid(mapActor));

   UE_LOG(LogTATMapScreenWidget, Verbose, TEXT("Refreshing widget appearance for map actor %s..."), *WorldMapWidgetHelpers::GetMapActorName(mapActor));

   const FTATMapSpriteEntry* mapSpriteEntry = mapActor->GetCurrentMapSpriteEntry();
   const FText& mapLabel = mapActor->GetCurrentMapLabel();

   if (mapSpriteEntry == nullptr && mapLabel.IsEmptyOrWhitespace())
   {
      UE_LOG(LogTATMapScreenWidget, Error, TEXT("World map widget for map actor %s does not have a sprite or a label"), *WorldMapWidgetHelpers::GetMapActorName(mapActor));
   }

   // Update sprite
   static const FTATMapSpriteEntry emptyMapSprite{};
   mapActorWidget->SetMapActorSprite((mapSpriteEntry != nullptr) ? (*mapSpriteEntry) : emptyMapSprite);

   // Update label
   mapActorWidget->SetMapLabel(!mapLabel.IsEmptyOrWhitespace() ? mapLabel : FText::GetEmpty());

   // Update map location
   const TOptional<FVector2D> mapLocation = _GetMapLocationForActor(mapActor);
   const bool mapActorVisible = mapLocation.IsSet();
   UpdateMapActorWidgetLocation(mapActorWidget, mapLocation.Get(FVector2D::Zero()), mapActorVisible);

   // Update facing direction
   const FTATMapRepresentationData& mapRepresentationData = mapActor->GetMapRepresentationData();
   if (mapRepresentationData.ShowFacingDirection)
   {
      mapActorWidget->SetMapActorFacingAngle(mapActor->GetActorFacingDirection());
   }

   // Determine if this should currently be visible on the map in regards to closeness to player
   bool showOnMap = true;
   if (mapRepresentationData.OnlyShowWhenCloseToPlayer)
   {
      // Get the local player
      if (const APlayerController* localController = GetWorld()->GetFirstPlayerController())
      {
         if (APawn* localPawn = localController->GetPawn())
         {
            if (localPawn != mapActor->GetOwner())
            {
               if (const UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
               {
                  FTATMapRepresentationData localPlayerRepData = mapRepresentationData;
                  localPlayerRepData.ShowGeneralAreaOnQuestLootObtained = false;
               
                  const UTATProjectSettings& tatSettings = UTATProjectSettings::Get();
                  const FVector localPawnLocation = localPawn->GetActorLocation();
                  const FVector mapActorLocation = mapActor->GetActorLocation();
                  const float mapDistance = FVector::Distance(localPawnLocation, mapActorLocation);
                  const float verticalDisplacement = FMath::Abs(localPawnLocation.Z - mapActorLocation.Z);
               
                  if (mapDistance > tatSettings.MaxDistanceToVisibleMapAndCompassActors
                     || verticalDisplacement > tatSettings.VerticalDisplacementThreshold)
                  {
                     showOnMap = false;
                  }
               }
            }
         }
         else
         {
            // If we land here, then there is no local player, likely because they are knocked out or dead.
            // In which case, they can't hold a map anyways, so whatever we do here is irrelevant...
            // But let's just make it not showable on map just in case.
            showOnMap = false;
         }
      }
   }

   // TODO: We should just use the showOnMap value to set this entire widget's visibility here in code instead of passing it to a BP event and letting it be handled in an external layer.
   // Refresh visuals for the map actor in BP
   mapActorWidget->RefreshMapActorVisuals(mapRepresentationData, showOnMap);
}

UTATWorldMapActorWidget* UTATWorldMapWidget::_ConstructMapActorWidget(const UTATMapActorComponent* mapActor)
{
   check(IsValid(mapActor));
   UE_LOG(LogTATMapScreenWidget, Verbose, TEXT("Constructing map actor widget for %s..."), *WorldMapWidgetHelpers::GetMapActorName(mapActor));

   // Create widget and add to screen
   UTATWorldMapActorWidget* mapActorWidget = CreateWidget<UTATWorldMapActorWidget>(this, _mapActorWidgetClass.Get());
   AddMapActorWidgetToScreen(mapActorWidget);

   // Initialize appearance
   _RefreshMapActorWidgetAppearance(mapActorWidget, mapActor);
   return mapActorWidget;
}

void UTATWorldMapWidget::_ClearMapActorWidgets()
{
   for (const TPair<TWeakObjectPtr<const UTATMapActorComponent>, UTATWorldMapActorWidget*>& mapActorWidgetPair : _mapActorWidgetTable)
   {
      UTATWorldMapActorWidget* mapActorWidget = mapActorWidgetPair.Value;
      RemoveMapActorWidgetFromScreen(mapActorWidget);
   }
   _mapActorWidgetTable.Reset();
}

void UTATWorldMapWidget::_BindToWorldMapSubsystem(UTATWorldMapSubsystem* worldMapSubsystem)
{
   check(IsValid(worldMapSubsystem));
   worldMapSubsystem->OnMapActorChanged.AddDynamic(this, &UTATWorldMapWidget::_OnMapActorChanged);
   worldMapSubsystem->OnMapActorRegistered.AddDynamic(this, &UTATWorldMapWidget::_OnMapActorRegistered);
   worldMapSubsystem->OnMapActorUnregistered.AddDynamic(this, &UTATWorldMapWidget::_OnMapActorUnregistered);
}

void UTATWorldMapWidget::_UnbindFromWorldMapSubsystem(UTATWorldMapSubsystem* worldMapSubsystem)
{
   check(IsValid(worldMapSubsystem));
   worldMapSubsystem->OnMapActorChanged.RemoveAll(this);
   worldMapSubsystem->OnMapActorRegistered.RemoveAll(this);
   worldMapSubsystem->OnMapActorUnregistered.RemoveAll(this);
}

TOptional<FVector2D> UTATWorldMapWidget::_GetMapLocationForActor(const UTATMapActorComponent* mapActor) const
{
   check(IsValid(mapActor));

   const UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>();
   check(worldMapSubsystem);

   auto getActorBoundingBox = [](const AActor* owner) -> FBox
   {
      check(owner != nullptr);
      FVector ownerOrigin;
      FVector ownerExtent;
      constexpr bool onlyCollidingComponents = true;
      owner->GetActorBounds(onlyCollidingComponents, ownerOrigin, ownerExtent);
      if (ownerExtent.X < 10 && ownerExtent.Y < 10 && ownerExtent.Z < 10)
      {
         // For actors without any colliding components, make a small fake bounding box around the actor location
         ownerOrigin = owner->GetActorLocation();
         ownerExtent = { 10, 10, 10 };
      }
      return FBox{ ownerOrigin - ownerExtent, ownerOrigin + ownerExtent};
   };

   if (ATATWorldMapBoundary* currentMapBoundary = worldMapSubsystem->GetCurrentWorldMapBoundaryActor())
   {
      // Check the map actor's map type requirement
      if (mapActor->UseMapTypeRequirement && currentMapBoundary->MapType != mapActor->RequiredMapType)
      {
         return NullOpt;
      }

      // If this is a secondary map, make sure the actor's bounding box intersects with the map's bounding box
      AActor* owner = mapActor->GetOwner();
      if (owner != nullptr
         && currentMapBoundary->MapType == ETATWorldMapBoundaryType::Secondary
         && !getActorBoundingBox(owner).Intersect(currentMapBoundary->GetMapAreaBoundingBox()))
      {
         return NullOpt;
      }
   }

   return worldMapSubsystem->ConvertWorldLocationToMapLocation(this, mapActor->GetMapLocation(), mapActor->GetMapRepresentationData(), _cachedMapDimensions, _cachedMapOffset);
}

UTATWorldMapActorWidget* UTATWorldMapWidget::_GetMapActorWidget(const UTATMapActorComponent* mapActor) const
{
   check(IsValid(mapActor));

   if (UTATWorldMapActorWidget* const* mapActorWidget = _mapActorWidgetTable.Find(mapActor))
   {
      return *mapActorWidget;
   }

   UE_LOG(LogTATMapScreenWidget, Warning, TEXT("Could not find map actor widget associated with map actor %s!"), *WorldMapWidgetHelpers::GetMapActorName(mapActor));
   return nullptr;
}

void UTATWorldMapWidget::_SetTickEnabled(bool enableWidgetTick)
{
   // Hacky way of toggling tick (UUserWidget does not expose a SetTickEnabled() function or any other way of achieving this, 
   // and we'll be reworking this to not necessitate a Tick at some point)
   if (TSharedPtr<SObjectWidget> SafeGCWidget = MyGCWidget.Pin())
   {
      SafeGCWidget->SetCanTick(enableWidgetTick);
   }
   else
   {
      UE_LOG(LogTATMapScreenWidget, Error, TEXT("Failed to %s map widget tick - widget is invalid"), (enableWidgetTick ? TEXT("enable") : TEXT("disable")));
   }
}

void UTATWorldMapWidget::_OnMapActorChanged(const UTATMapActorComponent* mapActor)
{
   check(IsValid(mapActor));

   // Refresh appearance
   UTATWorldMapActorWidget* mapActorWidget = _GetMapActorWidget(mapActor);
   check(IsValid(mapActorWidget));
   _RefreshMapActorWidgetAppearance(mapActorWidget, mapActor);
}

void UTATWorldMapWidget::_OnMapActorRegistered(const UTATMapActorComponent* mapActor)
{
   check(IsValid(mapActor));

   // Construct new map actor widget
   UTATWorldMapActorWidget* mapActorWidget = _ConstructMapActorWidget(mapActor);
   check(mapActorWidget);

   // Add to collection
   _mapActorWidgetTable.Emplace(mapActor, mapActorWidget);
}

void UTATWorldMapWidget::_OnMapActorUnregistered(const UTATMapActorComponent* mapActor)
{
   check(IsValid(mapActor));

   // Remove from map screen
   UTATWorldMapActorWidget* mapActorWidget = _GetMapActorWidget(mapActor);
   check(mapActorWidget);
   RemoveMapActorWidgetFromScreen(mapActorWidget);

   // Remove from set
   _mapActorWidgetTable.Remove(mapActor);
}
