// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Settings/TATUserSettingsInputAnyKey.h"

// ue
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUserSettingsInputAnyKey)

class ICursor;

class FSettingsPressAnyKeyInputPreProcessor : public IInputProcessor
{
public:
	FSettingsPressAnyKeyInputPreProcessor()
	{

	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override { }

	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		HandleKey(InKeyEvent.GetKey());
		return true;
	}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		return true;
	}

	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		HandleKey(MouseEvent.GetEffectingButton());
		return true;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		return true;
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		HandleKey(MouseEvent.GetEffectingButton());
		return true;
	}

	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) override
	{
		if (InWheelEvent.GetWheelDelta() != 0)
		{
			const FKey Key = InWheelEvent.GetWheelDelta() < 0 ? EKeys::MouseScrollDown : EKeys::MouseScrollUp;
			HandleKey(Key);
		}
		return true;
	}

	DECLARE_MULTICAST_DELEGATE(FSettingsPressAnyKeyInputPreProcessorCanceled);
	FSettingsPressAnyKeyInputPreProcessorCanceled OnKeySelectionCanceled;

	DECLARE_MULTICAST_DELEGATE_OneParam(FSettingsPressAnyKeyInputPreProcessorKeySelected, FKey);
	FSettingsPressAnyKeyInputPreProcessorKeySelected OnKeySelected;

private:
	void HandleKey(const FKey& Key) const
   {
		// Cancel this process if it's Escape, Touch, or a gamepad key.
		if (Key == EKeys::LeftCommand || Key == EKeys::RightCommand)
		{
			// Ignore
		}
		else if (Key == EKeys::Escape || Key.IsTouch() || Key.IsGamepadKey())
		{
			OnKeySelectionCanceled.Broadcast();
		}
		else
		{
			OnKeySelected.Broadcast(Key);
		}
	}
};

UTATUserSettingsInputAnyKey::UTATUserSettingsInputAnyKey(const FObjectInitializer& initializer)
{
}

void UTATUserSettingsInputAnyKey::NativeOnActivated()
{
   Super::NativeOnActivated();
   bKeySelected = false;

   InputProcessor = MakeShared<FSettingsPressAnyKeyInputPreProcessor>();
   InputProcessor->OnKeySelected.AddUObject(this, &ThisClass::HandleKeySelected);
   InputProcessor->OnKeySelectionCanceled.AddUObject(this, &ThisClass::HandleKeySelectionCanceled);
   FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
}

void UTATUserSettingsInputAnyKey::NativeOnDeactivated()
{
   Super::NativeOnDeactivated();

   if (FSlateApplication::IsInitialized())
   {
      FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
   }
}

void UTATUserSettingsInputAnyKey::HandleKeySelected(FKey InKey)
{
   if (bKeySelected == false)
   {
      bKeySelected = true;
      Dismiss([this, InKey]() {
         OnKeySelected.Broadcast(InKey);
      });
   }
}

void UTATUserSettingsInputAnyKey::HandleKeySelectionCanceled()
{
   if (bKeySelected == false)
   {
      bKeySelected = true;
      Dismiss([this]() {
         OnKeySelectionCanceled.Broadcast();
      });
   }
}

void UTATUserSettingsInputAnyKey::Dismiss(TFunction<void()> PostDismissCallback)
{
   // We delay a tick so that we're done processing input.
   FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, PostDismissCallback](float DeltaTime)
   {
      QUICK_SCOPE_CYCLE_COUNTER(STAT_UTATUserSettingsInputAnyKey_Dismiss);

      FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
      DeactivateWidget();
      PostDismissCallback();

      return false;
   }));
}
