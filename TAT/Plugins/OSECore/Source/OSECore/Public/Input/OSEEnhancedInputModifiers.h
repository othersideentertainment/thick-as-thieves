// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSEInputSettings.h"

// ue4
#include "CoreMinimal.h"
#include "InputModifiers.h"

#include "OSEEnhancedInputModifiers.generated.h"

class UCurveFloat;

UENUM(BlueprintType)
enum class EInputModifierRequireSignType : uint8
{
   Negative,
   Positive
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierRequireSign
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Requires Sign"))
class UOSEEnhancedInputModifierRequireSign final : public UInputModifier
{
   GENERATED_BODY()

public:
   
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings)
   EInputModifierRequireSignType Sign = EInputModifierRequireSignType::Negative;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "X")
   bool HandleX = true;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Y")
   bool HandleY = true;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Z")
   bool HandleZ = true;

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override
   {
      FVector vec = currentValue.Get<FVector>();
      switch(Sign)
      {
      case EInputModifierRequireSignType::Negative:
         {
            if (HandleX && currentValue[0] > 0.0f)
            {
               vec[0] = 0.0f;
            }
            if (HandleY && currentValue[1] > 0.0f)
            {
               vec[1] = 0.0f;
            }
            if (HandleZ && currentValue[2] > 0.0f)
            {
               vec[2] = 0.0f;
            }
         }
         break;
      case EInputModifierRequireSignType::Positive:
         {
            if (HandleX && currentValue[0] < 0.0f)
            {
               vec[0] = 0.0f;
            }
            if (HandleY && currentValue[1] < 0.0f)
            {
               vec[1] = 0.0f;
            }
            if (HandleZ && currentValue[2] < 0.0f)
            {
               vec[2] = 0.0f;
            }
         }
         break;
      }
      return FInputActionValue(vec).ConvertToType(currentValue.GetValueType());
   }
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierInvertGamepadLook
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[OSE] Setting: Invert Gamepad Look"))
class UOSEEnhancedInputModifierInvertGamepadLook final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override
   {
      if (UOSEInputSettings::GetOSEInputSettings()->Gamepad.InvertLook)
      {
         return currentValue.Get<FVector>() * FVector(1.0f, -1.0f, 1.0f);
      }
      else
      {
         return currentValue;
      }
   }
   FLinearColor GetVisualizationColor_Implementation(FInputActionValue sampleValue, FInputActionValue finalValue) const
   {
      FVector sampleVec = sampleValue.Get<FVector>();
      FVector finalVec = finalValue.Get<FVector>();
      return FLinearColor(1.0f, sampleVec.Y != finalVec.Y ? 1.f : 0.f, 1.0f);
   }
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierInvertMouseLook
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[OSE] Setting: Invert Mouse Look"))
class UOSEEnhancedInputModifierInvertMouseLook final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override
   {
      if (UOSEInputSettings::GetOSEInputSettings()->MouseKeyboard.InvertLook)
      {
         return currentValue.Get<FVector>() * FVector(1.0f, -1.0f, 1.0f);
      }
      else
      {
         return currentValue;
      }
   }
   FLinearColor GetVisualizationColor_Implementation(FInputActionValue sampleValue, FInputActionValue finalValue) const
   {
      FVector sampleVec = sampleValue.Get<FVector>();
      FVector finalVec = finalValue.Get<FVector>();
      return FLinearColor(1.0f, sampleVec.Y != finalVec.Y ? 1.f : 0.f, 1.0f);
   }
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierGamepadMoveSensitivity
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[OSE] Setting: Gamepad Move Sensitivity"))
class UOSEEnhancedInputModifierGamepadMoveSensitivity final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override
   {
      // Don't try and scale bools
      if (ensureMsgf(currentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Sensitivity modifier doesn't support boolean values.")))
      {
         return currentValue.Get<FVector>() * UOSEInputSettings::GetOSEInputSettings()->Gamepad.GetMoveSensitivityValue();
      }
      return currentValue;
   }
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierGamepadLookSensitivity
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[OSE] Setting: Gamepad Look Sensitivity"))
class UOSEEnhancedInputModifierGamepadLookSensitivity final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override
   {
      // Don't try and scale bools
      if (ensureMsgf(currentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Sensitivity modifier doesn't support boolean values.")))
      {
         return currentValue.Get<FVector>() * UOSEInputSettings::GetOSEInputSettings()->Gamepad.GetLookSensitivityValue();
      }
      return currentValue;
   }
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierMouseLookSensitivity
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[OSE] Setting: Mouse Look Sensitivity"))
class UOSEEnhancedInputModifierMouseLookSensitivity final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override
   {
      // Don't try and scale bools
      if (ensureMsgf(currentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Sensitivity modifier doesn't support boolean values.")))
      {
         return currentValue.Get<FVector>() * UOSEInputSettings::GetOSEInputSettings()->MouseKeyboard.GetLookSensitivityValue();
      }
      return currentValue;
   }
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierWallClimbLookAssist
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[OSE] Setting: Wall Climb Look Assist"))
class UOSEEnhancedInputModifierWallClimbLookAssist : public UInputModifier
{
   GENERATED_BODY()

protected:
   FInputActionValue GetScaledAssistInputValue(FInputActionValue currentValue, const FVector& viewDirection, const FVector& viewTarget, float assistRadius, float minScale, float maxScale, bool bReverseLerp = false);
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierWallClimbLookMovementAssist
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[OSE] Setting: Wall Climb Look Movement Assist"))
class UOSEEnhancedInputModifierWallClimbLookMovementAssist final : public UOSEEnhancedInputModifierWallClimbLookAssist
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierWallClimbLookWallAssist
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, Config = "Input", meta = (DisplayName = "[OSE] Setting: Wall Climb Look Wall Assist"))
class UOSEEnhancedInputModifierWallClimbLookWallAssist final : public UOSEEnhancedInputModifierWallClimbLookAssist
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputGamepadEdgeAcceleration
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Gamepad Edge Acceleration"))
class UOSEEnhancedInputGamepadEdgeAcceleration final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;

   // Curve for multiplier on input over time when the magnitude of the input is over the threshold
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, meta = (DisplayThumbnail = "false"))
   class UCurveFloat* BoostOverTime;

   // Minimum input magnitude for the boost to apply
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings)
   float EdgeThreshold = 1;

private:
   // How long input has been at edge
   float _edgeTime = 0.f;
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputSmooth
//---------------------------------------------------------------------------------------

// Fixes bug in UInputModifierSmooth
UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Mouse Smooth"))
class UOSEEnhancedInputMouseSmooth final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;

private:
   void _ClearSmoothedAxis();

private:
#define SMOOTH_TOTAL_SAMPLE_TIME_DEFAULT (0.0083f)
   // Input sampling total time.
   float _totalSampleTime = SMOOTH_TOTAL_SAMPLE_TIME_DEFAULT;
   // How long input has been zero.
   float _zeroTime = 0.f;
   // Current average input/sample
   FInputActionValue _averageValue;
   // Number of samples since input has been zero
   int32 _samples = 0;
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierClampValues
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Clamp Values"))
class UOSEEnhancedInputModifierClampValues final : public UInputModifier
{
   GENERATED_BODY()

public:
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Clamp X")
   bool ClampX = false;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Min X", meta = (EditCondition = "ClampX"))
   float MinX = 0.0f;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Max X", meta = (EditCondition = "ClampX"))
   float MaxX = 0.0f;

   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Clamp Y")
   bool ClampY = false;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Min Y", meta = (EditCondition = "ClampY"))
   float MinY = 0.0f;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Max Y", meta = (EditCondition = "ClampY"))
   float MaxY = 0.0f;

   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Clamp Z")
   bool ClampZ = false;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Min Z", meta = (EditCondition = "ClampZ"))
   float MinZ = 0.0f;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, DisplayName = "Max Z", meta = (EditCondition = "ClampZ"))
   float MaxZ = 0.0f;

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;
};

//------------------------------------------------------------------------------------------------------------------------
// UOSEInputModifierResponseCurveAbsolute
// - similar to UInputModifierResponseCurveUser but only requires curves define 0-1 and we map that in both directions
//------------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Response Curve Absolute (0-1)"))
class UOSEInputModifierResponseCurveAbsolute final : public   UInputModifier
{
   GENERATED_BODY()

public:
   // Whether to apply per-axis response curves or as radial
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings)
   bool bSeparateAxis = false;

   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, meta = (DisplayThumbnail = "false"))
   UCurveFloat* ResponseX;

   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, meta = (DisplayThumbnail = "false", EditCondition = "bSeparateAxis", EditConditionHides))
   UCurveFloat* ResponseY;

   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings, meta = (DisplayThumbnail = "false", EditCondition = "bSeparateAxis", EditConditionHides))
   UCurveFloat* ResponseZ;

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;
};

