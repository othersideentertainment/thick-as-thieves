// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATOnOffSequence.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATOnOffSequence)

namespace TATOnOffSequence
{
   FCompiledSequence Compile(float chargeTime, TConstArrayView<FTATOnOffSequenceEntry> rawSteps)
   {
      FCompiledSequence result;

      if(rawSteps.Num() > 0)
      {
         if(rawSteps[0].IsOn && chargeTime > 0)
         {
            result.IntroStep = { .State = ETATOnOffState::Charging, .Duration = chargeTime };
         }

         auto addStep = [&result](ETATOnOffState state, float duration)
         {
            if(duration <= 0)
            {
               return;
            }
            
            result.Steps.Add({
              .State = state,
              .Duration = duration,
              .StartTimeInCycle = result.CycleDuration
            });
            result.CycleDuration += duration;
         };

         for(int i = 0; i < rawSteps.Num(); i++)
         {
            const FTATOnOffSequenceEntry& entry = rawSteps[i];
            const FTATOnOffSequenceEntry& nextEntry = rawSteps[rawSteps.IsValidIndex(i + 1) ? i + 1 : 0];
            if(!entry.IsOn && nextEntry.IsOn)
            {
               addStep(ETATOnOffState::Off, entry.Duration - chargeTime);
               addStep(ETATOnOffState::Charging, FMath::Min(entry.Duration, chargeTime));
            }
            else
            {
               addStep(entry.IsOn ? ETATOnOffState::On : ETATOnOffState::Off, entry.Duration);
            }
         }
      }

      return result;
   }

   const FCompiledSequence& GetIndefiniteOn(float chargeTime)
   {
      static TMap<float, FCompiledSequence> sCache;
      if(const FCompiledSequence* found = sCache.Find(chargeTime))
      {
         return *found;
      }

      constexpr FTATOnOffSequenceEntry entries[] = { { .IsOn = true, .Duration = 60 } }; 
      return sCache.Add(chargeTime, Compile(chargeTime, entries));
   }
   
   FSampleResult Sample(const FCompiledSequence& sequence, const FSampleParams& params)
   {
      const float timeSinceStart = params.CurrentTime - params.StartTime;
      const float firstCycleTime = sequence.IntroStep ? sequence.IntroStep->Duration : 0;
      if(timeSinceStart < firstCycleTime)
      {
         return FSampleResult {
            .Success = true,
            .State = ETATOnOffState::Charging,
            .NextUpdateTime = params.StartTime + firstCycleTime
         };
      }

      const float timeSinceFirstCycle = timeSinceStart - firstCycleTime;
      check(timeSinceFirstCycle >= 0);
      const int cycleCount = (int)(timeSinceFirstCycle / sequence.CycleDuration);
      const float cycleStartTime = cycleCount*sequence.CycleDuration;
      const float timeInCycle = timeSinceFirstCycle - cycleStartTime;
      for(int i = sequence.Steps.Num()-1; i >= 0 ; --i)
      {
         const FCompiledStep& step = sequence.Steps[i];
         if(step.StartTimeInCycle <= timeInCycle)
         {
            const float nextUpdateTime = sequence.Steps.Num() > 1
                                 ? params.StartTime + firstCycleTime + cycleStartTime + step.StartTimeInCycle + step.Duration
                                 : -1;
            return FSampleResult {
               .Success = true,
               .State = step.State,
               .NextUpdateTime = nextUpdateTime
            };
         }
      }

      return FSampleResult { .Success = false };
   }
}


const TATOnOffSequence::FCompiledSequence& UTATOnOffSequence::GetCompiled(float chargeTime)
{
   if(const TATOnOffSequence::FCompiledSequence* found = _compiledSequenceCache.Find(chargeTime))
   {
      return *found;
   }

   return _compiledSequenceCache.Add(chargeTime, TATOnOffSequence::Compile(chargeTime, _entries));
}

#if WITH_EDITOR
void UTATOnOffSequence::PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _compiledSequenceCache.Reset();
}
#endif
