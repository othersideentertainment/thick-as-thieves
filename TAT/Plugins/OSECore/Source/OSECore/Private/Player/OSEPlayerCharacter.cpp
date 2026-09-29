// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Player/OSEPlayerCharacter.h"

// OSE
#include "OSECoreCollision.h"
#include "Camera/OSECameraComponent.h"
#include "Camera/OSECameraSettings.h"
#include "Camera/OSECameraUtils.h"
#include "Camera/Modifiers/OSECameraModifier_VelocityFOV.h"
#include "Character/OSECharacterMovement.h"
#include "Player/OSEPlayerState.h"
#include "Player/OSEPlayerController.h"

// UE4
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPlayerCharacter)


// Sets default values, and disables ability system component static construction
AOSEPlayerCharacter::AOSEPlayerCharacter(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
   , CameraModifierClass(UOSECameraModifier_VelocityFOV::StaticClass())
{
   // Default player capsule component settings
   GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
   GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);

   // Base mesh starts at the base of the capsule, rotated 90 degrees
   GetMesh()->SetupAttachment(GetCapsuleComponent());
   GetMesh()->SetRelativeRotation(FRotator(0, 270, 0));
   GetMesh()->SetCollisionObjectType(ECC_Pawn);
   GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
   GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
   GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);

   _cameraParentXfm = CreateOptionalDefaultSubobject<USceneComponent>(FCameraComponentName::ParentXfm);
   if(_cameraParentXfm)
   {
      _cameraParentXfm->SetupAttachment(GetCapsuleComponent());
      _cameraParentXfm->SetRelativeRotation(FRotator::ZeroRotator);
      _cameraParentXfm->SetRelativeLocation(FVector::ZeroVector);
      _cameraComponent = CreateOptionalDefaultSubobject<UOSECameraComponent>(FCameraComponentName::Component);
      if(_cameraComponent)
      {
         _cameraComponent->SetupAttachment(_cameraParentXfm);
         _cameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
         _cameraComponent->SetRelativeLocation(FVector::ZeroVector);
         _cameraComponent->bUsePawnControlRotation = true;
      }
   }

   // Allow player characters to mantle by default
   if (UOSECharacterMovement* movementComp = Cast<UOSECharacterMovement>(GetCharacterMovement()))
   {
      movementComp->SetMantleEnabled(true);
      movementComp->SetLedgeStateEnabled(true);
   }

   bSprintCanceledIfStationary = true;

   ApplyCapsuleParams();
}

void AOSEPlayerCharacter::BeginPlay()
{
   Super::BeginPlay();
   if(_cameraComponent)
   {
      _cameraPreviousZPosition = _cameraComponent->GetComponentLocation().Z;
   }
}

void AOSEPlayerCharacter::Tick(float deltaTime)
{
   Super::Tick(deltaTime);
   if(_cameraComponent == nullptr)
      return;
   // Update local camera component offsets
   {
      FTransform cameraMotionTransform = FTransform::Identity;
      cameraMotionTransform = _ApplyCameraZSmoothing_EyeHeight(cameraMotionTransform, deltaTime);
      cameraMotionTransform = _ApplyCameraZSmoothing_World(cameraMotionTransform, deltaTime);

      _cameraComponent->ClearAdditiveOffset();

      constexpr float fovOffset = 0.0f;
      _cameraComponent->AddAdditiveOffset(cameraMotionTransform, fovOffset);
   }

   _cameraPreviousZPosition = _cameraComponent->GetComponentLocation().Z;
}

// Called when this actor becomes the given PlayerController's ViewTarget. Triggers the Blueprint event K2_OnBecomeViewTarget.
void AOSEPlayerCharacter::BecomeViewTarget(APlayerController* PC)
{
   Super::BecomeViewTarget(PC);

   // Add the modifier
   CameraModifierObject = UOSECameraModifier::Add(PC, CameraModifierClass, CameraModifierObject.Get());
}

// Called when this actor is no longer the given PlayerController's ViewTarget. Also triggers the Blueprint event K2_OnEndViewTarget.
void AOSEPlayerCharacter::EndViewTarget(APlayerController* PC)
{
   Super::EndViewTarget(PC);

   // Remove and reset the modifier
   UOSECameraModifier::Remove(PC, CameraModifierObject.Get());
   CameraModifierObject.Reset();
}

void AOSEPlayerCharacter::RecalculateBaseEyeHeight()
{
   const float prevEyeHeight = BaseEyeHeight;

   Super::RecalculateBaseEyeHeight();

   // The difference between the heights is stored so we can smoothly interpolate
   // the camera locally using additive offsets
   const float currEyeHeight = BaseEyeHeight;
   _cameraEyeHeightOffset = prevEyeHeight - currEyeHeight;

   // Update the camera position
   if(_cameraParentXfm)
   {
      _cameraParentXfm->SetRelativeRotation(FRotator::ZeroRotator);
      _cameraParentXfm->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight));
   }
}

void AOSEPlayerCharacter::HandleSetPlayerState()
{
   AOSEPlayerState* OSEPlayerState = GetPlayerState<AOSEPlayerState>();
   if (OSEPlayerState)
   {
      OwningPlayerState = OSEPlayerState;
   }

   // @TODO good place for other events that need to happen on server and all clients
}

void AOSEPlayerCharacter::HandleSetPlayerController()
{
   AOSEPlayerController* OSEPlayerController = Cast<AOSEPlayerController>(Controller);
   if (OSEPlayerController)
   {
      OwningPlayerController = OSEPlayerController;
   }

   // @TODO good place for other initialization events that you do NOT want to happen on remote clients
}

