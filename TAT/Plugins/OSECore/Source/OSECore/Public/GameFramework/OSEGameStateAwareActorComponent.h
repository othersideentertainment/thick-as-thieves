// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "OSEGameStateAwareActorComponent.generated.h"

UCLASS(Blueprintable, BlueprintType)
class OSECORE_API UOSEGameStateAwareActorComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   // from UActorComponent
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
   AGameStateBase* _gameState;
};
