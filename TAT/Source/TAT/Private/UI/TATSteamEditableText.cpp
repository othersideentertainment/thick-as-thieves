// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATSteamEditableText.h"

// steam
#include "steam/steam_api_common.h"
#include "steam/isteamutils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSteamEditableText)

DEFINE_LOG_CATEGORY_STATIC(LogTATSteamEditableText, Log, All);

struct FTATSteamKeyboardTextInputDismissedCallback
{
   static constexpr uint32 kMaxStrLen = 127;
   static constexpr uint32 kBufferLen = kMaxStrLen + 1;

   TWeakObjectPtr<UTATSteamEditableText> WeakEditableTextWidget;

   STEAM_CALLBACK(FTATSteamKeyboardTextInputDismissedCallback, OnGamepadTextInputDismissed, GamepadTextInputDismissed_t);

   explicit FTATSteamKeyboardTextInputDismissedCallback(UTATSteamEditableText* steamEditableTextWidget)
      : WeakEditableTextWidget(steamEditableTextWidget)
   {
   }
};

bool UTATSteamEditableText::ShowSteamKeyboard()
{
   if (HasAnyFlags(RF_ClassDefaultObject))
   {
      UE_LOG(LogTATSteamEditableText, Error, TEXT("UTATSteamEditableText::ShowSteamKeyboard(): Can't call this from a CDO"));
      return false;
   }

   if (ISteamUtils* steamUtils = SteamUtils())
   {
      // set up the Steam API callback lazily
      if (_steamKeyboardCallback == nullptr)
      {
         _steamKeyboardCallback = MakeUnique<FTATSteamKeyboardTextInputDismissedCallback>(this);
      }
      if (!ensure(_steamKeyboardCallback))
      {
         return false;
      }

      bool successfullyShownSteamKeyboard = steamUtils->ShowGamepadTextInput(
         EGamepadTextInputMode::k_EGamepadTextInputModeNormal,
         EGamepadTextInputLineMode::k_EGamepadTextInputLineModeSingleLine,
         reinterpret_cast<const char*>(StringCast<UTF8CHAR>(*GetHintText().ToString()).Get()),
         FTATSteamKeyboardTextInputDismissedCallback::kMaxStrLen,
         reinterpret_cast<const char*>(StringCast<UTF8CHAR>(*GetText().ToString()).Get()));

      // We don't really need to do anything on failure.  The assumption here is that if a player has started the
      // game without Big Picture mode, then they have a Mouse and Keyboard available to them to type into this box.
      if (!successfullyShownSteamKeyboard)
      {
         UE_LOG(LogTATSteamEditableText, Warning, TEXT("UTATSteamEditableText::ShowSteamKeyboard(): SteamUtils() was valid but the Steam Keyboard was not shown.  This function only works if Steam is in Big Picture Mode!"));
      }

      return successfullyShownSteamKeyboard;
   }
   UE_LOG(LogTATSteamEditableText, Warning, TEXT("UTATSteamEditableText::ShowSteamKeyboard(): SteamUtils() was NOT valid when trying to show Steam Keyboard!"));
   return false;
}

void FTATSteamKeyboardTextInputDismissedCallback::OnGamepadTextInputDismissed(GamepadTextInputDismissed_t* pCallback)
{
   if (pCallback->m_bSubmitted)
   {
      ISteamUtils* steamUtils = SteamUtils();
      check(steamUtils != nullptr);

      // We cannot call SetText() from a Steam Thread, so cache the entered text
      char strBuffer[kBufferLen] = {};
      if (steamUtils->GetEnteredGamepadTextInput(strBuffer, sizeof(strBuffer)))
      {
         const int32 strLength = FMath::Clamp(static_cast<int32>(steamUtils->GetEnteredGamepadTextLength()), 0, static_cast<int32>(kMaxStrLen));
         const FUtf8StringView strView{ strBuffer, strLength };
         FText enteredText = FText::FromString(FString(strView));

         // Then start an async task to call it on the Game Thread
         // Use Weak Object Ptr in case user closes the menu immediately after entering the text
         TWeakObjectPtr<UTATSteamEditableText> WeakThis = WeakEditableTextWidget;
         AsyncTask(ENamedThreads::GameThread, [WeakThis, enteredText]()
         {
             if (WeakThis.IsValid())
             {
                 WeakThis->SetText(enteredText);
             }
         });
      }
      else
      {
         UE_LOG(LogTATSteamEditableText, Warning, TEXT("FTATSteamKeyboardTextInputDismissedCallback::OnGamepadTextInputDismissed(): GetEnteredGamepadTextInput() returned false!"));
      }
   }
}
