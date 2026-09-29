// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Character/OSETeamInterface.h"

// ue
#include "CoreMinimal.h"
#include "TATSessionGameMode.h"

#include "TATPvPGameMode.generated.h"

UCLASS()
class TAT_API ATATPvPGameMode : public ATATSessionGameMode
{
   GENERATED_BODY()

public:
   ATATPvPGameMode();

protected:
   virtual uint8 _GetTeamIndexForNewPlayer(APlayerController* newPlayer) override;
   
private:
   uint8 _nextTeamAssignment = IOSETeamInterface::kInvalidTeam;
};
