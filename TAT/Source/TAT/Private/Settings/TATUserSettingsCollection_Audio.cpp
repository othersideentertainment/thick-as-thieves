// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Settings/TATUserSettingsCollection_Audio.h"

// Wwise
#include "AkAudioDevice.h"
#include "Internationalization/StringTableRegistry.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUserSettingsCollection_Audio)

UTATUserSettingsCollection_Audio::UTATUserSettingsCollection_Audio()
{
   CollectionTag = Tag_Settings_Audio;
   FName tableId = GetSettingsStringTableId();

   CreateSetting<float>(
      Tag_Settings_Audio_MasterVolume, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Volume.MasterVolume")),
         .OnSetValue = TTATUserSettingValueDelegate<float>::CreateUObject(this, &ThisClass::SetVolume, TEXT("System_Volume_Master")),
      });

   CreateSetting<float>(
      Tag_Settings_Audio_MusicVolume, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Volume.MusicVolume")),
         .OnSetValue = TTATUserSettingValueDelegate<float>::CreateUObject(this, &ThisClass::SetVolume, TEXT("System_Volume_Music")),
      });

   CreateSetting<float>(
      Tag_Settings_Audio_EffectsVolume, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Volume.SFXVolume")),
         .OnSetValue = TTATUserSettingValueDelegate<float>::CreateUObject(this, &ThisClass::SetVolume, TEXT("System_Volume_SFX_All")),
      });

   CreateSetting<float>(
      Tag_Settings_Audio_DialogueVolume, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Volume.DialogueVolume")),
         .OnSetValue = TTATUserSettingValueDelegate<float>::CreateUObject(this, &ThisClass::SetVolume, TEXT("System_Volume_VO")),
      });
}

void UTATUserSettingsCollection_Audio::SetVolume(float& volume, const TCHAR* rtpcName)
{
   volume = FMath::Clamp(volume, 0.0f, 100.0f);

   FAkAudioDevice* audioDevice = FAkAudioDevice::Get();
   if (audioDevice)
   {
      audioDevice->SetRTPCValue(rtpcName, volume, 0.0f, nullptr);
   }
}
