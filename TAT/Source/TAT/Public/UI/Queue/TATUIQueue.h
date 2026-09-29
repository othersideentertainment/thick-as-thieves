// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATUIQueue.generated.h"

class UTATActivatableWidget;
class UTATScreenWidget;

UCLASS(Abstract)
class TAT_API UTATUIQueueAction : public UObject
{
   GENERATED_BODY()

public:
   // Maybe pre-load some stuff if relevant
   // But just a hint, not guaranteed
   virtual void Prepare() {}

   // Important to _always_ call next even if there is an error
   virtual void Run(APlayerController* controller, TFunction<void()>&& next) {}
};

// declared free because it confused the UHT when using across classes
DECLARE_DYNAMIC_DELEGATE(FTATDynamicQueueDelegate);

UCLASS()
class TAT_API UTATDynamicDelegateQueueAction : public UTATUIQueueAction
{
   GENERATED_BODY()

public:
   
   static UTATUIQueueAction* Create(FTATDynamicQueueDelegate delegate);

   virtual void Run(APlayerController* controller, TFunction<void()>&& next) override;

private:
   UPROPERTY()
   FTATDynamicQueueDelegate _delegate;
};

// CONSIDER: maybe pass through controller parameter? Will test ergonomics when I have a use-case
UCLASS()
class TAT_API UTATSimpleDelegateQueueAction : public UTATUIQueueAction
{
   GENERATED_BODY()

public:
   static UTATUIQueueAction* Create(FSimpleDelegate&& delegate);

   virtual void Run(APlayerController* controller, TFunction<void()>&& next) override;

private:
   FSimpleDelegate _delegate;
};

UCLASS()
class TAT_API UTATScreenQueueAction : public UTATUIQueueAction
{
   GENERATED_BODY()

public:
   using FScreenInitFunction = TUniqueFunction<void(UTATScreenWidget*)>;

   static UTATUIQueueAction* Create(TSoftClassPtr<UTATScreenWidget> screenClass, FScreenInitFunction&& init = FScreenInitFunction());

   virtual void Prepare() override;
   virtual void Run(APlayerController* controller, TFunction<void()>&& next) override;

private:
   TSoftClassPtr<UTATScreenWidget> _screenClass;
   FScreenInitFunction _initScreen;
};

UCLASS()
class TAT_API UTATActivatableWidgetQueueAction : public UTATUIQueueAction
{
   GENERATED_BODY()

public:
   using FWidgetInitFunction = TUniqueFunction<void(UTATActivatableWidget*)>;

   static UTATUIQueueAction* Create(TSoftClassPtr<UTATActivatableWidget> activatableWidgetClass, FWidgetInitFunction&& initDelegate = FWidgetInitFunction());

   virtual void Prepare() override;
   virtual void Run(APlayerController* controller, TFunction<void()>&& next) override;
   void CleanupAction();

private:
   TSoftClassPtr<UTATActivatableWidget> _activatableWidgetClass;
   FWidgetInitFunction _initDelegate;
   FDelegateHandle _OnDeactivatedDelegate;
   UTATActivatableWidget* _activatedWidget = nullptr;
};

UCLASS()
class TAT_API UTATToastQueueAction : public UTATUIQueueAction
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category="Quest|Flow", meta=(DisplayName="Create Toast Action"))
   static UTATUIQueueAction* Create(const FText& toastMessage, const FGameplayTag& toastId);
   virtual void Run(APlayerController* controller, TFunction<void()>&& next) override;

private:
   FText _message;
   FGameplayTag _toastId;
};

/// A simple class to manage a queue of mostly-screens to show in a sequence
///
/// Useful for simple railroad flows. If it diverges (after starting), it probably isn't the tool
/// But it doesn't have to be
UCLASS(BlueprintType)
class TAT_API UTATUIQueue : public UObject
{
   GENERATED_BODY()
   
public:
   // Being controller-parameterized means that screen-or-controller
   // specific stuff doesn't have to worry about that not being available.
   // 
   // I did vacillate on how generic this should be.
   UFUNCTION(BlueprintCallable, meta = (DisplayName = "CreateUIQueue"))
   static UTATUIQueue* Create(APlayerController* controller);
   
   UFUNCTION(BlueprintCallable)
   void AddAction(UTATUIQueueAction* action);
   UFUNCTION(BlueprintCallable, meta = (DisplayName = "AddInstantAction"))
   void AddInstantAction_Dynamic(FTATDynamicQueueDelegate delegate);
   void AddInstantAction(FSimpleDelegate&& delegate);

   UFUNCTION(BlueprintCallable)
   void Run();

   static bool AreAnyRunning(const UWorld* world);


private:
   void _RunNext();

   UPROPERTY()
   TObjectPtr<APlayerController> _controller;

   UPROPERTY()
   TArray<TObjectPtr<UTATUIQueueAction>> _actions;

   UPROPERTY()
   int32 _currentIndex = 0;
};
