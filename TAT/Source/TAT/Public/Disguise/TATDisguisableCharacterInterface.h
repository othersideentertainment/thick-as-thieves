// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Items/Disguise/TATDisguiseTool.h"

// ue5
#include "UObject/Interface.h"

#include "TATDisguisableCharacterInterface.generated.h"

UINTERFACE(BlueprintType)
class TAT_API UTATDisguisableCharacterInterface : public UInterface
{
   GENERATED_BODY()
};

class UTATDisguiseComponent;

class TAT_API ITATDisguisableCharacterInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "TAT|Disguise")
   UTATDisguiseComponent* GetDisguiseComponent() const;
   virtual UTATDisguiseComponent* GetDisguiseComponent_Implementation() const = 0;

   UFUNCTION(BlueprintNativeEvent, Category = "TAT|Disguise")
   void ApplyDisguiseVisuals(const FDisguiseSnapshot& disguiseSnapshot);
   virtual void ApplyDisguiseVisuals_Implementation(const FDisguiseSnapshot& disguiseSnapshot) { }

   UFUNCTION(BlueprintNativeEvent, Category = "TAT|Disguise")
   void RevertDisguiseVisuals();
   virtual void RevertDisguiseVisuals_Implementation() { }
};
