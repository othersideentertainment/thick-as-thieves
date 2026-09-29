// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/TATFirstPersonViewModifier.h"

// ue
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATFirstPersonViewModifier)

void UTATFirstPersonViewModifier_Dip::PlayDip(float speed, float strength)
{
   if(Config == nullptr || !Config->Enabled) return;
   
   _strength = strength;
   _speed = speed;
   _position = 0;
   _playing = true;
}

void UTATFirstPersonViewModifier_Dip::ModifyCamera(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform)
{
   if(!_playing) return;

   if(_position > 1)
   {
      _playing = false;
      return;
   }

   const float dipAlpha = Config->DipCurve.GetRichCurveConst()->Eval(_position) * _strength;
   _position += deltaTime * _speed;

   const float zTranslation = dipAlpha * Config->ZTranslationScale;
   const float pitch = dipAlpha * Config->PitchScale;

   transform.AddToTranslation(FVector(0, 0, zTranslation));
   transform.ConcatenateRotation(FRotator(pitch, 0, 0).Quaternion());
}

void UTATFirstPersonViewModifier_Dip::Reset(const FTATFirstPersonViewModifierContext& context)
{
   _position = 0;
   _playing = false;
}

void UTATFirstPersonViewModifier_LocationLag::ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform)
{
   // NB: These were implemented to match the behavior of the original versions in BP_Player_Base
   if(Config == nullptr || !Config->Enabled)
   {
      return;
   }
   const FVector velocity = context.Character->GetVelocity();

   FVector eyeLocation;
   FRotator eyeRotation;
   context.Character->GetActorEyesViewPoint(eyeLocation, eyeRotation);

   const float forwardDot = velocity.Dot(eyeRotation.RotateVector(FVector::ForwardVector));
   const float rightDot = velocity.Dot(eyeRotation.RotateVector(FVector::RightVector));
   const float upDot = velocity.Dot(eyeRotation.RotateVector(FVector::UpVector));

   const UCharacterMovementComponent* cmc = context.Character->GetCharacterMovement();
   check(cmc);
   const float xyScale = Config->LagScale / FMath::Max(cmc->GetMaxSpeed(), 1);
   const float zScale = Config->LagScale / cmc->JumpZVelocity;

   const FVector newOffset = FVector { forwardDot * xyScale, rightDot * xyScale, upDot * zScale}.GetClampedToMaxSize(Config->MaxLagDelta);

   _previousOffset = FMath::VInterpTo(_previousOffset, newOffset, deltaTime, Config->InterpolateSpeed);
   transform.AddToTranslation(_previousOffset);

   // addition tilt based on left/right
   transform.AddToTranslation(FVector(0, 0, -_previousOffset.Y * Config->TiltZMultiplier));
   transform.ConcatenateRotation(FRotator(0, 0, -_previousOffset.Y * Config->TiltRollMultiplier).Quaternion());
}

void UTATFirstPersonViewModifier_LocationLag::Reset(const FTATFirstPersonViewModifierContext& context)
{
   _previousOffset = FVector::ZeroVector;
}

#if WITH_EDITOR
EDataValidationResult UTATPitchOffsetConfig::IsDataValid(class FDataValidationContext& context) const
{
   // TODO: extract to file scope if duplicated again
   auto checkCurveAtZero = [&context](const FRuntimeFloatCurve& curve, const TCHAR* curveName)
   {
      const float valueAtZero = curve.GetRichCurveConst()->Eval(0);
      if(!FMath::IsNearlyZero(valueAtZero))
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Curve '%s' had a non-zero value at 0 (was %f). This will lead to a non-neutral view at rest"), curveName, valueAtZero)));
      }
   };

   checkCurveAtZero(PitchToZ, GET_MEMBER_NAME_STRING_CHECKED(ThisClass, PitchToZ));
   checkCurveAtZero(PitchToFrontBack, GET_MEMBER_NAME_STRING_CHECKED(ThisClass, PitchToFrontBack));

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

