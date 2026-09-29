// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Input/OSEEnhancedInputModifiers.h"

// ose
#include "Player/OSEPlayerController.h"
#include "Character/OSECharacterBase.h"
#include "Character/OSECharacterMovement.h"

// ue4
#include "EnhancedPlayerInput.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEEnhancedInputModifiers)

DEFINE_LOG_CATEGORY_STATIC(LogOSEInputModifiers, Log, All);

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputMouseSmooth
//---------------------------------------------------------------------------------------

void UOSEEnhancedInputMouseSmooth::_ClearSmoothedAxis()
{
   _zeroTime = 0.f;
   _averageValue.Reset();
   _samples = 0;
   _totalSampleTime = SMOOTH_TOTAL_SAMPLE_TIME_DEFAULT;
}

FInputActionValue UOSEEnhancedInputMouseSmooth::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   AOSEPlayerController* playerController = CastChecked<AOSEPlayerController>(playerInput->GetOuterAPlayerController());

   FInputActionValue startVal = currentValue;

   int32 sampleCount = playerController->GetMouseNumSamples();

   // TODO: This could be fired multiple times if modifiers are badly set up, breaking sample count/deltatime updates.

   if (_averageValue.GetMagnitudeSq() != 0.0f || currentValue.GetMagnitudeSq() != 0.0f)
   {
      _totalSampleTime += deltaTime;
      _samples += sampleCount;
   }

   // a frame with no mouse samples; use our rolling average until we have more data to continue smoothing
   if (sampleCount == 0)
   {
      return _averageValue;
   }

   // TODO: Config val instead?
   static const float kLongFrameTime = 0.25f;

   if (deltaTime < kLongFrameTime)
   {
      if (_samples > 0 && _totalSampleTime > 0.0f)
      {
         // this is seconds/sample
         const float axisSamplingTime = _totalSampleTime / _samples;
         check(axisSamplingTime > 0.0f);

         if (currentValue.GetMagnitudeSq() && sampleCount > 0)
         {
            _zeroTime = 0.0f;
            if (_averageValue.GetMagnitudeSq())
            {
               // this isn't the first tick with non-zero mouse movement
               if (deltaTime < axisSamplingTime * (sampleCount + 1))
               {
                  // smooth mouse movement so samples/tick is constant
                  currentValue *= deltaTime / (axisSamplingTime * sampleCount);
                  sampleCount = 1;
               }
            }

            _averageValue = currentValue * (1.0f / sampleCount);
         }
         else
         {
            // no mouse movement received
            if (_zeroTime < axisSamplingTime)
            {
               // zero mouse movement is possibly because less than the mouse sampling interval has passed
               currentValue = _averageValue.ConvertToType(currentValue) * (deltaTime / axisSamplingTime);
            }
            else
            {
               _ClearSmoothedAxis();
            }

            // increment length of time we've been at zero
            _zeroTime += deltaTime;
         }
      }
   }
   else
   {
      // if we had an abnormally long frame, clear everything so it doesn't distort the results
      _ClearSmoothedAxis();
   }

   // reset the sample counter
   playerController->ResetMouseNumSamples();

   UE_LOG(LogOSEInputModifiers, Verbose, TEXT("%02f|%.02f => %.02f|%.02f || Samples = %d, SampleTime = %.02f"), startVal[0], startVal[1], currentValue[0], currentValue[1], _samples, _totalSampleTime);

   return currentValue;
}

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputModifierClampValues
//---------------------------------------------------------------------------------------

FInputActionValue UOSEEnhancedInputModifierClampValues::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   FVector val = currentValue.Get<FVector>();
   if (ClampX)
   {
      val.X = FMath::Clamp(val.X, MinX, MaxX);
   }
   if (ClampY)
   {
      val.Y = FMath::Clamp(val.Y, MinY, MaxY);
   }
   if (ClampZ)
   {
      val.Z = FMath::Clamp(val.Z, MinZ, MaxZ);
   }
   return FInputActionValue(val).ConvertToType(currentValue.GetValueType());
}

