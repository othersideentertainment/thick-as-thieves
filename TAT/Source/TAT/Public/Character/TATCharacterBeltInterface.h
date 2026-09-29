// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "UObject/Interface.h"

#include "TATCharacterBeltInterface.generated.h"

class UTATCharacterBeltComponent;

//---------------------------------------------------------------------------------------
// ITATCharacterBeltInterface
//---------------------------------------------------------------------------------------

UINTERFACE(BlueprintType, MinimalAPI, Category = "Belt", meta = (CannotImplementInterfaceInBlueprint))
class UTATCharacterBeltInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATCharacterBeltInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category = "Belt")
   virtual UTATCharacterBeltComponent* GetCharacterBeltComponent() const = 0;
};
