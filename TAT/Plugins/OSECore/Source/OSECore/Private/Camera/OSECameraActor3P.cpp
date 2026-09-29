// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// OSE
#include "Camera/OSECameraActor3P.h"
#include "Camera/OSECameraComponent.h"
#include "Camera/OSECameraUtils.h"
#include "Camera/Modifiers/OSECameraModifier_VelocityFOV.h"

// UE4
#include "Camera/CameraComponent.h"
#include "Camera/CameraModifier.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECameraActor3P)


namespace CameraCVars
{
   namespace TPP
   {
      static float ExtraArmLength = 0;
      FAutoConsoleVariableRef CVarCameraTPPExtraArmLength(
         TEXT("OSE.Camera.3P.ExtraArmLength"),
         ExtraArmLength,
         TEXT("Extra arm length added to the 3P camera"),
         ECVF_Default);

      static float WorldRollMask = 1;
      FAutoConsoleVariableRef CVarCameraTPPWorldRollMask(
         TEXT("OSE.Camera.3P.WorldRollMask"),
         WorldRollMask,
         TEXT("Mask to apply to the world-space roll value"),
         ECVF_Default);

      static float WorldPitchMask = 1;
      FAutoConsoleVariableRef CVarCameraTPPWorldPitchMask(
         TEXT("OSE.Camera.3P.WorldPitchMask"),
         WorldPitchMask,
         TEXT("Mask to apply to the world-space pitch value"),
         ECVF_Default);

      static float WorldYawMask = 1;
      FAutoConsoleVariableRef CVarCameraTPPWorldYawMask(
         TEXT("OSE.Camera.3P.WorldYawMask"),
         WorldYawMask,
         TEXT("Mask to apply to the world-space yaw value"),
         ECVF_Default);

      static float WorldRollDelta = 0;
      FAutoConsoleVariableRef CVarCameraTPPWorldRollDelta(
         TEXT("OSE.Camera.3P.WorldRollDelta"),
         WorldRollDelta,
         TEXT("Delta to apply to the world-space roll value"),
         ECVF_Default);

      static float WorldPitchDelta = 0;
      FAutoConsoleVariableRef CVarCameraTPPWorldPitchDelta(
         TEXT("OSE.Camera.3P.WorldPitchDelta"),
         WorldPitchDelta,
         TEXT("Delta to apply to the world-space pitch value"),
         ECVF_Default);

      static float WorldYawDelta = 0;
      FAutoConsoleVariableRef CVarCameraTPPWorldYawDelta(
         TEXT("OSE.Camera.3P.WorldYawDelta"),
         WorldYawDelta,
         TEXT("Delta to apply to the world-space yaw value"),
         ECVF_Default);
   }
}


AOSECameraActor3P::AOSECameraActor3P(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer.SetDefaultSubobjectClass<UOSECameraComponent>(FCameraComponentName::Component))
   , CameraModifierClass(UOSECameraModifier_VelocityFOV::StaticClass())
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;

   CameraStartXfm = CreateDefaultSubobject<USceneComponent>(FCameraComponentName::StartXfm);
   CameraStartXfm->SetupAttachment(RootComponent);
   CameraStartXfm->SetRelativeRotation(FRotator(-10, 0, 0));
   CameraStartXfm->SetRelativeLocation(FVector(-100, 50, 50));

   CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(FCameraComponentName::SpringArm);
   CameraSpringArm->SetupAttachment(CameraStartXfm);
   CameraSpringArm->SetRelativeRotation(FRotator::ZeroRotator);
   CameraSpringArm->SetRelativeLocation(FVector::ZeroVector);
   CameraSpringArm->CameraLagSpeed = 50.0f;
   CameraSpringArm->CameraRotationLagSpeed = 25.0f;
   CameraSpringArm->bEnableCameraLag = true;
   CameraSpringArm->bEnableCameraRotationLag = true;

   CameraEndXfm = CreateDefaultSubobject<USceneComponent>(FCameraComponentName::EndXfm);
   CameraEndXfm->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
   CameraEndXfm->SetRelativeRotation(FRotator::ZeroRotator);
   CameraEndXfm->SetRelativeLocation(FVector::ZeroVector);

   GetCameraComponent()->FieldOfView = 75.0f;
   GetCameraComponent()->SetupAttachment(CameraEndXfm);
}

AOSECameraActor3P::Settings::Settings(const AOSECameraActor3P& InCamera)
   : RelativeLocation(InCamera.CameraStartXfm->GetRelativeLocation())
   , RelativeRotation(InCamera.CameraStartXfm->GetRelativeRotation())
   , TargetArmLength(InCamera.CameraSpringArm->TargetArmLength)
   , AspectRatio(InCamera.GetCameraComponent()->AspectRatio)
   , FieldOfView(InCamera.GetCameraComponent()->FieldOfView)
{
}