//---------------------------------------------------------------------------------------
// UOSEEnhancedInputGamepadEdgeAcceleration
//---------------------------------------------------------------------------------------

FInputActionValue UOSEEnhancedInputGamepadEdgeAcceleration::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   FVector responseValue = currentValue.Get<FVector>();
   const float sizeSquared = responseValue.SizeSquared();
   if (sizeSquared < FMath::Square(EdgeThreshold))
   {
      _edgeTime = 0;
      return currentValue;
   }

   _edgeTime += deltaTime;
   float additionalBoost = BoostOverTime ? BoostOverTime->GetFloatValue(_edgeTime) : 1.f;
   return responseValue.GetSafeNormal() * (FMath::Sqrt(sizeSquared) * additionalBoost);
}

//---------------------------------------------------------------------------------------
// UOSEInputModifierResponseCurveAbsolute
//---------------------------------------------------------------------------------------

namespace OSEInputModifierResponseCurveAbsoluteUtl
{
   float GetCurveValue(UCurveFloat* curve, float currentValue)
   {
      const float sign = (currentValue >= 0.0f) ? 1.0f : -1.0f;
      const float value = curve ? curve->GetFloatValue(FMath::Abs(currentValue)) : 0.0f;
      return value * sign;
   }
}

FInputActionValue UOSEInputModifierResponseCurveAbsolute::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   FVector responseValue = currentValue.Get<FVector>();

   if (bSeparateAxis)
   {
      switch (currentValue.GetValueType())
      {
         case EInputActionValueType::Axis3D:
            responseValue.Z = OSEInputModifierResponseCurveAbsoluteUtl::GetCurveValue(ResponseZ, responseValue.Z);
            //[[fallthrough]];
         case EInputActionValueType::Axis2D:
            responseValue.Y = OSEInputModifierResponseCurveAbsoluteUtl::GetCurveValue(ResponseY, responseValue.Y);
            //[[fallthrough]];
         case EInputActionValueType::Axis1D:
         case EInputActionValueType::Boolean:
            responseValue.X = OSEInputModifierResponseCurveAbsoluteUtl::GetCurveValue(ResponseX, responseValue.X);
            break;
      }
   }
   else
   {
      switch (currentValue.GetValueType())
      {
         case EInputActionValueType::Boolean:
         case EInputActionValueType::Axis1D:
         {
            responseValue.X = OSEInputModifierResponseCurveAbsoluteUtl::GetCurveValue(ResponseX, responseValue.X);
            break;
         }

         case EInputActionValueType::Axis2D:
         case EInputActionValueType::Axis3D:
         {
            const float currentMagnitude = currentValue.GetMagnitude();
            if (currentMagnitude > UE_SMALL_NUMBER)
            {
               const float remappedMagnitude = OSEInputModifierResponseCurveAbsoluteUtl::GetCurveValue(ResponseX, currentMagnitude);
               responseValue = (responseValue / currentMagnitude) * remappedMagnitude;
            }

            break;
         }
      }
   }

   return responseValue;
}

//------------------------------------------------------------------------------------------------------------------------
// UOSEInputModifierGamepadLookSpeed
//------------------------------------------------------------------------------------------------------------------------

