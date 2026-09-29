// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "AI/Perception/AISense_VisualEvent.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Perception/AISenseEvent.h"

#include "AISenseEvent_VisualEvent.generated.h"

UCLASS()
class OSEAI_API UAISenseEvent_VisualEvent : public UAISenseEvent
{
   GENERATED_BODY()

protected:

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
   FAIVisualEvent Event;

public:

   virtual FAISenseID GetSenseID() const override;

   FORCEINLINE FAIVisualEvent GetSightEvent()
   {
      Event.Compile();
      return Event;
   }
};
