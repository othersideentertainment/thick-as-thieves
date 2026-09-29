// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Online/TATGameState.h"

#include "TATThievesDenGameState.generated.h"

class ATATThievesDenManager;

UENUM(BlueprintType)
enum class ETATThievesDenScreen : uint8
{
   None,
   SetupMatch,
   JoinMatch,
   MAX UMETA(Hidden)
};

UCLASS()
class TAT_API ATATThievesDenGameState : public ATATGameState
{
   GENERATED_BODY()

public:
   ATATThievesDenGameState(const FObjectInitializer& objectInitializer);

   UFUNCTION(BlueprintPure, DisplayName = "Get TAT Thieves' Den Game State", Category = "Thieves Den", meta = (WorldContext = "worldContext"))
   static ATATThievesDenGameState* Get(const UObject* worldContext);

   // from AActor
   virtual void PostInitProperties() override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthoritySetThievesDenScreen(ETATThievesDenScreen newScreen);

   ETATThievesDenScreen GetThievesDenScreen() const { return _thievesDenScreen; }

   UFUNCTION(BlueprintPure)
   FORCEINLINE ATATThievesDenManager* GetThievesDenManager() const { return _thievesDenManager; }

private:
   UFUNCTION()
   void _OnRep_ThievesDenScreen(ETATThievesDenScreen oldState);

public:
   DECLARE_MULTICAST_DELEGATE_TwoParams(FOnThievesDenScreenChanged, ETATThievesDenScreen, ETATThievesDenScreen);
   FOnThievesDenScreenChanged OnThievesDenScreenChanged;

private:
   /// State indicating which screen should be visible
   UPROPERTY(Transient, ReplicatedUsing=_OnRep_ThievesDenScreen)
   ETATThievesDenScreen _thievesDenScreen = ETATThievesDenScreen::None;

   UPROPERTY(Transient)
   ATATThievesDenManager* _thievesDenManager = nullptr;
};
