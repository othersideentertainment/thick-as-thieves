// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue4
#include "CoreMinimal.h"

class AOSECharacterBase;

class FOSEConditionContext 
{
public:
   FOSEConditionContext(const AActor* actor);

   const AActor* GetActor() const { return _actor; }
   const AOSECharacterBase* GetCharacter() const { return _character; }
private:

   const AActor* _actor;

   // Pre-cache conversion to character base.
   const AOSECharacterBase* _character;
};
