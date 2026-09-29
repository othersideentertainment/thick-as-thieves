// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "OSECameraActor3P.generated.h"


//--------------------------------------------------------------------------------------------------
/// Custom third-person camera actor. Handles switching view targets and framing the desired target
/// regardless of its size or shape. Also handles collisions via the spring arm.
//--------------------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API AOSECameraActor3P : public ACameraActor
{
   GENERATED_BODY()

public:

   AOSECameraActor3P(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

   /// Overridable native event for when play begins for this actor
   virtual void BeginPlay() override;

   /// Overridable function called whenever this actor is being removed from a level
   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

   /// Called when this actor becomes the given PlayerController's ViewTarget. Triggers the Blueprint event K2_OnBecomeViewTarget.
   virtual void BecomeViewTarget(class APlayerController* PC) override;

   /// Called when this actor is no longer the given PlayerController's ViewTarget. Also triggers the Blueprint event K2_OnEndViewTarget.
   virtual void EndViewTarget(class APlayerController* PC) override;

   /// Called to notify that this camera was cut to, so it can update things like interpolation if necessary.
   /// Typically called by the camera component.
   virtual void NotifyCameraCut() override;

   /// Called by default only when this actor is a view target
   virtual void Tick(float DeltaSeconds) override;

private:

   /// Camera start position (used to attach spring arm)
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
   class USceneComponent* CameraStartXfm;

   /// Camera spring arm
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
   class USpringArmComponent* CameraSpringArm;

   /// Camera end position (used to attach spring arm)
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
   class USceneComponent* CameraEndXfm;

   /// The speed at which the target is framed in third person. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag).
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"))
   float FramingSpeed = 10.0f;

   /// Optional target arm extra length
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, AdvancedDisplay, meta = (AllowPrivateAccess = "true"))
   float ExtraCameraArmLength = 0.0f;

   /// Optional world-space delta orientation to apply
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, AdvancedDisplay, meta = (AllowPrivateAccess = "true"))
   FRotator OrientationDelta = FRotator::ZeroRotator;

   /// Optional mask for world-space orientation values
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, AdvancedDisplay, meta = (AllowPrivateAccess = "true"))
   FVector OrientationMask = FVector::OneVector;

   /// Camera modifier class that is added when this becomes the view target. Added to the camera manager.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, AdvancedDisplay, meta = (AllowPrivateAccess = "true"))
   TSubclassOf<class UOSECameraModifier> CameraModifierClass;

   /// Optional camera modifier object that is created. Used to remove the modifier.
   UPROPERTY(Transient)
   TWeakObjectPtr<class UOSECameraModifier> CameraModifierObject;

protected:
   virtual const AActor* GetTargetActorForCamera() const;
private:

   /// Simple structure to encapsulate the variables
   /// we may be modifying over time
   struct Settings
   {
      Settings() = default;
      Settings(const AOSECameraActor3P& InCamera);

      void ApplyTo(AOSECameraActor3P& InOutCamera) const;

      static Settings InterpTo(const Settings& Current, const Settings& Target, const float DeltaTime, const float InterpSpeed);

      FVector RelativeLocation = FVector::ZeroVector;
      FQuat RelativeRotation = FQuat::Identity;
      float TargetArmLength = 300.0f;
      float AspectRatio = 16.0f / 9.0f;
      float FieldOfView = 90.0f;
   };

   /// Computes camera settings to frame the specified actor for third person
   Settings ComputeSettings(const AActor* InActor) const;
   Settings ComputeSettings() const;

   Settings SettingsInitial;
   Settings SettingsCurrent;
   Settings SettingsDesired;
};
