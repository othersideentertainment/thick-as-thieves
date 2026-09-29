// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Input/OSEEnhancedInputPriority.h"

// ue4
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "OSEInputSettings.generated.h"

class UEnhancedAbilityInputActionsAsset;
class UOSECharacterInputActionsAsset;
enum class EOSEInputHardwareType : uint8;

UENUM(BlueprintType)
enum class EOSEInputSensitivityType : uint8
{
   VeryLow,
   Low,
   Normal,
   Fast,
   VeryFast,

   MAX              UMETA(Hidden)
};

//---------------------------------------------------------------------------------------
// FOSEGamepadInputSettings
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FOSEGamepadInputSettings
{
   GENERATED_BODY()

public:
   /// Do we invert the Y axis on the gamepad look?
   UPROPERTY(BlueprintReadWrite, Category = "OSE Input Settings", Config)
   bool InvertLook = false;

   /// If true, pressing the sprint key will start sprinting until we press it again (or are forced to stop)
   UPROPERTY(BlueprintReadWrite, Category = "OSE Input Settings", Config)
   bool SprintToggle = true;
   
   /// How sensitive is the move thumbstick?
   UPROPERTY(BlueprintReadWrite, Category = "OSE Input Settings", Config)
   EOSEInputSensitivityType MoveSensitivity = EOSEInputSensitivityType::Normal;
   float GetMoveSensitivityValue() const;

   /// How sensitive is the look thumbstick?
   UPROPERTY(BlueprintReadWrite, Category = "OSE Input Settings", Config)
   EOSEInputSensitivityType LookSensitivity = EOSEInputSensitivityType::Normal;
   float GetLookSensitivityValue() const;
};

//---------------------------------------------------------------------------------------
// FOSEGamepadMouseKeyboardSettings
//---------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FOSEGamepadMouseKeyboardSettings
{
   GENERATED_BODY()

public:
   /// Do we invert the mouse look axis?
   UPROPERTY(BlueprintReadWrite, Category = "OSE Input Settings", Config)
   bool InvertLook = false;

   /// If true, pressing the sprint key will start sprinting until we press it again (or are forced to stop)
   UPROPERTY(BlueprintReadWrite, Category = "OSE Input Settings", Config)
   bool SprintToggle = false;

   /// How sensitive is mouse look?  1.0 = default
   UPROPERTY(BlueprintReadWrite, Category = "OSE Input Settings", Config)
   EOSEInputSensitivityType LookSensitivity = EOSEInputSensitivityType::Normal;
   float GetLookSensitivityValue() const;
};

//---------------------------------------------------------------------------------------
// UOSEInputSettings
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Config = "OSEInput")
class OSECORE_API UOSEInputSettings : public UObject
{
   GENERATED_BODY()

public:

   //---------------------------------------------------------------------------------------
   // Get/Save
   //---------------------------------------------------------------------------------------

   /// Get the input settings object
   UFUNCTION(BlueprintPure, Category = "OSE Input Settings")
   static UOSEInputSettings* GetOSEInputSettings() { return GetMutableDefault<UOSEInputSettings>(); }

   /// Call this to save the OSE Input Settings to Input.ini
   UFUNCTION(BlueprintCallable, Category = "OSE Input Settings")
   void SaveInputSettings()
   {
      SaveConfig();

      OnInputSettingsConfigChanged.Broadcast();
   }

   DECLARE_MULTICAST_DELEGATE(FOnInputSettingsConfigChanged);
   FOnInputSettingsConfigChanged OnInputSettingsConfigChanged;

   UFUNCTION(BlueprintPure, CAtegory = "OSE Input Settings")
   bool IsSprintToggleSetForInputHardwareType(EOSEInputHardwareType inputHardwareType) const;
   
   //---------------------------------------------------------------------------------------
   // Gamepad Input Settings
   //---------------------------------------------------------------------------------------

   /// Gamepad input settings
   UPROPERTY(BlueprintReadWrite, Category = "OSE Input Settings", Config)
   FOSEGamepadInputSettings Gamepad;

   //---------------------------------------------------------------------------------------
   // Mouse/Keyboard Input Settings
   //---------------------------------------------------------------------------------------

   // Mouse/keyboard Input Settings
   UPROPERTY(BlueprintReadWrite, Category = "OSE Input Settings", Config)
   FOSEGamepadMouseKeyboardSettings MouseKeyboard;
};

//---------------------------------------------------------------------------------------
// UOSEInputDeveloperSettings
//---------------------------------------------------------------------------------------

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] Input Settings"))
class OSECORE_API UOSEInputDeveloperSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UOSEInputDeveloperSettings();
   static const UOSEInputDeveloperSettings& Get() { return *GetDefault<UOSEInputDeveloperSettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, EditFixedSize, Category = "OSE Input Settings")
   TMap<EOSEInputSensitivityType, float> MouseLookSensitivity;
   UPROPERTY(Config, EditDefaultsOnly, EditFixedSize, Category = "OSE Input Settings")
   TMap<EOSEInputSensitivityType, float> GamepadLookSensitivity;
   UPROPERTY(Config, EditDefaultsOnly, EditFixedSize, Category = "OSE Input Settings")
   TMap<EOSEInputSensitivityType, float> GamepadMoveSensitivity;

   // Project Default Character Input Mapping Contexts
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "OSE Input Settings|Character Defaults")
   TArray<FOSEInputContextPriority> DefaultCharacterInputMappingContexts;

   // Project Default Character Input Actions
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "OSE Input Settings|Character Defaults")
   TSoftObjectPtr<UOSECharacterInputActionsAsset> DefaultCharacterInputActionsAsset;

   // Project Default Enhanced Input Action Settings
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "OSE Input Settings|Character Defaults")
   TSoftObjectPtr<UEnhancedAbilityInputActionsAsset> DefaultEnhancedAbilityInputActionsAsset;
};
