// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "UI/OSEActivatableWidgetInterface.h"

// ue4
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSEUIFunctionLibrary.generated.h"

class AOSEPlayerController;
class UPanelWidget;
class UWidgetAnimation;

// Enum used to split return execution pins in AnimateWidgetReversible()
UENUM()
enum class EAnimResult : uint8
{
   AnimStarted,
   AnimChangedDirection,
   AnimAlreadyPlaying
};

UCLASS()
class OSECORE_API UOSEUIFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   
   UFUNCTION(BlueprintPure, Category = "User Interface|OSE")
   static bool GetIsMobileUIMode();
   
   UFUNCTION(BlueprintPure, Category = "User Interface|OSE")
   static UWidget* GetFocusedChild(const UPanelWidget* panelWidget, APlayerController* playerController);

   // Animates a widget in the specified direction. If already playing, the animation is reversed in place (or continues playing) to match the specified direction.
   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "User Interface|OSE", meta = (ExpandEnumAsExecs = "outAnimResult", DefaultToSelf = "userWidget"))
   static void AnimateWidgetReversible(UUserWidget* userWidget, UWidgetAnimation* animation, EAnimResult& outAnimResult, const float startAtTime = 0.0f, const int32 numLoops = 1, const float playbackSpeed = 1.0f, const bool restoreState = false, const bool forward = true);

   static bool SetInputMappingContext(ULocalPlayer* localPlayer, bool enabled, const FOSEInputContextPriority& inputContext);

   UFUNCTION(BlueprintCallable, DisplayName = "[OSE] Set Input Mapping Context (With Priority)", Category = "User Interface|OSE")
   static bool BP_SetInputMappingContext(APlayerController* localPlayerController, bool enabled, const FOSEInputContextPriority& inputContext);

   UFUNCTION(BlueprintCallable, DisplayName = "Activate Widget (Activatable Widget Interface)", Category = "User Interface|OSE")
   static bool ActivatableWidgetInterface_Activate(UWidget* widget);

   UFUNCTION(BlueprintCallable, DisplayName = "Deactivate Widget (Activatable Widget Interface)", Category = "User Interface|OSE")
   static bool ActivatableWidgetInterface_Deactivate(UWidget* widget);

   UFUNCTION( BlueprintPure, Category = "Widget|Event Reply" )
   static FEventReply NavigateInDirection( UPARAM( ref ) FEventReply& reply, EUINavigation direction );
};
