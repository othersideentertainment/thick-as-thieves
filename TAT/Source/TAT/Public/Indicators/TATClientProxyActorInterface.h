// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATClientProxyActorInterface.generated.h"

USTRUCT(BlueprintType)
struct TAT_API FTATClientProxySpawnParams
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Spawn Params")
   TObjectPtr<AActor> Instigator;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Spawn Params")
   uint8 CustomData = 0;
};

UINTERFACE(BlueprintType, MinimalAPI)
class UTATClientProxyActorInterface : public UInterface
{
   GENERATED_BODY()
};

///
/// Interface for a client proxy actor - that is, an actor that is spawned directly on a client instead of being replicated
/// normally by the server. This allows spawning the actor on specific clients (eg. just for players that have a certain
/// ability active) instead of always spawning it on all clients.
/// See also: UTATThiefVisionSubsystem
///
class TAT_API ITATClientProxyActorInterface
{
   GENERATED_BODY()

public:

   /// Called when a client proxy actor is spawned, just BEFORE BeginPlay is called.
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "TAT|Client Proxy Actor")
   void OnSpawnedAsClientProxy(float remainingLifeSpan, float indicatorLifeSpan, const FTATClientProxySpawnParams& spawnParams);
   virtual void OnSpawnedAsClientProxy_Implementation(float remainingLifeSpan, float indicatorLifeSpan, const FTATClientProxySpawnParams& spawnParams) {}

   /// Called when a client proxy actor should be destroyed.
   /// It should always destroy itself after receiving this event, although it may play a fade out or transition effect first before doing so.
   /// NB. If you fail to destroy or schedule the destruction of this actor after this event has been fired, it will stick around forever!
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "TAT|Client Proxy Actor")
   void OnDestroyClientProxy();
   virtual void OnDestroyClientProxy_Implementation() {}

   /// Called when a client proxy actor should change its visibility.
   /// After BeginPlay, this function will always be called to set up visibility state.
   /// You can use this to play any kind of fade/transition/animate effect to show/hide the actor.
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "TAT|Client Proxy Actor")
   void OnClientProxyActorSetVisible(bool newVisible);
   virtual void OnClientProxyActorSetVisible_Implementation(bool newVisible) {}

   /// Called when a client proxy actor is refreshed, which means that its lifespan has been extended.
   /// This generally happens when an indicator has been deduplicated (eg. instead of spawning another nearby indicator, this one should be kept around longer instead).
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "TAT|Client Proxy Actor")
   void OnClientProxyActorLifeSpanRefreshed(float newRemainingLifeSpan);
   virtual void OnClientProxyActorLifeSpanRefreshed_Implementation(float newRemainingLifeSpan) {}

};
