// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"

#include "TATSafeRoomPlayerStart.generated.h"

class ATATPlayerState;
class ATATSafeRoom;

/// A PlayerStart that is associated with a specific safe room
///
/// NOTE: Safe Rooms are soft DEPRECATED pending removal, and should not be used going forward
UCLASS(BlueprintType, HideCategories = (Object, Rendering, Replication, Input, LOD, Cooking))
class TAT_API ATATSafeRoomPlayerStart : public APlayerStart
{
   GENERATED_BODY()

public:

   ATATSafeRoomPlayerStart(const FObjectInitializer& objectInitializer);

   bool IsSafeRoomAvailableForInitialAssignment() const;
   void AuthorityAssignPlayerToSafeRoom(ATATPlayerState* playerState);


#if WITH_EDITOR
   // from UObject
   virtual void CheckForErrors() override;
#endif
   
public:
   UPROPERTY(EditInstanceOnly, Category = "SafeRoom")
   TObjectPtr<ATATSafeRoom> SafeRoom;
};
