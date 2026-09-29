// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATDevToolInterface.generated.h"


UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTATDevToolInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATDevToolInterface
{
   GENERATED_BODY()

public:
   virtual void GetDevToolObjectName(FString& objectName) const {}
   virtual void DrawDevToolObjectEditor(float deltaSeconds) {}

};
