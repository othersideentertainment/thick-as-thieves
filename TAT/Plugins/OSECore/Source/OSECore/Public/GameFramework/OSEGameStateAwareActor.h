// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OSEGameStateAwareActor.generated.h"

UCLASS(Blueprintable, BlueprintType)
class OSECORE_API AOSEGameStateAwareActor : public AActor
{
   GENERATED_BODY()

public:
   // from AActor
   virtual void BeginPlay() override;

   UFUNCTION(BlueprintNativeEvent, Category = "OSE GameStateAwareActor")
   void OnGameStateFound(AGameStateBase* gameState);
   
   UFUNCTION(BlueprintPure)
   AGameStateBase* GetGameState() const { return _gameState; }

protected:
   virtual void _OnGameStateFound(AGameStateBase* gameState);

private:
   UFUNCTION()
   void _OnGameStateSetEvent(AGameStateBase* gameState);

private:
   UPROPERTY(Transient)
   AGameStateBase* _gameState;
};
