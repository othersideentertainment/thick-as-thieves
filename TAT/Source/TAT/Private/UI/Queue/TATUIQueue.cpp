// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/Queue/TATUIQueue.h"

// tat
#include "UI/TATScreenMgr.h"
#include "UI/TATScreenWidget.h"
#include "UI/TATActivatableWidget.h"
#include "UI/TATLayoutSubsystem.h"
#include "UI/TATLayoutWidget.h"
#include "UI/TATUIFunctionLibrary.h"

// ose
#include "OSECommon.h"

// ue
#include "Engine/AssetManager.h"
#include "GameplayTagsManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUIQueue)

UTATUIQueueAction* UTATDynamicDelegateQueueAction::Create(FTATDynamicQueueDelegate delegate)
{
   auto* action = NewObject<UTATDynamicDelegateQueueAction>();
   action->_delegate = MoveTemp(delegate);
   return action;
}

void UTATDynamicDelegateQueueAction::Run(APlayerController* controller, TFunction<void()>&& next)
{
   _delegate.ExecuteIfBound();
   next();
}

UTATUIQueueAction* UTATSimpleDelegateQueueAction::Create(FSimpleDelegate&& delegate)
{
   auto* action = NewObject<UTATSimpleDelegateQueueAction>();
   action->_delegate = MoveTemp(delegate);
   return action;
}

void UTATSimpleDelegateQueueAction::Run(APlayerController* controller, TFunction<void()>&& next)
{
   _delegate.ExecuteIfBound();
   next();
}

UTATUIQueueAction* UTATScreenQueueAction::Create(TSoftClassPtr<UTATScreenWidget> screenClass, FScreenInitFunction&& init /*= FScreenInitFunction()*/)
{
   auto* action = NewObject<UTATScreenQueueAction>();
   action->_screenClass = screenClass;
   action->_initScreen = MoveTemp(init);
   return action;
}

void UTATScreenQueueAction::Prepare()
{
   // Should I hold onto the handle?
   UAssetManager::GetStreamableManager().RequestAsyncLoad(_screenClass.ToSoftObjectPath());
}

void UTATScreenQueueAction::Run(APlayerController* controller, TFunction<void()>&& next)
{
   check(controller);

   UWorld* world = controller->GetWorld();
   // Do not create the widget as the world is being destroyed.
   if (!world || world->bIsTearingDown)
   {
      next();
      return;
   }

   UTATScreenMgr* screenMgr = UTATScreenMgr::TryGetScreenManager(controller);
   if (screenMgr == nullptr)
   {
      next();
      return;
   }

   UTATScreenWidget* screen = CreateWidget<UTATScreenWidget>(controller, _screenClass.LoadSynchronous());
   if (screen == nullptr)
   {
      next();
      return;
   }

   if (_initScreen)
   {
      _initScreen(screen);
   }
   screenMgr->AddScreen(screen, [next = MoveTemp(next)](UTATScreenWidget* screen) { next(); });
}

UTATUIQueueAction* UTATActivatableWidgetQueueAction::Create(TSoftClassPtr<UTATActivatableWidget> activatableWidgetClass, FWidgetInitFunction&& initDelegate /*= FWidgetInitFunction()*/)
{
   auto* action = NewObject<UTATActivatableWidgetQueueAction>();
   action->_activatableWidgetClass = activatableWidgetClass;
   action->_initDelegate = MoveTemp(initDelegate);

   return action;
}

void UTATActivatableWidgetQueueAction::Prepare()
{
   UAssetManager::GetStreamableManager().RequestAsyncLoad(_activatableWidgetClass.ToSoftObjectPath());
}