FInputActionValue UOSEInputModifierGamepadLookSpeed::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   FVector responseValue = currentValue.Get<FVector>();

   // when we configure this in our modifier list, we are going to assume that this comes in as a -1 => 1 value
   // and that we're not placing it after a value that takes it out of range, like edge acceleration
   ensure(responseValue.X >= -1.0f && responseValue.X <= 1.0f);
   ensure(responseValue.Y >= -1.0f && responseValue.Y <= 1.0f);

   if (!responseValue.IsZero())
   {
      // algorithm from https://www.gdcvault.com/play/1017942/Techniques-for-Building-Aim-Assist

      // Aiming w/ vertical bias:

      const float x = responseValue.X;
      const float y = responseValue.Y;
      const float rawAbsX = FMath::Abs(x);
      const float absX = FMath::Pow(FMath::Abs(x), HorizontalPow);
      const float absY = FMath::Abs(y);

      const float cameraSpeed = ((HorizontalTurnSpeed * absX) + (VerticalTurnSpeed * absY)) / (absX + absY);
      const float horizontalTurnSpeed = cameraSpeed * x * rawAbsX;
      const float verticalTurnSpeed = cameraSpeed * y;

      // Final result takes into account horizontal/vertical turn speed and dt to result in a turn rate for this frame
      responseValue.X = horizontalTurnSpeed * deltaTime;
      responseValue.Y = verticalTurnSpeed * deltaTime;

      // leaving this in but commented out; useful for eyeballing what would have been our previous behavior vs this behavior...
      //UE_LOG(LogTemp, Log, TEXT("Raw: x = %.02f, y = %.02f"), x, y);
      //UE_LOG(LogTemp, Log, TEXT("Was: x = %.02f, y = %.02f"), x * HorizontalTurnSpeed * deltaTime, y * VerticalTurnSpeed * deltaTime);
      //UE_LOG(LogTemp, Log, TEXT("Now: x = %.02f, y = %.02f"), responseValue.X, responseValue.Y);
   }
   
   return responseValue;
};

FInputActionValue UOSEEnhancedInputFloatingThreshold1D::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   float magnitude = currentValue.GetMagnitude();

   if (_active)
   {
      _watermark = FMath::Max(_watermark, magnitude);

      const float deactivateThreshold = FMath::Max(0.f, _watermark - DeactivateThreshold);
      if (magnitude < _watermark && magnitude <= deactivateThreshold)
      {
         _watermark = magnitude;
         _active = false;
      }
   }
   else
   {
      _watermark = FMath::Min(_watermark, magnitude);

      const float activateThreshold = FMath::Min(1.f, _watermark + ActivateThreshold);
      if (magnitude > _watermark && magnitude >= activateThreshold)
      {
         _watermark = magnitude;
         _active = true;
      }
   }

   return _active ? 1.0f : 0.f;
}

FInputActionValue UOSEEnhancedInputModifierWallClimbLookAssist::GetScaledAssistInputValue(FInputActionValue currentValue, const FVector& viewDirection, const FVector& viewTarget, float assistRadius, float minScale, float maxScale, bool bReverseLerp)
{
   FVector inputVector = currentValue.Get<FVector>();
   float dotProduct = (viewDirection | viewTarget);
   FVector crossProduct = FVector::CrossProduct(viewDirection, viewTarget);
   float angleRadians = FMath::Atan2(crossProduct.Size(), dotProduct);
   float angleDegrees = FMath::RadiansToDegrees(angleRadians);

   // Don't try and scale bools
   if (ensureMsgf(currentValue.GetValueType() != EInputActionValueType::Boolean, TEXT("Sensitivity modifier doesn't support boolean values."))
      && angleDegrees < assistRadius)
   {
      FVector localViewTarget2D;
      localViewTarget2D.X = viewTarget.X;
      localViewTarget2D.Y = viewTarget.Y;
      localViewTarget2D.Z = 0.0f;

      FVector localViewDirection2D;
      localViewDirection2D.X = viewDirection.X;
      localViewDirection2D.Y = viewDirection.Y;
      localViewDirection2D.Z = 0.0f;

      float dotProduct2D = (localViewDirection2D | localViewTarget2D);
      FVector crossProduct2D = FVector::CrossProduct(localViewDirection2D, localViewTarget2D);
      float angleRadians2D = FMath::Atan2(crossProduct2D.Size(), dotProduct2D);
      float angleDegrees2D = FMath::RadiansToDegrees(angleRadians2D);

      if (angleDegrees2D < assistRadius)
      {
         float distanceRatioXY = bReverseLerp ? (1.0f - (angleDegrees2D / assistRadius)) : angleDegrees2D / assistRadius;
         float inputScalarX = FMath::Lerp<float>(minScale, maxScale, distanceRatioXY);
         float inputScaledX = inputVector.X * inputScalarX;

         FVector2D additionalScaleVectorX;
         additionalScaleVectorX.X = inputScaledX - inputVector.X;
         additionalScaleVectorX.Y = 0.0f;

         FInputActionValue additionalScaleX(additionalScaleVectorX);
         currentValue += additionalScaleX;
      }

      float distanceUpDown;
      if (viewDirection.Z < viewTarget.Z)
      {
         distanceUpDown = viewTarget.Z - viewDirection.Z;
      }
      else
      {
         distanceUpDown = viewDirection.Z - viewTarget.Z;
      }

      // Convert normalized distance to degrees
      float distanceDegreesZ = distanceUpDown * 90.0f;
      if (distanceDegreesZ < assistRadius)
      {
         float distanceRatioZ = bReverseLerp ? (1.0f - (distanceDegreesZ / assistRadius)) : distanceDegreesZ / assistRadius;
         float inputScalarY = FMath::Lerp<float>(minScale, maxScale, distanceRatioZ);
         float inputScaledY = inputVector.Y * inputScalarY;

         FVector2D additionalScaleVectorY;
         additionalScaleVectorY.Y = inputScaledY - inputVector.Y;
         additionalScaleVectorY.X = 0.0f;

         FInputActionValue additionalScaleY(additionalScaleVectorY);
         currentValue += additionalScaleY;
      }
   }

   return currentValue;
}

