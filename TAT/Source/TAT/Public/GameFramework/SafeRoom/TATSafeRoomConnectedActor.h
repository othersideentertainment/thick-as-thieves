// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "GameFramework/Actor.h"

#include "TATSafeRoomConnectedActor.generated.h"

class ATATSafeRoom;

// An actor that is meant to be connected to a particular safe room
///
/// NOTE: Safe Rooms are soft DEPRECATED pending removal, and should not be used going forward
UCLASS()
class TAT_API ATATSafeRoomConnectedActor : public AActor
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintPure)
   ATATSafeRoom* GetSafeRoom() const { return _safeRoom; }

#if WITH_EDITOR
   // from UObject
   virtual void CheckForErrors() override;
#endif

protected:
   UPROPERTY(EditInstanceOnly, Category = SafeRoom)
   TObjectPtr<ATATSafeRoom> _safeRoom;
}; 
