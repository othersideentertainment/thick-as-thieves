// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Settings/TATUserSettingsCollection_General.h"

// tat
#include "Settings/TATGameUserSettings.h"
#include "Input/TATInputSettings.h"

// OSE
#include "Input/OSEInputSettings.h"

// UE
#include <Internationalization/Culture.h>
#include <Internationalization/Internationalization.h>
#include <Internationalization/TextLocalizationManager.h>


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUserSettingsCollection_General)

namespace GeneralSettingsHelpers
{
   static void ApplyReducedCameraBounceSettings(bool& newValue)
   {
      static IConsoleVariable* cvarReducedBounce = IConsoleManager::Get().FindConsoleVariable(TEXT("tat.Player.ReducedCameraBounce"));
      if (cvarReducedBounce)
      {
         cvarReducedBounce->Set(newValue, ECVF_SetByGameSetting);
      }
   }
}

UTATUserSettingsCollection_General::UTATUserSettingsCollection_General()
{
   CollectionTag = Tag_Settings_General;
   FName tableId = GetSettingsStringTableId();

   CreateSetting<FString>(
      Tag_Settings_General_Language, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.General.Language")),
         .GetOptions = TTATUserSettingOptionsDelegate<FString>::CreateUObject(this, &ThisClass::GetLanguageOptions),
         .OnApplyValue = TTATUserSettingValueDelegate<FString>::CreateUObject(this, &ThisClass::ApplyLanguage)
      });
   
   CreateSetting<bool>(
      Tag_Settings_General_AllowAnalytics, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Gameplay.AllowAnalytics"))
      });

   CreateSetting<bool>(
      Tag_Settings_General_SprintToggle,
      {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Gameplay.SprintToggle")),
         .OnApplyValue = TTATUserSettingValueDelegate<bool>::CreateUObject(this, &ThisClass::ApplySprintToggle)
      });

   CreateSetting<bool>(
      Tag_Settings_General_ReducedCameraBounce, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Gameplay.ReducedCameraBounce")),
         .OnApplyValue = TTATUserSettingValueDelegate<bool>::CreateStatic(GeneralSettingsHelpers::ApplyReducedCameraBounceSettings)
      });

   CreateSetting<float>(
      Tag_Settings_General_MouseSensitivityRel,
      {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Mouse.MouseSensitivity")),
         .OnApplyValue = TTATUserSettingValueDelegate<float>::CreateUObject(this, &ThisClass::ApplyMouseSensitivity)
      });

   CreateSetting<bool>(
      Tag_Settings_General_MouseInvertYAxis,
      {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Mouse.MouseInvertYAxis")),
         .OnApplyValue = TTATUserSettingValueDelegate<bool>::CreateUObject(this, &ThisClass::ApplyMouseInvertYAxis)
      });

   CreateSetting<float>(
      Tag_Settings_General_GamepadSensitivityRel,
      {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Gamepad.MouseSensitivity")),
         .OnApplyValue = TTATUserSettingValueDelegate<float>::CreateUObject(this, &ThisClass::ApplyGamepadSensitivity)
      });

   CreateSetting<bool>(
      Tag_Settings_General_GamepadInvertYAxis,
      {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Gamepad.GamepadInvertYAxis")),
         .OnApplyValue = TTATUserSettingValueDelegate<bool>::CreateUObject(this, &ThisClass::ApplyGamepadInvertYAxis)
      });
}

void UTATUserSettingsCollection_General::LoadSettings()
{
   Super::LoadSettings();
   
   FString language;
   if (GetSetting(Tag_Settings_General_Language, language) && language.IsEmpty())
   {
      const TArray<FString> cultureNames = FTextLocalizationManager::Get().GetLocalizedCultureNames(ELocalizationLoadFlags::Game);
      const FCultureRef osCulture = FInternationalization::Get().GetDefaultCulture();
      if (cultureNames.Contains(osCulture->GetName()))
      {
         SetSetting(Tag_Settings_General_Language, osCulture->GetName());
      }
      else
      {
         SetSetting(Tag_Settings_General_Language, FString(TEXT("en")));
      }
      ApplySetting(Tag_Settings_General_Language);
   }
}

void UTATUserSettingsCollection_General::GetLanguageOptions(TMap<FString, FText>& options)
{
   TArray<FString> cultureNames = FTextLocalizationManager::Get().GetLocalizedCultureNames(ELocalizationLoadFlags::Game);
   for (const FString& cultureName : cultureNames)
   {
      if (FCulturePtr culture = FInternationalization::Get().GetCulture(cultureName))
      {
         options.Add(culture->GetName(), FText::FromString(culture->GetNativeName()));
      }
   }
}

void UTATUserSettingsCollection_General::ApplyLanguage(FString& language)
{
   if (FInternationalization::Get().SetCurrentCulture(language))
   {
      GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *language, GGameUserSettingsIni);
   }
}

void UTATUserSettingsCollection_General::GetSensitivityOptions(TMap<EOSEInputSensitivityType, FText>& options)
{
   FName tableId = GetSettingsStringTableId();
   options.Add(EOSEInputSensitivityType::VeryLow, FText::FromStringTable(tableId,TEXT("Settings.InputSensitivity.VeryLow")));
   options.Add(EOSEInputSensitivityType::Low, FText::FromStringTable(tableId,TEXT("Settings.InputSensitivity.Low")));
   options.Add(EOSEInputSensitivityType::Normal, FText::FromStringTable(tableId,TEXT("Settings.InputSensitivity.Normal")));
   options.Add(EOSEInputSensitivityType::Fast, FText::FromStringTable(tableId,TEXT("Settings.InputSensitivity.Fast")));
   options.Add(EOSEInputSensitivityType::VeryFast, FText::FromStringTable(tableId,TEXT("Settings.InputSensitivity.VeryFast")));
}

void UTATUserSettingsCollection_General::ApplyMouseInvertYAxis(bool& bValue)
{
   UOSEInputSettings* settings = UOSEInputSettings::GetOSEInputSettings();
   if (settings)
   {
      settings->MouseKeyboard.InvertLook = bValue;
   }
}

void UTATUserSettingsCollection_General::ApplyMouseSensitivity(float& curveTime)
{
   UTATInputSettings* settings = UTATInputSettings::GetTATInputSettings();
   if (settings)
   {
      settings->MouseKeyboard.LookSensitivityTime = curveTime;
   }
}

void UTATUserSettingsCollection_General::ApplyGamepadInvertYAxis(bool& bValue)
{
   UOSEInputSettings* settings = UOSEInputSettings::GetOSEInputSettings();
   if (settings)
   {
      settings->Gamepad.InvertLook = bValue;
   }
}

void UTATUserSettingsCollection_General::ApplyGamepadSensitivity(float& curveTime)
{
   UTATInputSettings* settings = UTATInputSettings::GetTATInputSettings();
   if (settings)
   {
      settings->Gamepad.LookSensitivityTime = curveTime;
   }
}

void UTATUserSettingsCollection_General::ApplySprintToggle(bool& bValue)
{
   UOSEInputSettings* settings = UOSEInputSettings::GetOSEInputSettings();
   if (settings)
   {
      settings->MouseKeyboard.SprintToggle = bValue;
      settings->Gamepad.SprintToggle = bValue;

      settings->OnInputSettingsConfigChanged.Broadcast();
   }
}
