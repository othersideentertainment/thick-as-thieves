// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

// ose
#include "OSEVoiceLineKnowledgeComponent.h"
#include "OSEVoiceLineKnowledgeInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class OSEVOICELINEKNOWLEDGE_API UOSEVoiceLineKnowledgeInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEVOICELINEKNOWLEDGE_API IOSEVoiceLineKnowledgeInterface
{
   GENERATED_BODY()

public:
   virtual UOSEVoiceLineKnowledgeComponent* GetVoiceLineKnowledgeComponent() const = 0; 
};
