// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//ue
#include "Components/EditableText.h"
#include "CoreMinimal.h"

#include "TATSteamEditableText.generated.h"

struct FTATSteamKeyboardTextInputDismissedCallback;

/**
 * An editable text widget that can bring up the virtual steam keyboard if the right conditions are met.
 * (e.g. game is being run in Steam's Big Picture Mode)
 * 
 * This class does not inherit from UUserWidget and thus cannot bind to any focus events.
 * Therefore, a wrapping UUserWidget class has to tell this widget when to show the virtual steam keyboard.
 * (This is why the ShowSteamKeyboard() function is BlueprintCallable)
 */
UCLASS()
class TAT_API UTATSteamEditableText : public UEditableText
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category="TATEditableTextBox")
   bool ShowSteamKeyboard();

private:
   TUniquePtr<FTATSteamKeyboardTextInputDismissedCallback> _steamKeyboardCallback;
};
