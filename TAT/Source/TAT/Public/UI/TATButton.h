// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// TAT
#include "UI/TATTabButtonInterface.h"

// UE
#include <CommonButtonBase.h>
#include <Input/CommonBoundActionButtonInterface.h>

#include "TATButton.generated.h"

class UAkAudioEvent;
class UCommonTextBlock;

UCLASS(MinimalAPI, Abstract, meta=(DisableNativeTick))
class UTATButton : public UCommonButtonBase, public ICommonBoundActionButtonInterface, public ITATTabButtonInterface
{
   GENERATED_BODY()

public:
   virtual void NativePreConstruct() override;
   virtual void NativeDestruct() override;

   virtual void NativeOnCurrentTextStyleChanged() override;

   virtual void NativeOnHovered() override;
   virtual void NativeOnUnhovered() override;

   virtual void NativeOnSelected(bool bBroadcast) override;
   virtual void NativeOnDeselected(bool bBroadcast) override;

   virtual void NativeOnPressed() override;
   virtual void NativeOnReleased() override;

   virtual void NativeOnClicked() override;

   virtual void SetRepresentedAction(FUIActionBindingHandle bindingHandle) override;
   virtual void SetRepresentedTab_Implementation(UTATTabList* tabList, const FTATTabDescriptor& tabDescriptor) override;

   UFUNCTION(BlueprintCallable)
   void SetButtonText(const FText& text);

protected:
   virtual void UpdateInputActionWidget() override;

   void BindRepresentedActionEvents();
   void UnbindRepresentedActionEvents();

   bool bRepresentsActionBinding = false;
   FUIActionBindingHandle RepresentedActionBindingHandle;
   FSimpleDelegate RepresentedActionExecuteEvent;

   UPROPERTY(BlueprintReadOnly, Category = CommonButton, meta=(BindWidgetOptional))
   TObjectPtr<UCommonTextBlock> ButtonTextWidget;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CommonButton)
   FText ButtonText;

   UPROPERTY(Transient, meta=(BindWidgetAnimOptional))
   TObjectPtr<UWidgetAnimation> HoveredAnim;

   UPROPERTY(Transient, meta=(BindWidgetAnimOptional))
   TObjectPtr<UWidgetAnimation> SelectedAnim;

   UPROPERTY(Transient, meta=(BindWidgetAnimOptional))
   TObjectPtr<UWidgetAnimation> PressedAnim;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sound)
   TObjectPtr<UAkAudioEvent> HoveredSoundEvent;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sound)
   TObjectPtr<UAkAudioEvent> SelectedSoundEvent;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sound)
   TObjectPtr<UAkAudioEvent> PressedSoundEvent;
};
