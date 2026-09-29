// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/OSEGameStateAwareActorComponent.h"

// ue4
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameStateAwareActorComponent)

void UOSEGameStateAwareActorComponent::BeginPlay()
{
   Super::BeginPlay();

   if (UWorld* world = GetWorld())
   {
      if (AGameStateBase* gameState = world->GetGameState())
      {
         _gameState = gameState;
         _OnGameStateFound(gameState);
      }
      else
      {
         world->GameStateSetEvent.AddUObject(this, &UOSEGameStateAwareActorComponent::_OnGameStateSetEvent);
      }
   }
}

void UOSEGameStateAwareActorComponent::OnGameStateFound_Implementation(AGameStateBase* gameState)
{

}

void UOSEGameStateAwareActorComponent::_OnGameStateSetEvent(AGameStateBase* gameState)
{
   _gameState = gameState;

   // cpp
   _OnGameStateFound(gameState);

   // bp
   OnGameStateFound(gameState);
}

void UOSEGameStateAwareActorComponent::_OnGameStateFound(AGameStateBase* gameState)
{
   // for subclasses to impl
}

