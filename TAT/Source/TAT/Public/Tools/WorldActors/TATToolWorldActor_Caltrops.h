// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATToolWorldActor_AOE.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"
#include "TATToolWorldActor_Caltrops.generated.h"

UCLASS()
class TAT_API ATATToolWorldActor_Caltrops : public ATATToolWorldActor_AOE
   , public ITATSmartObjectOwnerInterface
   , public ITATUtilityAITargetingGroupInterface
{
   GENERATED_BODY()

public:
   ATATToolWorldActor_Caltrops();
   // from ITATSmartObjectOwnerInterface
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const override;
   
   // from ITATUtilityAITargetingGroupInterface
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;
protected:
   UPROPERTY(EditDefaultsOnly)
   class UTATSmartObjectComponent* _smartObjectComponent = nullptr;
};
