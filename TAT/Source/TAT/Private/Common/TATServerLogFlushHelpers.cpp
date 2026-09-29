// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATServerLogFlushHelpers.h"

// ue
#include "CoreMinimal.h"

namespace TATServerLogFlushHelpers
{

   static float GetFlushInterval()
   {
      float interval = 0;
      FParse::Value(FCommandLine::Get(), TEXT("-StdOutFlushInterval="), interval);
      return interval;
   }

   static void FlushStdOut()
   {
      // This should be fast
      TRACE_CPUPROFILER_EVENT_SCOPE(FlushStdOut)
      fflush(stdout);
   }
   
   void TryInitialize(FTimerManager& timerManager)
   {
      if(!IsRunningDedicatedServer())
      {
         return;
      }

      const float interval = GetFlushInterval();
      if(interval > 0)
      {
         // Logs to stdout may be buffered when in a headless container, so this allows
         // -stdout is another alternative that flushes after each log
         FTimerHandle timerHandle;
         timerManager.SetTimer(timerHandle, TFunction<void()>(FlushStdOut), interval, true);
      }
   }
}


