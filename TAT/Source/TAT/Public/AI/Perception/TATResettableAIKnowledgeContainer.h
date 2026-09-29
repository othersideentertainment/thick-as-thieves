// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATResettableAIKnowledgeContainer.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UTATResettableAIKnowledgeContainer : public UInterface
{
   GENERATED_BODY()
};


class TAT_API ITATResettableAIKnowledgeContainer
{
   GENERATED_BODY()
public:
   virtual void ResetKnowledgeOfActor(AActor* actor) = 0;
};
