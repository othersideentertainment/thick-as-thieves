// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Interface.h"

#include "TATLivingWorldAgentInterface.generated.h"

class UTATLivingWorldAgentComponent;

// This class does not need to be modified.
UINTERFACE(BlueprintType, MinimalAPI, Category = "Living World", meta = (CannotImplementInterfaceInBlueprint))
class UTATLivingWorldAgentInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATLivingWorldAgentInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   virtual UTATLivingWorldAgentComponent* GetLivingWorldAgentComponent() const = 0;
};
