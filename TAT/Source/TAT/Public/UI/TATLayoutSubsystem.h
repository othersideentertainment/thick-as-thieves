// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <GameplayTagContainer.h>
#include <Subsystems/GameInstanceSubsystem.h>

#include "TATLayoutSubsystem.generated.h"

class UTATLayoutWidget;
class UCommonActivatableWidget;

DECLARE_DYNAMIC_DELEGATE_OneParam(FTATLayoutInitFuncDelegate, UCommonActivatableWidget*, widget);

UCLASS(MinimalAPI)
class UTATLayoutSubsystem : public UGameInstanceSubsystem
{
   GENERATED_BODY()

public:
   virtual void Initialize(FSubsystemCollectionBase& Collection) override;

   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "TAT|UI")
   static UTATLayoutWidget* GetPlayerLayout(ULocalPlayer* player);

   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "TAT|UI", meta=(DeterminesOutputType = "WidgetClass"))
   static UCommonActivatableWidget* PushWidget(ULocalPlayer* owningPlayer, UPARAM(meta=(Categories = "UI.Layer")) FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass, FTATLayoutInitFuncDelegate initFunc);

   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "TAT|UI")
   static bool PopWidget(UCommonActivatableWidget* widget);

   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "TAT|UI")
   static bool PopWidgetsByClass(ULocalPlayer* owningPlayer, FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass);

private:
   UTATLayoutWidget* _GetPlayerLayout(ULocalPlayer* player) const;

   void _OnLocalPlayerAdded(ULocalPlayer* player);
   void _OnLocalPlayerRemoved(ULocalPlayer* player);

   UPROPERTY()
   TArray<TObjectPtr<UTATLayoutWidget>> _layoutWidgets;
};
