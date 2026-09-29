// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "AI/Perception/TATAISense_Hearing.h"

// ose
#include "Character/OSETeamInterface.h"

// ue4
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig.h"

#include "TATAISenseConfig_Hearing.generated.h"

class FGameplayDebuggerCategory;
class UAIPerceptionComponent;
class UDataTable;

UCLASS(meta = (DisplayName = "TAT AI Hearing Config"))
class TAT_API UTATAISenseConfig_Hearing : public UAISenseConfig
{
   GENERATED_BODY()

public:
   UTATAISenseConfig_Hearing(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense")
   float HearingRange;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FOSEAISenseAffiliationFilter DetectionByAffiliation;

   UPROPERTY(EditDefaultsOnly, Category="Sense")
   UOSEAISenseSharedConfigData* SharedConfigData {nullptr};

   virtual TSubclassOf<UAISense> GetSenseImplementation() const override;

#if WITH_GAMEPLAY_DEBUGGER
   virtual void DescribeSelfToGameplayDebugger(const UAIPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const;
#endif // WITH_GAMEPLAY_DEBUGGER
};
