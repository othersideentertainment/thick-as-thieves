// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "TATOnOffSequence.generated.h"

// Utilities for timed sequences of on/off patterns with charging between off->on
//
// Can be sampled and simulated independently on client and server, as long as the
// start time is synchronized. Each sampling tells the next time to try to sample, so
// timers can be scheduled. But it will recover if clocks drift subsequently.
//
// Initial use-case is shock floors, which integrate the sampling directly (since small),
// but also could make a wrapper component if there is a use-case.

// ? name ?
UENUM(BlueprintType)
enum class ETATOnOffState : uint8
{
   Off,
   Charging,
   On
};

USTRUCT()
struct TAT_API FTATOnOffSequenceEntry
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   bool IsOn = false;

   UPROPERTY(EditAnywhere, meta = (ClampMin=0.1, Units="seconds"))
   float Duration = 1;
};

namespace TATOnOffSequence
{
   struct FCompiledStep
   {
      ETATOnOffState State = ETATOnOffState::Off;
      float Duration = 0;
      float StartTimeInCycle = 0;
   };

   struct FCompiledSequence
   {
      TArray<FCompiledStep> Steps;
      TOptional<FCompiledStep> IntroStep;
      float CycleDuration = 0;
   };

   struct FSampleParams
   {
      float StartTime = 0;
      float CurrentTime = 0;
   };

   struct FSampleResult
   {
      bool Success = false;
      ETATOnOffState State = ETATOnOffState::Off;
      float NextUpdateTime = 0;

      bool ShouldScheduleTimer() const
      {
         return Success && NextUpdateTime >= 0;
      }

      float GetNextTimerDelay(float currentTime) const
      {
         // If it is in the past (which should not happen), still wait some time so it doesn't get stuck
         return FMath::Max(NextUpdateTime - currentTime, 0.05f);
      }
   };

   // TODO: very unit testable
   FCompiledSequence Compile(float chargeTime, TConstArrayView<FTATOnOffSequenceEntry> rawSteps);
   const FCompiledSequence& GetIndefiniteOn(float chargeTime);
   FSampleResult Sample(const FCompiledSequence& sequence, const FSampleParams& params);
}

UCLASS()
class TAT_API UTATOnOffSequence : public UDataAsset
{
   GENERATED_BODY()

public:

   const TATOnOffSequence::FCompiledSequence& GetCompiled(float chargeTime);

#if WITH_EDITOR
   virtual void PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent) override;
#endif
private:
   UPROPERTY(EditAnywhere, meta=(TitleProperty="IsOn={IsOn} Duration={Duration}s"))
   TArray<FTATOnOffSequenceEntry> _entries;

   TMap<float, TATOnOffSequence::FCompiledSequence> _compiledSequenceCache;
};
