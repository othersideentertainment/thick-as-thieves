// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "GameFramework/TATWorldTypes.h"

// ose
#include "Player/OSEPlayerController.h"

// ue
#include "GameFramework/WorldSettings.h"

#include "TATWorldSettings.generated.h"

class ATATWeatherManager;
class ATATWeatherPreset;
class ATATWorldMapBoundary;
class UTATSpawnDataAsset;
class UTATQuestGraph;
struct FGameplayTag;

UCLASS(BlueprintType)
class TAT_API ATATWorldSettings : public AWorldSettings
{
   GENERATED_BODY()

public:
   // From AWorldSettings
   virtual void NotifyBeginPlay() override;
   virtual void BeginPlay() override;

   // static
   static ATATWorldSettings& Get(const UObject* contextObj);

   UFUNCTION(BlueprintPure, Category = "TAT World Settings", meta = (WorldContext = "contextObj"))
   static ATATWorldSettings* GetTATWorldSettings(const UObject* contextObj);

   // Preferred over UWorld::OnWorldBeginPlay, which doesn't account for the delay in replicated-begin-play for clients (see UWorld::BeginPlay() / ATATGameState::OnRep_ReplicatedHasBegunPlay())
   DECLARE_MULTICAST_DELEGATE(FTATOnWorldBeginPlay);
   FTATOnWorldBeginPlay OnWorldBeginPlay;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT World Settings")
   ETATMapType MapType = ETATMapType::Mission;

   UPROPERTY(EditDefaultsOnly, Category = "TAT World Settings")
   TObjectPtr<UTATSpawnDataAsset> SpawnData = nullptr;

   // Missions that this map will choose playing, if no missions are configured
   // Most useful for test or development maps
   // (But the mission still has to exist, which is less helpful in terms of isolated testing)
   UPROPERTY(EditDefaultsOnly, Category = "TAT World Settings", meta=(Categories="Mission"))
   TArray<FGameplayTag> DevelopmentMissions;

#if WITH_EDITORONLY_DATA
   // An optional scene-set that level instance or sublevels can validate against to catch references to other scenes (and eventually filter in the UI)
   UPROPERTY(EditDefaultsOnly, Category = "TAT World Settings|Validation")
   TObjectPtr<class UTATSceneSetAsset> ValidationSceneSet = nullptr;
#endif

   UPROPERTY(EditDefaultsOnly, Category = "TAT World Settings")
   EPlayerCameraMode MapStartCameraMode = EPlayerCameraMode::FirstPerson;

   // Drives the cardinal orientation of the compass.
   // North = positive X, East = Negative Y, etc.
   UPROPERTY(EditDefaultsOnly, Category = "TAT World Settings")
   FQuat WorldCardinalAxes;

   // The weather manager for this level.
   // NB. The weather system is only enabled for levels that have this assigned to a valid actor.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT World Settings")
   TObjectPtr<ATATWeatherManager> WeatherManager;

#if WITH_EDITORONLY_DATA
   // [Editor Only] The default weather preset to use when loading this level in the editor.
   // Has no effect on gameplay at all.
   /// Note that if you change this, you need to reload the level (or close and re-open the weather preset editor tool) before this takes effect.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT World Settings")
   TSoftClassPtr<ATATWeatherPreset> EditorDefaultWeatherPreset;
#endif

   // Actor used for defining the boundary of valid map-screen-represented space
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT World Settings|Map")
   ATATWorldMapBoundary* WorldMapBoundary = nullptr;
};
