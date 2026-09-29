// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

// ose
#include "OSEIndividualKnowledgeComponent.h"

#include "OSEIndividualKnowledgeInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class OSEINDIVIDUALKNOWLEDGE_API UOSEIndividualKnowledgeInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEINDIVIDUALKNOWLEDGE_API IOSEIndividualKnowledgeInterface
{
   GENERATED_BODY()
public:
   virtual UOSEIndividualKnowledgeComponent* GetIndividualKnowledgeComponent() const = 0; 
};