void AOSECameraActor3P::Settings::ApplyTo(AOSECameraActor3P& InOutCamera) const
{
   // Apply optional arm length
   const float finalArmLength = TargetArmLength + (InOutCamera.ExtraCameraArmLength + CameraCVars::TPP::ExtraArmLength);

   InOutCamera.CameraStartXfm->SetRelativeLocation(RelativeLocation);
   InOutCamera.CameraStartXfm->SetRelativeRotation(RelativeRotation);
   InOutCamera.CameraSpringArm->TargetArmLength = finalArmLength;
   InOutCamera.GetCameraComponent()->AspectRatio = AspectRatio;
   InOutCamera.GetCameraComponent()->FieldOfView = FieldOfView;

   FRotator worldRotation = InOutCamera.GetActorRotation();

   // Apply optional mask
   worldRotation.Roll *= InOutCamera.OrientationMask.X * CameraCVars::TPP::WorldRollMask;
   worldRotation.Pitch *= InOutCamera.OrientationMask.Y * CameraCVars::TPP::WorldPitchMask;
   worldRotation.Yaw *= InOutCamera.OrientationMask.Z * CameraCVars::TPP::WorldYawMask;
   worldRotation = worldRotation.GetNormalized();

   // Apply optional delta
   worldRotation.Roll += InOutCamera.OrientationDelta.Roll + CameraCVars::TPP::WorldRollDelta;
   worldRotation.Pitch += InOutCamera.OrientationDelta.Pitch + CameraCVars::TPP::WorldPitchDelta;
   worldRotation.Yaw += InOutCamera.OrientationDelta.Yaw + CameraCVars::TPP::WorldYawDelta;
   worldRotation = worldRotation.GetNormalized();

   InOutCamera.SetActorRotation(worldRotation);
}

AOSECameraActor3P::Settings AOSECameraActor3P::Settings::InterpTo(
   const Settings& Current,
   const Settings& Target,
   const float DeltaTime,
   const float InterpSpeed)
{
   Settings Result = Target;
   Result.RelativeLocation = FMath::VInterpTo(Current.RelativeLocation, Target.RelativeLocation, DeltaTime, InterpSpeed);
   Result.RelativeRotation = FMath::QInterpTo(Current.RelativeRotation, Target.RelativeRotation, DeltaTime, InterpSpeed);
   Result.TargetArmLength = FMath::FInterpTo(Current.TargetArmLength, Target.TargetArmLength, DeltaTime, InterpSpeed);
   Result.AspectRatio = FMath::FInterpTo(Current.AspectRatio, Target.AspectRatio, DeltaTime, InterpSpeed);
   Result.FieldOfView = FMath::FInterpTo(Current.FieldOfView, Target.FieldOfView, DeltaTime, InterpSpeed);
   return Result;
}

// Overridable native event for when play begins for this actor
void AOSECameraActor3P::BeginPlay()
{
   Super::BeginPlay();

   // Save some initial values so we have a baseline for any run-time modifications
   SettingsInitial = Settings(*this);
   SettingsCurrent = SettingsInitial;
   SettingsDesired = SettingsInitial;
}

// Overridable function called whenever this actor is being removed from a level
void AOSECameraActor3P::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   Super::EndPlay(EndPlayReason);
}

// Called when this actor becomes the given PlayerController's ViewTarget. Triggers the Blueprint event K2_OnBecomeViewTarget.
void AOSECameraActor3P::BecomeViewTarget(APlayerController* PC)
{
   Super::BecomeViewTarget(PC);
   SetActorTickEnabled(true);

   // Add the modifier
   CameraModifierObject = UOSECameraModifier::Add(PC, CameraModifierClass, CameraModifierObject.Get());
}

// Called when this actor is no longer the given PlayerController's ViewTarget. Also triggers the Blueprint event K2_OnEndViewTarget.
void AOSECameraActor3P::EndViewTarget(APlayerController* PC)
{
   Super::EndViewTarget(PC);
   SetActorTickEnabled(false);

   // Remove and reset the modifier
   UOSECameraModifier::Remove(PC, CameraModifierObject.Get());
   CameraModifierObject.Reset();
}

// Called to notify that this camera was cut to, so it can update things like interpolation if necessary.
// Typically called by the camera component.
void AOSECameraActor3P::NotifyCameraCut()
{
   Super::NotifyCameraCut();

   SettingsDesired = ComputeSettings();
   SettingsCurrent = SettingsDesired;
   SettingsCurrent.ApplyTo(*this);
}

