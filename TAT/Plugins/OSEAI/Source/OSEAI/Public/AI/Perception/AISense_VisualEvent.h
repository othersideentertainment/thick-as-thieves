// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Perception/AISense.h"

#include "AISense_VisualEvent.generated.h"

USTRUCT(BlueprintType)
struct OSEAI_API FAIVisualEvent
{
   GENERATED_BODY()

   typedef class UAISense_VisualEvent FSenseClass;

   FAIVisualEvent() {}
   FAIVisualEvent(AActor* instigator, FName tag = NAME_None);

   float Age = 0.0f;

   /// Actor triggering the stim.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
   AActor* Instigator = nullptr;

   /// Named identifier for the stim.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
   FName Tag;

   FGenericTeamId TeamIdentifier;

   /// Verifies and calculates derived data.
   void Compile();
};

/// Sense visual events sent via ReportVisualEvent.
/// 
/// This sense depends on AISense_Sight for its raycasts, so make sure to include that sense along
/// with this one.
UCLASS()
class OSEAI_API UAISense_VisualEvent : public UAISense
{
   GENERATED_BODY()

public:

   void RegisterEvent(const FAIVisualEvent& event);
   void RegisterEventsBatch(const TArray<FAIVisualEvent>& events);

   // Part of BP interface. Translates PerceptionEvent to FAIVisualEvent and calls RegisterEvent(const FAIVisualEvent& event)
   virtual void RegisterWrappedEvent(UAISenseEvent& perceptionEvent) override;

   /// Report a visual event.
   ///
   /// @param instigator Actor that triggered the visual. Required.
   /// @param tag Identifier for the event.
   UFUNCTION(BlueprintCallable, Category = "AI|Perception|OSE", meta = (WorldContext="WorldContextObject"))
   static void ReportVisualEvent(UObject* worldContextObject, AActor* instigator = nullptr, FName tag = NAME_None);

protected:

   virtual float Update() override;

   TArray<FAIVisualEvent> VisualEvents;

};
