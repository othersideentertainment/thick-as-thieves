// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Engine/NavigationObjectBase.h"

#include "TATRespawnAreaSpawnLocation.generated.h"

// The spawn location used when within a TATRespawnAreaOverlapVolume
UCLASS()
class TAT_API ATATRespawnAreaSpawnLocation : public ANavigationObjectBase
{
   GENERATED_BODY()

public:
   ATATRespawnAreaSpawnLocation();

private:
#if WITH_EDITORONLY_DATA
   UPROPERTY()
   TObjectPtr<class UArrowComponent> _arrowComponent;
#endif
};
