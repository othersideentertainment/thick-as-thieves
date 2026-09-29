// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "AI/Perception/AISense_VisualEvent.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig.h"

#include "AISenseConfig_VisualEvent.generated.h"

class FGameplayDebuggerCategory;
class UAIPerceptionComponent;

UCLASS(meta = (DisplayName = "AI Visual Event config"))
class OSEAI_API UAISenseConfig_VisualEvent : public UAISenseConfig
{
   GENERATED_BODY()

public:

   UAISenseConfig_VisualEvent();

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", NoClear, config)
   TSubclassOf<UAISense_VisualEvent> Implementation;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FAISenseAffiliationFilter DetectionByAffiliation;

   virtual TSubclassOf<UAISense> GetSenseImplementation() const override;

#if WITH_GAMEPLAY_DEBUGGER
   virtual void DescribeSelfToGameplayDebugger(const UAIPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const override;
#endif // WITH_GAMEPLAY_DEBUGGER
};
