// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATCombatPositioningInterface.generated.h"

class UTATCombatPositioningComponent;
// This class does not need to be modified.
UINTERFACE(BlueprintType, MinimalAPI, Category = "AI|OSE|Utility")
class UTATCombatPositioningInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATCombatPositioningInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintNativeEvent)
   UTATCombatPositioningComponent* GetTATCombatPositioningComponent() const;
   virtual UTATCombatPositioningComponent* GetTATCombatPositioningComponent_Implementation() const = 0;   
};