FInputActionValue UOSEEnhancedInputModifierWallClimbLookMovementAssist::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   AOSEPlayerController* playerController = CastChecked<AOSEPlayerController>(playerInput->GetOuterAPlayerController());
   AOSECharacterBase* playerCharacter = IsValid(playerController) ? Cast<AOSECharacterBase>(playerController->GetPawn()) : nullptr;
   UOSECharacterMovement* characterMovement = IsValid(playerCharacter) ? Cast<UOSECharacterMovement>(playerCharacter->GetMovementComponent()) : nullptr;

   if (!IsValid(playerCharacter))
   {
      return currentValue;
   }

   FVector localViewDirection = characterMovement->GetActorTransform().InverseTransformVectorNoScale(playerCharacter->GetViewRotation().Vector());
   FVector localVelocityNormal = characterMovement->GetLocalVelocity().GetSafeNormal();

   const FOSEWallClimbSettings& settings = characterMovement->GetCurrentWallClimbSettings();
   float radiusOfInfluence = settings.CameraLookTowardsMovementRadiusDegrees;

   if (playerCharacter->IsWallClimbing() && characterMovement->GetCurrentAcceleration() != FVector::ZeroVector)
   {
      return GetScaledAssistInputValue(currentValue, localViewDirection, localVelocityNormal, radiusOfInfluence, settings.CameraLookTowardsMovementMinimumScalar, settings.CameraLookTowardsMovementMaximumScalar);
   }

   return currentValue;
}

FInputActionValue UOSEEnhancedInputModifierWallClimbLookWallAssist::ModifyRaw_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue currentValue, float deltaTime)
{
   AOSEPlayerController* playerController = CastChecked<AOSEPlayerController>(playerInput->GetOuterAPlayerController());
   AOSECharacterBase* playerCharacter = IsValid(playerController) ? Cast<AOSECharacterBase>(playerController->GetPawn()) : NULL;
   UOSECharacterMovement* characterMovement = IsValid(playerCharacter) ? Cast<UOSECharacterMovement>(playerCharacter->GetMovementComponent()) : NULL;

   if (!IsValid(playerCharacter))
   {
      return currentValue;
   }

   FVector localViewDirection = characterMovement->GetActorTransform().InverseTransformVectorNoScale(playerCharacter->GetViewRotation().Vector());
   FVector forwardVectorNormal = FVector(1.0f, 0.0f, 0.0f);

   const FOSEWallClimbSettings& settings = characterMovement->GetCurrentWallClimbSettings();
   float radiusOfInfluence = settings.CameraLookTowardsWallRadiusDegrees;

   if (playerCharacter->IsWallClimbing())
   {
      return GetScaledAssistInputValue(currentValue, localViewDirection, forwardVectorNormal, radiusOfInfluence, settings.CameraLookTowardsWallMinimumScalar, settings.CameraLookTowardsWallMaximumScalar, true);
   }

   return currentValue;
}
