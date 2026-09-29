// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Engine/NavigationObjectBase.h"
#include "GameFramework/Actor.h"

#include "TATGuardDefensiveLocation.generated.h"

UCLASS(BlueprintType, Placeable)
class TAT_API ATATGuardDefensiveLocation : public ANavigationObjectBase
{
   GENERATED_BODY()

public:
   ATATGuardDefensiveLocation(const FObjectInitializer& objectInitializer);
#if WITH_EDITORONLY_DATA
private:
   /** Arrow component to indicate forward direction of start */
   UPROPERTY()
   TObjectPtr<class UArrowComponent> _arrowComponent;
#endif
};
