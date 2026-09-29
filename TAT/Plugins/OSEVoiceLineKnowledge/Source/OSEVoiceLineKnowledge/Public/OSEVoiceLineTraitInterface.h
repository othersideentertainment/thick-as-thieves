// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OSEVoiceLineTraitInterface.generated.h"

struct FGameplayTagContainer;
// This class does not need to be modified.
UINTERFACE()
class OSEVOICELINEKNOWLEDGE_API UOSEVoiceLineTraitInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEVOICELINEKNOWLEDGE_API IOSEVoiceLineTraitInterface
{
   GENERATED_BODY()
public:
   virtual void GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const = 0; 
};