// Called by default only when this actor is a view target
void AOSECameraActor3P::Tick(float DeltaSeconds)
{
   Super::Tick(DeltaSeconds);

   SettingsDesired = ComputeSettings();
   SettingsCurrent = Settings::InterpTo(SettingsCurrent, SettingsDesired, DeltaSeconds, FramingSpeed);
   SettingsCurrent.ApplyTo(*this);
}

// Computes camera settings to frame the specified actor for third person
AOSECameraActor3P::Settings AOSECameraActor3P::ComputeSettings(const AActor* InActor) const
{
   Settings Result = SettingsInitial;

   if (InActor != nullptr)
   {
      float Radius = 0.0f;
      float HalfHeight = 0.0f;
      InActor->GetSimpleCollisionCylinder(Radius, HalfHeight);

      if ((Radius > KINDA_SMALL_NUMBER) && (HalfHeight > KINDA_SMALL_NUMBER))
      {
         // If this is a character that is crouching, adjust our cylinder half height
         if (auto characterCurrent = Cast<ACharacter>(InActor))
         {
            if (characterCurrent->bIsCrouched)
            {
               auto characterDefault = GetDefault<ACharacter>(characterCurrent->GetClass());
               HalfHeight *= characterDefault->BaseEyeHeight / characterCurrent->CrouchedEyeHeight;
            }
         }

         // Ideal human proportions assumes the height is eight heads tall
         const float HeightPawn = HalfHeight * 2.0f;
         const float HeightHead = HeightPawn / 8.0f;

         // This is the total width, height, and depth given our pawn and FOV
         const float ScreenZ = (HeightPawn * 8.0f) / 5.0f;
         const float ScreenY = (ScreenZ * Result.AspectRatio);
         const float ScreenX = (ScreenY * 0.5f) / FMath::Tan(FMath::DegreesToRadians(Result.FieldOfView * 0.5f));

         // Offset relative to the actor in YZ plane (width and height)
         const float OffsetY = HeightHead * 3.0f;
         const float OffsetZ = HeightHead * 3.0f;

         // Minimum distance to prevent collisions. Less than this we'll need to increase field of view.
         // This takes the offset into account since rotation occurs around the center.
         const float DistanceMin = FVector(0.0f, OffsetY + Radius, OffsetZ + HalfHeight).Size();

         // Offset relative to the actor in X (depth)
         const float OffsetX = DistanceMin;

         // We have enough information to set the location
         {
            Result.RelativeLocation = FVector(-OffsetX, OffsetY, OffsetZ);
         }

         // Maximum distance beyond which we'll need to decrease the field of view
         const float DistanceMax = DistanceMin + Result.TargetArmLength;

         // These are the forward and right distances for the default field of view
         const float DistAdjacent = ScreenX;
         const float DistOpposite = ScreenY * 0.5f;

         // We have enough information to set the rotation
         {
            const float YawDistance = HeightHead;
            const float PitchDistance = HeightHead;
            
            const float YawAngle = FMath::RadiansToDegrees(FMath::Asin(YawDistance / DistAdjacent));
            const float PitchAngle = FMath::RadiansToDegrees(FMath::Asin(PitchDistance / DistAdjacent));
            Result.RelativeRotation = FQuat(FRotator(-PitchAngle, -YawAngle, 0.0f));
         }

         if (DistAdjacent < DistanceMin)
         {
            // We're closer than expected. Zero out the arm length, and adjust the field of view
            Result.FieldOfView = 2.0f * FMath::RadiansToDegrees(FMath::Atan2(DistanceMin, DistOpposite));
            Result.TargetArmLength = 0.0f;
         }
         else if (DistAdjacent > DistanceMax)
         {
            // We're further than expected. Keep the arm length, and adjust the field of view
            Result.FieldOfView = 2.0f * FMath::RadiansToDegrees(FMath::Atan2(DistanceMax, DistOpposite));
         }
         else
         {
            // We're within the expected range. Keep the field of view, and adjust the arm length
            Result.TargetArmLength = DistAdjacent - DistanceMin;
         }
      }
   }

   return Result;
}

AOSECameraActor3P::Settings AOSECameraActor3P::ComputeSettings() const
{
   return ComputeSettings(GetTargetActorForCamera());
}

const AActor* AOSECameraActor3P::GetTargetActorForCamera() const
{
   const AActor* MyOwner = GetOwner();

   if (const APlayerController* MyController = Cast<APlayerController>(MyOwner))
   {
      if (const APawn* ControlledPawn = MyController->GetPawnOrSpectator())
      {
         return ControlledPawn;
      }
   }
   return MyOwner;
}
