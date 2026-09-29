// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat

// ue4
#include "CoreMinimal.h"

#include "TATScreenMgr.generated.h"

class ATATPlayerController;
class UTATScreenWidget;
class UAkStateValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTATScreenMgr, Log, All);

USTRUCT()
struct TAT_API FTATScreenMgrEntry
{
   GENERATED_BODY()

   UPROPERTY()
   TObjectPtr<UTATScreenWidget> Widget;

   TFunction<void(UTATScreenWidget*)> OnRemovedCallback;
};

UCLASS(BlueprintType)
class TAT_API UTATScreenMgr : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATScreenMgr();

   // from AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   // statics
   UFUNCTION(BlueprintCallable, Category = "TAT Screen", meta = (WorldContext = "contextObj"))
   static UTATScreenMgr* TryGetScreenManager(const UObject* contextObj);
   UFUNCTION(BlueprintCallable, Category = "TAT Screen", meta = (WorldContext = "contextObj"))
   static void AddScreenToViewport(const UObject* contextObj, UTATScreenWidget* widget);
   UFUNCTION(BlueprintCallable, Category = "TAT Screen", meta = (WorldContext = "contextObj"))
   static void RemoveScreenFromViewport(const UObject* contextObj, UTATScreenWidget* widget);

   void AddScreen(UTATScreenWidget* widget, TFunction<void(UTATScreenWidget*)> onRemoveCallback = nullptr);
   void RemoveScreen(UTATScreenWidget* widget, int32 indexHint = INDEX_NONE);
   void RemoveAllScreensFromStack();

   UFUNCTION(BlueprintCallable, Category = "TAT Screen")
   UTATScreenWidget* GetTopScreen() const;

   // WARNING - Calling this on a widget that is already the top widget in the stack could cause the ignore input counts to increase too much.
   // Make sure to reduce count before calling. (see TATScreenWidget::_SetInputState and _SetIgnoreAbilityInput.
   // This function was only meant as a fix while transitioning to CommonUI to fix TVT-8607 and TVT-8677.
   UFUNCTION(BlueprintCallable, Category = "TAT Screen")
   void RestoreFocusToTopScreen() const;

   UFUNCTION(BlueprintPure, Category = "TAT Screen")
   bool IsScreenInStack(const UTATScreenWidget* widget) const;
   UFUNCTION(BlueprintPure, Category = "TAT Screen")
   int GetNumScreensInStack() const { return _screenStack.Num(); }   

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScreenAddedToStack, UTATScreenWidget*, widget);
   UPROPERTY(BlueprintAssignable, Category = "TAT Screen")
   FOnScreenAddedToStack OnScreenAddedToStack;
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScreenRemovedFromStack, UTATScreenWidget*, widget);
   UPROPERTY(BlueprintAssignable, Category = "TAT Screen")
   FOnScreenRemovedFromStack OnScreenRemovedFromStack;

private:
   void _RecalculateInputState();
   ATATPlayerController& _GetOwnerPC() const;
   
   void _LoadAkStates();
   void _RefreshAkState();

   bool _ShouldRefreshAkState() const;

   void _OnMoviePlaybackFinished();

private:
   UPROPERTY()
   TArray<FTATScreenMgrEntry> _screenStack;

   UPROPERTY(Transient)
   TObjectPtr<UAkStateValue> _akStateWhileStackPopulated;
   UPROPERTY(Transient)
   TObjectPtr<UAkStateValue> _akStateWhileStackEmpty;

   bool _recalcInputStatePending = false;
};
