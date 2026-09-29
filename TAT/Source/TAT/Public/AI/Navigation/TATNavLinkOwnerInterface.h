// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

// tat
#include "TATNavLinkOwnerComponent.h"

#include "TATNavLinkOwnerInterface.generated.h"

UINTERFACE(BlueprintType)
class UTATNavLinkOwnerInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATNavLinkOwnerInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
   UTATNavLinkOwnerComponent* GetNavLinkOwnerComponent() const;
   virtual UTATNavLinkOwnerComponent* GetNavLinkOwnerComponent_Implementation() const = 0;
};