void UTATFirstPersonViewModifier_PitchOffset::ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime,
                                                                        FTransform& transform)
{
   // NB: These were implemented to match the behavior of the original versions in BP_Player_Base
   if(Config == nullptr || !Config->Enabled)
   {
      return;
   }

   FVector eyeLocation;
   FRotator eyeRotation;
   context.Character->GetActorEyesViewPoint(eyeLocation, eyeRotation);


   const float pitch = FRotator::NormalizeAxis(eyeRotation.Pitch);
   const float xOffset = Config->FrontBackOffsetScale * Config->PitchToFrontBack.GetRichCurveConst()->Eval(pitch);
   const float zOffset = Config->ZOffsetScale * Config->PitchToZ.GetRichCurveConst()->Eval(pitch);

   transform.AddToTranslation(FVector(xOffset, 0 , zOffset));
}

#if WITH_EDITOR
EDataValidationResult UTATRotationLagConfig::IsDataValid(class FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   auto checkCurveAtZero = [&context](const FRuntimeFloatCurve& curve, const TCHAR* curveName)
   {
      const float valueAtZero = curve.GetRichCurveConst()->Eval(0);
      if(!FMath::IsNearlyZero(valueAtZero))
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Curve '%s' had a non-zero value at 0 (was %f). This will lead to a non-neutral view at rest"), curveName, valueAtZero)));
      }
   };

   checkCurveAtZero(PitchDeltaToZ, GET_MEMBER_NAME_STRING_CHECKED(ThisClass, PitchDeltaToZ));
   checkCurveAtZero(YawDeltaToLeftRight, GET_MEMBER_NAME_STRING_CHECKED(ThisClass, YawDeltaToLeftRight));

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

void UTATFirstPersonViewModifier_RotationLag::ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform)
{
   // NB: These were implemented to match the behavior of the original versions in BP_Player_Base
   if(Config == nullptr || !Config->Enabled)
   {
      return;
   }

   FVector eyeLocation;
   FRotator eyeRotation;
   context.Character->GetActorEyesViewPoint(eyeLocation, eyeRotation);

   FRotator rotationDelta = eyeRotation - _previousRotation;
   rotationDelta.Normalize();
   rotationDelta *= Config->LagScale / (deltaTime * 60); // Use previous frame dependent time as if with a 60hz standard

   _previousRotation = eyeRotation;

   auto clampMagnitude = [](const float value, float magnitude){
      return FMath::Clamp(value, -magnitude, magnitude);
   };

   FRotator scaledDelta { ForceInit };
   scaledDelta.Pitch = clampMagnitude(rotationDelta.Pitch, Config->MaxPitchDelta);
   scaledDelta.Yaw = clampMagnitude(rotationDelta.Yaw, Config->MaxYawDelta);
   _rotationRate = FMath::RInterpTo(_rotationRate, scaledDelta, deltaTime, Config->InterpolateSpeed);

   // To allow lerping inverted intervals
   auto lerpInterval = [](const FFloatInterval& range, float alpha) {
      return FMath::Lerp(range.Min, range.Max, alpha);
   };
   const float zOffset = Config->PitchDeltaToZ.GetRichCurveConst()->Eval(_rotationRate.Pitch);
   const float yOffset = Config->YawDeltaToLeftRight.GetRichCurveConst()->Eval(_rotationRate.Yaw);

   transform.ConcatenateRotation(_rotationRate.Quaternion());
   transform.AddToTranslation(FVector(0, yOffset, zOffset));
}

void UTATFirstPersonViewModifier_RotationLag::Reset(const FTATFirstPersonViewModifierContext& context)
{
   FVector eyeLocation;
   FRotator eyeRotation;
   context.Character->GetActorEyesViewPoint(eyeLocation, eyeRotation);

   _previousRotation = eyeRotation;
   _rotationRate = FRotator::ZeroRotator;
}

void UTATFirstPersonViewModifier_BlueprintBase::ModifyCamera(const FTATFirstPersonViewModifierContext& context, float deltaTime,
   FTransform& transform)
{
   FTransform delta;
   BP_CalculateCameraModifier(context.Character, deltaTime, delta);
   transform.Accumulate(delta);
}

void UTATFirstPersonViewModifier_BlueprintBase::ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime,
   FTransform& transform)
{
   FTransform delta;
   BP_CalculateFirstPersonMeshViewModifier(context.Character, deltaTime, delta);
   transform.Accumulate(delta);
}