void UTATActivatableWidgetQueueAction::Run(APlayerController* controller, TFunction<void()>&& next)
{
   UWorld* world = controller->GetWorld();
   // Do not display the widget as the world is being destroyed
   if (!world || world->bIsTearingDown)
   {
      next();
      return;
   }

   UTATLayoutSubsystem* layoutSubsystem = controller->GetGameInstance()->GetSubsystem<UTATLayoutSubsystem>();
   if (layoutSubsystem == nullptr)
   {
      next();
      return;
   }
   _activatedWidget = Cast<UTATActivatableWidget>(layoutSubsystem->PushWidget(controller->GetLocalPlayer(), Tag_UI_Layer_Modal, _activatableWidgetClass.LoadSynchronous(), FTATLayoutInitFuncDelegate()));

   if (!IsValid(_activatedWidget))
   {
      next();
      return;
   }

   if (_initDelegate)
   {
      _initDelegate(_activatedWidget);
   }

   _OnDeactivatedDelegate = _activatedWidget->OnDeactivated().AddLambda([weakThis = MakeWeakObjectPtr(this), next = MoveTemp(next)]()
      {
         // Copy the delegate to avoid it being null after we have cleaned up the action and crashing the game.
         TFunction<void()> nextDelegate = next;

         if (UTATActivatableWidgetQueueAction* self = weakThis.Get())
         {
            self->CleanupAction();
         }

         nextDelegate();
      });
}

void UTATActivatableWidgetQueueAction::CleanupAction()
{
   if (IsValid(_activatedWidget) && _OnDeactivatedDelegate.IsValid())
   {
      _activatedWidget->OnDeactivated().Remove(_OnDeactivatedDelegate);
      _activatedWidget = nullptr;
      _OnDeactivatedDelegate.Reset();
   }
}

UTATUIQueueAction* UTATToastQueueAction::Create(const FText& toastMessage, const FGameplayTag& toastId)
{
   auto* action = NewObject<UTATToastQueueAction>();
   action->_message = toastMessage;
   action->_toastId = toastId;
   return action;
}

void UTATToastQueueAction::Run(APlayerController* controller, TFunction<void()>&& next)
{
   UWorld* world = controller->GetWorld();
   // Do not request a toast as the world is being destroyed.
   if (!world || world->bIsTearingDown)
   {
      next();
      return;
   }

   UTATUIFunctionLibrary::RequestToastIfLocallyControlled(controller->GetPawn(), _toastId, _message);
   next();
}

UTATUIQueue* UTATUIQueue::Create(APlayerController* controller)
{
   if (!ensure(controller))
   {
      return nullptr;
   }
   UTATUIQueue* queue = NewObject<UTATUIQueue>(controller);
   queue->_controller = controller;
   return queue;
}

void UTATUIQueue::AddAction(UTATUIQueueAction* action)
{
   if (action)
   {
      _actions.Add(action);
   }
}

void UTATUIQueue::AddInstantAction_Dynamic(FTATDynamicQueueDelegate delegate)
{
   AddAction(UTATDynamicDelegateQueueAction::Create(MoveTemp(delegate)));
}

void UTATUIQueue::AddInstantAction(FSimpleDelegate&& delegate)
{
   AddAction(UTATSimpleDelegateQueueAction::Create(MoveTemp(delegate)));
}

void UTATUIQueue::Run()
{
   if (_actions.IsValidIndex(_currentIndex))
   {
      // Adding world's ExtraReferencedObjects to prevent being GC-ed while running
      // some risk of leaking if action does not call next, but won't last beyond
      // the lifetime of the world, so any leak is finite

      if (UWorld* world = GetWorld())
      {
         world->ExtraReferencedObjects.AddUnique(this);
      }
      _RunNext();
   }
}

bool UTATUIQueue::AreAnyRunning(const UWorld* world)
{
   if(world == nullptr)
   {
      return false;
   }

   // We add and remove when a queue starts or stops running, so this should be equivalent
   return world->ExtraReferencedObjects.ContainsByPredicate(
      [](const UObject* object) { return object && object->IsA(UTATUIQueue::StaticClass()); });
}

void UTATUIQueue::_RunNext()
{
   if (!_actions.IsValidIndex(_currentIndex))
   {
      // must be complete
      if (UWorld* world = GetWorld())
      {
         world->ExtraReferencedObjects.RemoveSingleSwap(this);
      }
      return;
   }

   if (_actions.IsValidIndex(_currentIndex + 1))
   {
      _actions[_currentIndex + 1]->Prepare();
   }

   UTATUIQueueAction* action = _actions[_currentIndex];
   check(action);
   _currentIndex++;

   // need a clear path after here, since it could recurse
   action->Run(_controller, [weakSelf = MakeWeakObjectPtr(this)]() {
      if (UTATUIQueue* self = weakSelf.Get())
      {
         self->_RunNext();
      }
   });
}
