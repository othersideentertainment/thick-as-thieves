// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "TATThievesDenManager.generated.h"

class UTATScreenWidget;

UCLASS(Blueprintable, BlueprintType)
class TAT_API ATATThievesDenManager : public AActor
{
   GENERATED_BODY()

public:
   ATATThievesDenManager();

   UFUNCTION(BlueprintPure, DisplayName = "Get TAT Thieves' Den Manager", Category = "Thieves Den", meta = (WorldContext = "worldContext", CompactNodeTitle = "Thieves' Den Manager"))
   static ATATThievesDenManager* Get(const UObject* worldContext);

   // from AActor
   virtual void BeginPlay() override;
   virtual void Tick(float deltaSeconds) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

public:
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thieves Den Manager")
   TSubclassOf<UTATScreenWidget> PostMatchScreenWidget;

   UPROPERTY(EditDefaultsOnly)
   float SecondsToTravel = 6.0f;

   UFUNCTION(BlueprintPure)
   float GetTravelWorldTime() const { return _authorityTravelWorldTime; }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTravelWorldTimeChanged, float, serverTravelWorldTime);
   UPROPERTY(BlueprintAssignable)
   FOnTravelWorldTimeChanged OnTravelWorldTimeChanged;

   UFUNCTION(BlueprintPure)
   TSoftObjectPtr<UWorld> GetSelectedMap() const { return _authoritySelectedMap; }

   void AuthoritySetSelectedMap(const TSoftObjectPtr<UWorld>& newMap);

protected:
   void _OnAuthorityTravelToMap(const TSoftObjectPtr<UWorld>& map);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAuthoritySelectedMapChanged);
   UPROPERTY(BlueprintAssignable)
   FOnAuthoritySelectedMapChanged OnSelectedMapChanged;

private:
   void _ShowPostMatchScreenIfNeeded();

   UFUNCTION()
   void _OnRep_AuthoritySelectedMap();

   UFUNCTION()
   void _OnRep_ServerTravelWorldTime();

   void _UpdateHUDMissionCountdown();

   void _AuthorityUpdateParty();

private:
   UPROPERTY(ReplicatedUsing = _OnRep_ServerTravelWorldTime)
   float _authorityTravelWorldTime = static_cast<float>(INDEX_NONE);

   UPROPERTY(Transient)
   UTATScreenWidget* _postMatchScreenWidget = nullptr;

   // authority state
   bool _authorityTravelStarted = false;

   UPROPERTY(EditAnywhere, ReplicatedUsing = _OnRep_AuthoritySelectedMap)
   TSoftObjectPtr<UWorld> _authoritySelectedMap;
};
