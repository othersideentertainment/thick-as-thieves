// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATSafeRoomClaimAffectedInterface.generated.h"

class ATATSafeRoom;

UINTERFACE(BlueprintType, MinimalAPI, Category = "Safe Room", meta = (CannotImplementInterfaceInBlueprint))
class UTATSafeRoomClaimAffectedInterface : public UInterface
{
   GENERATED_BODY()
};

/// Some actors (e.g. recalling stones) are not directly tied to safe rooms, but may still want to know if they're
/// within the bounds of a safe room that is then claimed. This interface will let them receive such events,
/// provided they also have a collider that can overlap with the safe room overlap volume
///
/// NOTE: Safe Rooms are soft DEPRECATED pending removal, and should not be used going forward
class TAT_API ITATSafeRoomClaimAffectedInterface
{
   GENERATED_BODY()

public:
   /// Called on authority if we are inside the bounds of a safe room that was just claimed
   /// NOTE: Currently may be called multiple times per claim: this is not currently an issue
   /// but if it is we can de-dupe them in the calling code
   virtual void AuthorityOnSurroundingSafeRoomClaimed(ATATSafeRoom* safeRoom) { }
};

