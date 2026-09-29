// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "TATInputSettings.generated.h"

//---------------------------------------------------------------------------------------
// FTATMouseKeyboardSettings
//---------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FTATMouseKeyboardSettings
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintReadWrite, Category = "TAT Input Settings", Config)
   float LookSensitivityTime = 1.0f;

   float GetLookSensitivityValue() const;
};

USTRUCT(BlueprintType)
struct FTATGamepadSettings
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintReadWrite, Category = "TAT Input Settings", Config)
   float LookSensitivityTime = 1.0f;

   float GetLookSensitivityValue() const;
};

//---------------------------------------------------------------------------------------
// UTATInputSettings
//---------------------------------------------------------------------------------------
UCLASS(NotBlueprintable, Config = "TATInput")
class TAT_API UTATInputSettings : public UObject
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "TAT Input Settings")
   static UTATInputSettings* GetTATInputSettings() { return GetMutableDefault<UTATInputSettings>(); }

   UFUNCTION(BlueprintCallable, Category = "TAT Input Settings")
   void SaveInputSettings()
   {
      SaveConfig();
      OnTATInputSettingsConfigChanged.Broadcast();
   }

   DECLARE_MULTICAST_DELEGATE(FOnTATInputSettingsConfigChanged);
   FOnTATInputSettingsConfigChanged OnTATInputSettingsConfigChanged;

   UPROPERTY(BlueprintReadWrite, Category = "TAT Input Settings")
   FTATMouseKeyboardSettings MouseKeyboard;

   UPROPERTY(BlueprintReadWrite, Category = "TAT Input Settings")
   FTATGamepadSettings Gamepad;
};

//---------------------------------------------------------------------------------------
// UTATInputSettings
//---------------------------------------------------------------------------------------
UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Input Settings"))
class TAT_API UTATInputDeveloperSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UTATInputDeveloperSettings();

   // BP access
   UFUNCTION(BlueprintPure, Category = "TAT Input Settings", meta = (DisplayName = "GetTATInputDeveloperSettings"))
   static UTATInputDeveloperSettings* Get() { return GetMutableDefault<UTATInputDeveloperSettings>(); }

   UFUNCTION(BlueprintCallable)
   UCurveFloat* GetMouseLookSensitivityCurve();

   UFUNCTION(BlueprintCallable)
   UCurveFloat* GetGamepadLookSensitivityCurve();

   UPROPERTY(Config, EditAnywhere, Category = "TAT Input Settings", meta = (AllowedClasses = "/Script/Engine.CurveFloat"))
   FSoftObjectPath MouseLookSensitivityCurve;

   UPROPERTY(Config, EditAnywhere, Category = "TAT Input Settings", meta = (AllowedClasses = "/Script/Engine.CurveFloat"))
   FSoftObjectPath GamepadLookSensitivityCurve;
};
