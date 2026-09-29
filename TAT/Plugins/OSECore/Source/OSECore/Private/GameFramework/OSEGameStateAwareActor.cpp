// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/OSEGameStateAwareActor.h"

// ue4
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameStateAwareActor)

void AOSEGameStateAwareActor::BeginPlay()
{
   Super::BeginPlay();

   if (UWorld* world = GetWorld())
   {
      if (AGameStateBase* gameState = world->GetGameState())
      {
         _gameState = gameState;
         _OnGameStateSetEvent(_gameState);
      }
      else
      {
         world->GameStateSetEvent.AddUObject(this, &AOSEGameStateAwareActor::_OnGameStateSetEvent);
      }
   }
}

void AOSEGameStateAwareActor::OnGameStateFound_Implementation(AGameStateBase* gameState)
{

}

void AOSEGameStateAwareActor::_OnGameStateSetEvent(AGameStateBase* gameState)
{
   _gameState = gameState;

   // cpp
   _OnGameStateFound(gameState);

   // bp
   OnGameStateFound(gameState);
}

void AOSEGameStateAwareActor::_OnGameStateFound(AGameStateBase* gameState)
{
   // for subclasses to impl
}

