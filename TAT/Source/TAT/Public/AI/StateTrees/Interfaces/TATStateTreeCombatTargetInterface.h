// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATStateTreeCombatTargetInterface.generated.h"

class AOSECharacterBase;
// This class does not need to be modified.
UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class UTATStateTreeCombatTargetInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATStateTreeCombatTargetInterface
{
   GENERATED_BODY()
public:
   virtual void OnEnterTargetedByStateTree(AOSECharacterBase* aiCharacter) = 0;
   virtual void OnExitTargetedByStateTree(AOSECharacterBase* aiCharacter) = 0;
};
