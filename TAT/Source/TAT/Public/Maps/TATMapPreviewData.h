// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "SaveGame/TATSaveGame.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATMapPreviewData.generated.h"

// A class for defining data of Map Preview objects
UCLASS(BlueprintType)
class TAT_API UTATMapPreviewInfo : public UObject
{
   GENERATED_BODY()
public:

   // Soft Object Pointer to the actual UMAP file
   UPROPERTY(EditAnywhere, BlueprintReadonly, Category = MapPreview)
   TSoftObjectPtr<UWorld> Map;
   
   // Name to be displayed when previewing the map
   UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = MapPreview)
   FText MapDisplayName;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = MapPreview)
   FString MapName;

   // Set of quests which can be played on this map
   UPROPERTY(EditAnywhere, BlueprintReadonly, Category = MapPreview, meta = (Categories = "Mission"))
   TArray<FGameplayTag> Missions;

   // Mesh representing the map
   UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = MapPreview)
   TSoftObjectPtr<UStaticMesh> MapPreviewMesh;

   // Scalar for the MapPreviewMesh
   UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = MapPreview)
   FVector MeshScaling = FVector(1.0f);

   // Coordinates that define this map's location on the City Map
   UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = MapPreview)
   FVector2D CityMapCoordinates = FVector2D::ZeroVector;

   // Represents if the map is available to the player
   UPROPERTY(BlueprintReadonly, Category = MapPreview)
   ETATFeatureAvailabilityToPlayer AvailabilityToPlayer = ETATFeatureAvailabilityToPlayer::Available;

   // Represents if the player has a Contract in this map
   UPROPERTY(BlueprintReadonly, Category = MapPreview)
   bool HasContract = false;
};
