// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UI/OSEUserWidget.h"

// ose
#include "Player/OSEPlayerController.h"
#include "UI/AsyncTaskAnimateWidget.h"

// ue5
#include "Animation/WidgetAnimation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEUserWidget)

void UOSEUserWidget::NativeConstruct()
{
   Super::NativeConstruct();
   
   if (ReceiveOnInputHardwareTypeChanged)
   {
      if (UWorld* world = GetWorld())
      {
         if (AOSEPlayerController* pc = AOSEPlayerController::GetLocalOSEPlayerController(world))
         {
            pc->OnInputHardwareTypeChanged.AddUniqueDynamic(this, &UOSEUserWidget::_OnInputHardwareTypeChanged);
         }
      }
   }
}

void UOSEUserWidget::NativeDestruct()
{
   Super::NativeDestruct();

   if (ReceiveOnInputHardwareTypeChanged)
   {
      if (UWorld* world = GetWorld())
      {
         if (AOSEPlayerController* pc = AOSEPlayerController::GetLocalOSEPlayerController(world))
         {
            pc->OnInputHardwareTypeChanged.RemoveAll(this);
         }
      }
   }
}

void UOSEUserWidget::RemoveFromParent()
{
   Super::RemoveFromParent();
   OnWidgetRemovedFromParent.Broadcast();
}

UAsyncTaskAnimateWidget* UOSEUserWidget::GetAsyncAnimateWidgetTask(const UWidgetAnimation* animation) const
{
   if (IsValid(animation))
   {
      if (UAsyncTaskAnimateWidget* const* task = AsyncAnimateWidgetTasks.Find(animation))
      {
         return *task;
      }
   }
   
   return nullptr;
}

bool UOSEUserWidget::AddAsyncAnimateWidgetTask(UAsyncTaskAnimateWidget* task, const UWidgetAnimation* animation)
{
   check(IsValid(task));
   check(IsValid(animation));
   check(task->GetAnimation() == animation);

   // Make sure we don't already have a task associated with this animation
   if (!IsValid(GetAsyncAnimateWidgetTask(animation)))
   {
      AsyncAnimateWidgetTasks.Add(animation, task);
      return true;
   }
   
   UE_LOG(LogAsyncTaskAnimateWidget, Warning, TEXT("AddAsyncAnimateWidgetTask() on widget %s was called, but a task is already bound to animation %s! Make sure to check if one already exists on the widget before spawning/adding a new one."), *GetFName().ToString(), *animation->GetFName().ToString());
   return false;
}

