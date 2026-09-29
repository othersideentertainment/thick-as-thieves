// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/SmartObjects/TATActionNodeComponent_Ambient.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"

// ue
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATAmbientSmartObject.generated.h"

UCLASS()
class TAT_API ATATAmbientSmartObject : public AActor, public ITATSmartObjectOwnerInterface
{
   GENERATED_BODY()

public:
   ATATAmbientSmartObject();

   // ITATSmartObjectOwnerInterface start
   UFUNCTION(BlueprintCallable)
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const override { return _ambientNode; }
   // ITATSmartObjectOwnerInterface end

protected:
   UPROPERTY(EditDefaultsOnly, Category="TAT|AI", BlueprintReadOnly)
   UTATActionNodeComponent_Ambient* _ambientNode { nullptr };
};
