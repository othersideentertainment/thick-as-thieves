// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATLayoutSubsystem.h"

// TAT
#include "UI/TATLayoutWidget.h"
#include "UI/TATUIDeveloperSettings.h"
#include "UI/TATUIZOrder.h"

// UE
#include <CommonActivatableWidget.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLayoutSubsystem)

void UTATLayoutSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
   Super::Initialize(Collection);

   UGameInstance* gameInstance = GetGameInstance();

   gameInstance->OnLocalPlayerAddedEvent.AddUObject(this, &ThisClass::_OnLocalPlayerAdded);
   gameInstance->OnLocalPlayerRemovedEvent.AddUObject(this, &ThisClass::_OnLocalPlayerRemoved);
}

UTATLayoutWidget* UTATLayoutSubsystem::GetPlayerLayout(ULocalPlayer* player)
{
   if (!player)
   {
      return nullptr;
   }

   const UTATLayoutSubsystem* layoutSubsystem = UGameInstance::GetSubsystem<UTATLayoutSubsystem>(player->GetGameInstance());
   if (!layoutSubsystem)
   {
      return nullptr;
   }

   return layoutSubsystem->_GetPlayerLayout(player);
}

UCommonActivatableWidget* UTATLayoutSubsystem::PushWidget(ULocalPlayer* owningPlayer, FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass, FTATLayoutInitFuncDelegate initFunc)
{
   const UTATLayoutWidget* playerLayout = GetPlayerLayout(owningPlayer);
   if (!playerLayout)
   {
      return nullptr;
   }

   return playerLayout->PushWidget<UCommonActivatableWidget>(layer, widgetClass, [initFunc](UCommonActivatableWidget& widget)
   {
      initFunc.ExecuteIfBound(&widget);
   });
}

bool UTATLayoutSubsystem::PopWidget(UCommonActivatableWidget* widget)
{
   if (!widget)
   {
      return false;
   }

   const UTATLayoutWidget* playerLayout = GetPlayerLayout(widget->GetOwningLocalPlayer());
   if (!playerLayout)
   {
      return false;
   }

   return playerLayout->PopWidget(widget);
}

bool UTATLayoutSubsystem::PopWidgetsByClass(ULocalPlayer* owningPlayer, FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass)
{
   if (!widgetClass)
   {
      return false;
   }

   const UTATLayoutWidget* playerLayout = GetPlayerLayout(owningPlayer);
   if (!playerLayout)
   {
      return false;
   }

   return playerLayout->PopWidgetsByClass(layer, widgetClass);
}

UTATLayoutWidget* UTATLayoutSubsystem::_GetPlayerLayout(ULocalPlayer* player) const
{
   const int32 playerLayoutIdx = _layoutWidgets.IndexOfByPredicate(
      [player](const UTATLayoutWidget* layoutWidget)
      {
         return layoutWidget->GetOwningLocalPlayer() == player;
      });

   return playerLayoutIdx != INDEX_NONE ? _layoutWidgets[playerLayoutIdx] : nullptr;
}

void UTATLayoutSubsystem::_OnLocalPlayerAdded(ULocalPlayer* player)
{
   player->OnPlayerControllerChanged().AddWeakLambda(this, [this, player](APlayerController* playerController)
   {
      UTATLayoutWidget* playerLayout = _GetPlayerLayout(player);
      if (playerLayout)
      {
         playerLayout->RemoveFromParent();
      }

      if (playerController)
      {
         if (!playerLayout)
         {
            const UTATUIDeveloperSettings* settings = GetDefault<UTATUIDeveloperSettings>();
            const TSubclassOf<UTATLayoutWidget> layoutWidgetClass = settings->LayoutWidgetClass.LoadSynchronous();

            playerLayout = layoutWidgetClass ? CreateWidget<UTATLayoutWidget>(playerController, layoutWidgetClass) : nullptr;
            if (ensureMsgf(playerLayout, TEXT("Failed to create player layout (%s)"), *GetNameSafe(layoutWidgetClass)))
            {
               _layoutWidgets.Add(playerLayout);
            }
         }

         if (playerLayout)
         {
            // #TODO: Prefer AddToPlayerScreen but clashes with TATScreenMgr widgets; switch back once fully migrated to CommonUI
            playerLayout->AddToViewport(TATUIZOrder::CommonUILayout);
         }
      }
   });
}

void UTATLayoutSubsystem::_OnLocalPlayerRemoved(ULocalPlayer* player)
{
   const int32 playerLayoutIdx = _layoutWidgets.IndexOfByPredicate(
      [player](const UTATLayoutWidget* layoutWidget)
      {
         return layoutWidget->GetOwningLocalPlayer() == player;
      });

   if (playerLayoutIdx != INDEX_NONE)
   {
      UTATLayoutWidget* playerLayout = _layoutWidgets[playerLayoutIdx];
      playerLayout->RemoveFromParent();

      _layoutWidgets.RemoveAtSwap(playerLayoutIdx);
   }
}
