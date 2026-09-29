// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Camera/TATCameraBob.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"

// ue
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCameraBob)


void UTATFirstPersonViewModifier_CameraBob::ModifyCamera(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform)
{
   // NB: These were implemented to match the behavior of the original versions in BP_Player_Base
   if(Config == nullptr || !Config->Enabled)
   {
      return;
   }

   const UCharacterMovementComponent* cmc = context.Character->GetCharacterMovement();
   bool isAllowed = cmc->IsMovingOnGround() && context.AllowBouncing;

   // Check if the character has any gameplay tags that would disable the effect
   if (isAllowed && !Config->DisabledGameplayTags.IsEmpty())
   {
      const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(context.Character);
      if (tagInterface != nullptr && tagInterface->HasAnyMatchingGameplayTags(Config->DisabledGameplayTags))
      {
         isAllowed = false;
      }
   }

   if (isAllowed)
   {
      const FVector floorNormal = cmc->CurrentFloor.HitResult.Normal;
      FVector projectedVelocity = FVector::VectorPlaneProject(context.Character->GetVelocity(), floorNormal);
      const float normalizedSpeed = projectedVelocity.Length() / cmc->MaxWalkSpeed;
      _alpha = normalizedSpeed;
      _curveTime += deltaTime * _alpha * Config->PlayRateMultiplier;
      while(_curveTime > 1)
      {
         _curveTime -= 1;
      }

      const float leftRightUnscaled = Config->LeftRightCurve.GetRichCurveConst()->Eval(_curveTime);
      const float upDownUnscaled = Config->UpDownCurve.GetRichCurveConst()->Eval(_curveTime);
      const float rollUnscaled = Config->RollCurve.GetRichCurveConst()->Eval(_curveTime);

      const bool isSprintingSpeed = _alpha > Config->SprintSpeedThreshold;
      const FTATCameraBobScales scales = isSprintingSpeed ? Config->SprintScales : Config->StandardScales;
      
      _translation.Z = upDownUnscaled * scales.UpDownTranslationScale;
      _translation.Y = leftRightUnscaled * scales.LeftRightTranslationScale;
      _rotation.Roll = rollUnscaled * scales.RollRotationScale;
   }
   else
   {
      _alpha = 0;
   }

   _interpolatedAlpha = FMath::FInterpTo(_interpolatedAlpha, _alpha, deltaTime, Config->InterpolationSpeed);

   if(_interpolatedAlpha > 0)
   {
      const float clampedAlpha = FMath::Clamp(_interpolatedAlpha, 0.0f, 1.0f);
      transform.AddToTranslation(_translation * clampedAlpha);
      transform.ConcatenateRotation((_rotation * clampedAlpha).Quaternion());
   }
}

void UTATFirstPersonViewModifier_CameraBob::ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime,  FTransform& transform)
{
   if(Config == nullptr)
   {
      return;
   }
   
   if(_alpha > 0)
   {
      transform.AddToTranslation(_translation * _alpha * Config->FirstPersonMeshViewTranslationScale);
      transform.ConcatenateRotation((_rotation * _alpha * Config->FirstPersonMeshViewRotationScale).Quaternion());
   }
}

void UTATFirstPersonViewModifier_CameraBob::Reset(const FTATFirstPersonViewModifierContext& context)
{
   _alpha = 0;
   _interpolatedAlpha = 0;
   _curveTime = 0;
   _translation = FVector::ZeroVector;
   _rotation = FRotator::ZeroRotator;
}