//------------------------------------------------------------------------------------------------------------------------
// UOSEInputModifierGamepadLookSpeed
// - NOTE: Takes in input in -1 to 1 space and outputs a turning rate for this frame
// - NOTE: Intended to only be used w/ a Gamepad Look Thumbstick
//------------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Gamepad Look Speed"))
class UOSEInputModifierGamepadLookSpeed final : public   UInputModifier
{
   GENERATED_BODY()

public:
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings)
   float HorizontalTurnSpeed = 100.0f;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings)
   float VerticalTurnSpeed = 60.0f;
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings)
   float HorizontalPow = 4.0f;

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;
};

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputFloatingThreshold1D
// 
// Quantized to 0 or 1 using dynamic thresholds based on the high or low watermark
// 
// Loosely based on http://blog.hypersect.com/analog-to-digital-input-translation/
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Floating Threshold 1D"))
class UOSEEnhancedInputFloatingThreshold1D final : public UInputModifier
{
   GENERATED_BODY()

protected:
   virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime) override;

   // Minimum input magnitude (from low water mark) to activate
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings)
   float ActivateThreshold = 0.25;

   // Minimum input magnitude (from high water mark) to deactivate
   UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = Settings)
   float DeactivateThreshold = 0.25;

private:
   float _watermark = 0.f;
   bool _active = false;
};