FTransform AOSEPlayerCharacter::_ApplyCameraZSmoothing_World(const FTransform& input, const float deltaTime)
{
   const UOSECameraSettings& cameraSettings = UOSECameraSettings::Get();
   const float interpSpeed = cameraSettings.GetPlayerParams().WorldZInterpolationSpeed;
   const float maxZOffset = cameraSettings.GetPlayerParams().MaxCameraZOffsetDistance;
   
   if (cameraSettings.GetPlayerParams().ShouldSmoothWorldZPosition)
   {
      const float cameraCurrentZPosition = _cameraComponent->GetComponentLocation().Z;
      _cameraZPositionOffset += _cameraPreviousZPosition - cameraCurrentZPosition;
      _cameraZPositionOffset = FMath::Clamp(_cameraZPositionOffset, -maxZOffset, maxZOffset);
      _cameraZPositionOffset = FMath::FInterpTo(_cameraZPositionOffset, 0.0f, deltaTime, interpSpeed);
   }

   const FQuat rotationOffset = FQuat::Identity;
   const FVector positionOffset = FVector(0.0f, 0.0f, _cameraZPositionOffset);
   return input * FTransform(rotationOffset, positionOffset);
}

FTransform AOSEPlayerCharacter::_ApplyCameraZSmoothing_EyeHeight(const FTransform& input, const float deltaTime)
{
   const UOSECameraSettings& cameraSettings = UOSECameraSettings::Get();
   const float interpSpeed = cameraSettings.GetDampingInterpolationSpeed();

   // Damp eye height offset towards zero
   _cameraEyeHeightOffset = FMath::FInterpTo(_cameraEyeHeightOffset, 0.0f, deltaTime, interpSpeed);

   const FQuat rotationOffset = FQuat::Identity;
   const FVector positionOffset = FVector(0.0f, 0.0f, _cameraEyeHeightOffset);
   return input * FTransform(rotationOffset, positionOffset);
}

UNetConnection* AOSEPlayerCharacter::GetNetConnection() const
{
   // Use the owning controller even if we're not possessed
   if (OwningPlayerController)
   {
      return OwningPlayerController->GetNetConnection();
   }

   return Super::GetNetConnection();
}

void AOSEPlayerCharacter::PossessedBy(class AController* C)
{
   Super::PossessedBy(C);

   // This is server only, and super will set PlayerState and controller
   HandleSetPlayerState();
   HandleSetPlayerController();

   if (!bCreatedStaticAbilityComponents && OwningPlayerState && OwningPlayerState->GetAbilitySystemComponent())
   {
      InitializeAbilities(OwningPlayerState->GetAbilitySystemComponentFromActor(), OwningPlayerState->GetBaseAttributeSet());
   }
}

void AOSEPlayerCharacter::OnRep_PlayerState()
{
   Super::OnRep_PlayerState();

   HandleSetPlayerState();

}

void AOSEPlayerCharacter::OnRep_Controller()
{
   Super::OnRep_Controller();

   // This will happen after OnRep_PlayerState in normal gameplay, but will not happen on remote clients

   HandleSetPlayerController();
}

#if WITH_EDITOR

bool AOSEPlayerCharacter::CanEditChange(const FProperty* inProperty) const
{
   if (inProperty)
   {
      if (!_capsuleParams.UseManualEyeHeights &&
          (inProperty->GetFName() == GET_MEMBER_NAME_CHECKED(APawn, BaseEyeHeight) || 
           inProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ACharacter, CrouchedEyeHeight)))
      {
         return false;
      }
   }
   return Super::CanEditChange(inProperty);
}

void AOSEPlayerCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
   Super::PostEditChangeProperty(PropertyChangedEvent);

   const FName memberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
   if (memberPropertyName == GET_MEMBER_NAME_CHECKED(AOSEPlayerCharacter, _capsuleParams))
   {
      ApplyCapsuleParams();
   }
   if (memberPropertyName == GET_MEMBER_NAME_CHECKED(AOSEPlayerCharacter, BaseEyeHeight))
   {
      _UpdateCameraParentToEyeHeight();
   }
}
#endif

void AOSEPlayerCharacter::ApplyCapsuleParams()
{
   const float capsuleRadius = FMath::Max(0.0f, _capsuleParams.CapsuleRadius);
   const float capsuleHalfHeight = FMath::Max3(0.0f, _capsuleParams.CapsuleHalfHeight, capsuleRadius);
   const float crouchedHalfHeight = FMath::Max3(0.0f, _capsuleParams.CrouchedHalfHeight, capsuleRadius);

   // Initialize the capsule sizes
   {
      GetCapsuleComponent()->InitCapsuleSize(capsuleRadius, capsuleHalfHeight);

      if (UCharacterMovementComponent* movementComp = GetCharacterMovement())
      {
         movementComp->SetCrouchedHalfHeight(crouchedHalfHeight);
      }
   }


   if(!_capsuleParams.UseManualEyeHeights)
   {
      // Default camera height is based on a human being ~8 heads tall.
      // If the eyes are at the halfway point, the eye height is always 
      // half a head height lower than the capsule half height.
      const float halfHeadHeight = capsuleHalfHeight / 8.0f;
      BaseEyeHeight = FMath::Max(0.0f, capsuleHalfHeight - halfHeadHeight);
      CrouchedEyeHeight = FMath::Max(0.0f, crouchedHalfHeight - halfHeadHeight);
   }

   // Update the camera position
   _UpdateCameraParentToEyeHeight();

   // Base mesh starts at the base of the capsule
   GetMesh()->SetRelativeLocation(FVector(0, 0, -capsuleHalfHeight));
}

void AOSEPlayerCharacter::_UpdateCameraParentToEyeHeight()
{
   if (_cameraParentXfm)
   {
      _cameraParentXfm->SetRelativeRotation(FRotator::ZeroRotator);
      _cameraParentXfm->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight));
   }
}

