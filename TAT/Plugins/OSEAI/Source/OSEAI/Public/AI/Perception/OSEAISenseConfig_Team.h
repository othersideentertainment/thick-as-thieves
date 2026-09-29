// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/OSEAISense_Team.h"

// ue4
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig.h"

#include "OSEAISenseConfig_Team.generated.h"

UCLASS(meta = (DisplayName = "OSE AI Team sense config"))
class OSEAI_API UOSEAISenseConfig_Team : public UAISenseConfig
{
   GENERATED_BODY()
public:   
   virtual TSubclassOf<UAISense> GetSenseImplementation() const override { return UOSEAISense_Team::StaticClass(); }
};
