// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"

#include "OSEPingableInterface.generated.h"

UENUM(BlueprintType)
enum class EOSEPingSpawnType : uint8
{
   AtPingWorldLocation,
   AtSpecificWorldLocation,
   AttachToActor,
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEPingSpawnInfo
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Spawn Info")
   EOSEPingSpawnType SpawnType = EOSEPingSpawnType::AtPingWorldLocation;

   // when using EOSEPingSpawnType::AtSpecificWorldLocation this is the world location to spawn
   // when using EOSEPingSpawnType::AttachToActor this is the relative offset from the AttachComponent
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "SpawnType == EOSEPingSpawnType::AtSpecificWorldLocation || SpawnType == EOSEPingSpawnType::AttachToActor", EditConditionHides), Category = "Ping Spawn Info")
   FVector Location = FVector(ForceInit);

   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "SpawnType == EOSEPingSpawnType::AttachToActor", EditConditionHides), Category = "Ping Spawn Info")
   USceneComponent* AttachComponent = nullptr;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "SpawnType == EOSEPingSpawnType::AttachToActor", EditConditionHides), Category = "Ping Spawn Info")
   FName AttachSocket;
};

UINTERFACE(BlueprintType, Category = "Ping System")
class OSECORE_API UOSEPingableInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IOSEPingableInterface
{
   GENERATED_BODY()

public:
   // Return true if we should be able to ping this actor, and false if we should not
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pingable")
   bool IsPingable() const;

   // Which ping do we apply when this object is single clicked?
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pingable")
   FGameplayTagContainer GetDefaultSingleInputPingTag() const;

   // Which ping do we apply when this object is double clicked?
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pingable")
   FGameplayTagContainer GetDefaultDoubleInputPingTag() const;

   // Where should we anchor the ping when this object is tagged?
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pingable")
   void GetPingSpawnInfo(FOSEPingSpawnInfo& spawnInfo) const;
};
