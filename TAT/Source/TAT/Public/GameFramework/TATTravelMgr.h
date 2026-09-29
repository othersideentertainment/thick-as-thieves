// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat

// ose
#include "Player/OSEPlayerController.h"

// ue4
#include "CoreMinimal.h"
#include "Tickable.h"

#include "TATTravelMgr.generated.h"

UENUM(BlueprintType)
enum class ETATTravelType : uint8
{
   WithLoadingScreen,
   WithTransitionMap,
};

class ATATCharacter;
class ATATPlayerState;
class ATATPlayerController;
class UTATSeamlessTravelLoadingScreenWidget;
class UTATGameInstance;

UCLASS()
class TAT_API UTATTravelMgr : public UObject, public FTickableGameObject
{
   GENERATED_BODY()

public:
   void Init(UTATGameInstance* instanceOwner);
   void OnWorldChanged(UWorld* oldWorld, UWorld* newWorld);
   void Shutdown();

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "TAT Travel", meta = (WorldContext = "contextObj"))
   static void ServerInitiateTravel(const UObject* contextObj, const FString& mapName, ETATTravelType travelType);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "TAT Travel", meta = (WorldContext = "contextObj"))
   static void ServerInitiateTravelToWorld(const UObject* contextObj, const TSoftObjectPtr<UWorld>& map, ETATTravelType travelType);

   void ShowLoadingScreen();
   void HideLoadingScreen();

public:
   DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLocalPlayerLoadedIntoMap, class  ATATPlayerState*, class  ATATPlayerController*, class ATATCharacter*)
   FOnLocalPlayerLoadedIntoMap OnLocalPlayerLoadedIntoMap;

protected:
   // from UObject
   virtual UWorld* GetWorld() const;

   // from FTickableGameObject
   virtual bool IsTickable() const { return !HasAnyFlags(RF_ClassDefaultObject) && _gameInstance; }
   virtual TStatId GetStatId() const override { return Super::GetStatID(); }
   virtual void Tick(float deltaTime) override;

private:
   void _ServerInitiateTravel(const FString& mapName, ETATTravelType travelType);
   void _SetupMoviePlayerForTravel();
   void _OnPostLoadMapWithWorld(UWorld* loadedWorld);

private:
   UTATGameInstance* _gameInstance;

   // which map are we loading?
   FString _serverDestinationMapName;

   // our loading screen widget
   UPROPERTY()
   UUserWidget* _loadingScreenUserWidget;
   TSharedPtr<SWidget> _loadingScreenSlateWidget;

   // are we waiting to remove the loading screen on the destination map?
   bool _loadingScreenShowingOnDestinationMap = false;
};
