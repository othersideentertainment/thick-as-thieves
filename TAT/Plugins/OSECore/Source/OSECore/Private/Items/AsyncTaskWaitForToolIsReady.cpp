// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/AsyncTaskWaitForToolIsReady.h"

// ose
#include "Items/ToolComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncTaskWaitForToolIsReady)

// ue4

DEFINE_LOG_CATEGORY_STATIC(LogAsyncTaskListenForToolIsReady, Log, All);

/* static */
UAsyncTaskWaitForToolIsReady* UAsyncTaskWaitForToolIsReady::WaitForToolIsReady(UObject* worldContextObject, UToolComponent* toolComponent)
{
   if (toolComponent)
   {
      UAsyncTaskWaitForToolIsReady* listenForToolIsReadyTask = NewObject<UAsyncTaskWaitForToolIsReady>();
      check(listenForToolIsReadyTask);
      listenForToolIsReadyTask->_toolComponent = toolComponent;
      return listenForToolIsReadyTask;
   }
   return nullptr;   
}

void UAsyncTaskWaitForToolIsReady::Activate()
{
   Super::Activate();

   check(_toolComponent.Get());
   if (_toolComponent->IsReady())
   {
      // broadcast/delete self
      ToolIsReady.Broadcast(_toolComponent.Get());
      SetReadyToDestroy();
      MarkAsGarbage();
   }
   else
   {
      // polling at 30fps because that's fast enough?
      float tickRate = 1.0f / 30.0f;
      _toolComponent->GetWorld()->GetTimerManager().SetTimer(_timerHandle, this, &UAsyncTaskWaitForToolIsReady::_TickIsToolReady, tickRate, true);
   }
}

void UAsyncTaskWaitForToolIsReady::_TickIsToolReady()
{
   if (!IsValid(this))
      return;

   bool destroy = false;
   if (_toolComponent.Get())
   {
      if (_toolComponent->IsReady())
      {
         ToolIsReady.Broadcast(_toolComponent.Get());

         // ready to die once we figure out that we're ready
         destroy = true;

         // stop ticking
         _toolComponent->GetWorld()->GetTimerManager().ClearTimer(_timerHandle);
      }
   }
   else
   {
      // ready to die because the component went away at some point
      destroy = true;
   }

   if (destroy)
   {
      _timerHandle.Invalidate();
      SetReadyToDestroy();
      MarkAsGarbage();
   }
}

