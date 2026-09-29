// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/NavigationObjectBase.h"
#include "GameFramework/Actor.h"
#include "TATTutorialRespawnPoint.generated.h"

// A marker actor where the player should respawn during the tuturial
//
// Essentially looks like a PlayerStart without being one
UCLASS()
class TAT_API ATATTutorialRespawnPoint : public ANavigationObjectBase
{
   GENERATED_BODY()

public:
   ATATTutorialRespawnPoint();

private:
#if WITH_EDITORONLY_DATA
   UPROPERTY()
   TObjectPtr<class UArrowComponent> _arrowComponent;
#endif
};
