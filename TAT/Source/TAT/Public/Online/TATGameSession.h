// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"

#include "TATGameSession.generated.h"

/**
 * 
 */
UCLASS()
class TAT_API ATATGameSession : public AGameSession
{
   GENERATED_BODY()

public:
   virtual bool AtCapacity(bool bSpectator) override;
   
   virtual void RegisterPlayer(APlayerController* newPlayer, const FUniqueNetIdRepl& uniqueId, bool wasFromInvite) override;

private:
   int32 _GetPlayerIdFor(const FUniqueNetIdRepl& uniqueId);
   
   int32 _nextPlayerId = INDEX_NONE;
};
