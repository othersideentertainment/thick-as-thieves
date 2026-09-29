// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Maps/TATThievesDenMapSubsystem.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "SaveGame/TATSaveGame.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThievesDenMapSubsystem)


bool UTATThievesDenMapSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   UWorld* world = CastChecked<UWorld>(outer);

   const ATATWorldSettings* worldSettings = CastChecked<ATATWorldSettings>(world->GetWorldSettings());
   return worldSettings->MapType == ETATMapType::ThievesDen;
}

void UTATThievesDenMapSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);

   // Note: the save game should be available in the thieves den, but that may not be true
   //       if something goes directly into that level (outside of PIE, where that always works)
   //       Not handling that for now, but could bind to ose save game initialization flow if needed.

   _saveGame = UTATSaveGame::GetTATSaveGame(this);
   if (!ensure(_saveGame))
   {
      return;
   }

   _saveGame->OnUnlocksChanged.AddUniqueDynamic(this, &UTATThievesDenMapSubsystem::_RefreshMapPreviews);
   _InitMapPreviews();
   _RefreshMapPreviews();
}

void UTATThievesDenMapSubsystem::Deinitialize()
{
   if (IsValid(_saveGame))
   {
      _saveGame->OnUnlocksChanged.RemoveAll(this);
   }
}

void UTATThievesDenMapSubsystem::_InitMapPreviews()
{
   check(_mapPreviews.Num() == 0);
   for(const FTATMapSettings& mapSettings : UTATProjectSettings::Get().Maps)
   {
      UTATMapPreviewInfo* mapPreview = NewObject<UTATMapPreviewInfo>(this, NAME_None,RF_Transient);
      mapPreview->Map = mapSettings.Map;
      mapPreview->MapDisplayName = mapSettings.MapDisplayName;
      mapPreview->MapName = mapSettings.MapName;
      mapPreview->Missions = mapSettings.Missions;
      mapPreview->MapPreviewMesh = mapSettings.MapPreviewMesh;
      mapPreview->MeshScaling = mapSettings.MeshScaling;
      mapPreview->CityMapCoordinates = mapSettings.CoordinatesOnCityMap;
      _mapPreviews.Add(mapPreview);
   }
}

void UTATThievesDenMapSubsystem::_RefreshMapPreviews()
{
   const TArray<FTATMapSettings>& mapSettings = UTATProjectSettings::Get().Maps;
   const int mapCount = FMath::Min(mapSettings.Num(), _mapPreviews.Num());
   bool dirty = false;
   for(int mapIndex = 0; mapIndex < mapCount; mapIndex++)
   {
      UTATMapPreviewInfo* mapPreview = _mapPreviews[mapIndex];
      const bool isUnlocked = _saveGame->HasUnlockedContent(mapSettings[mapIndex].MapTag);
      const ETATFeatureAvailabilityToPlayer newAvailability = isUnlocked ? ETATFeatureAvailabilityToPlayer::Available : ETATFeatureAvailabilityToPlayer::Locked;
      if(newAvailability != mapPreview->AvailabilityToPlayer)
      {
         mapPreview->AvailabilityToPlayer = newAvailability;
         dirty = true;
      }
   }

   if(dirty)
   {
      OnMapPreviewsChanged.Broadcast();
   }
}
