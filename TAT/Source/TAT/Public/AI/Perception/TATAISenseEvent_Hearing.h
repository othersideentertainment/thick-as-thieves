// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Perception/TATAISense_Hearing.h"

// ose

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Perception/AISenseEvent.h"

#include "TATAISenseEvent_Hearing.generated.h"

UCLASS()
class TAT_API UTATAISenseEvent_Hearing : public UAISenseEvent
{
   GENERATED_BODY()

public:
   UTATAISenseEvent_Hearing(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());
   virtual FAISenseID GetSenseID() const override;
   
   FORCEINLINE FTATAINoiseEvent GetNoiseEvent()
   {
      Event.Compile();
      return Event;
   }

protected:
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
   FTATAINoiseEvent Event;
};
