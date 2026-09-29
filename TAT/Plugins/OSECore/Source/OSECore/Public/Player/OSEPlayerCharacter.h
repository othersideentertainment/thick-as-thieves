// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Character/OSECharacterBase.h"
#include "OSEPlayerCharacter.generated.h"


//--------------------------------------------------------------------------------------------------
/// CAPSULE AND CAMERA NOTES:
/// 
/// Player capsule parameters ensures we have one consistent way to set properties related to
/// capsule size that spans multiple properties across actors and components. These include:
/// 
///  - UCapsuleComponent::CapsuleRadius
///  - UCapsuleComponent::CapsuleHalfHeight
///  - UCharacterMovementComponent::CrouchedHalfHeight
///  - APawn::BaseEyeHeight
///  - ACharacter::CrouchedEyeHeight
/// 
/// Proportions are based on artistic proportions for an ideal figure.
/// See https://en.wikipedia.org/wiki/Body_proportions
/// 
///  - Assume a figure approximately eight (8) head heights tall
///  - Crouching height is approximately 2/3 of standing height.
///  - Eye heights are half a head lower than these respective height values.
//--------------------------------------------------------------------------------------------------

USTRUCT()
struct OSECORE_API FOSEPlayerCapsuleParams
{
   GENERATED_BODY()

   /// Capsule radius (71.12cm = 2'4")
   UPROPERTY(EditDefaultsOnly, meta = (Units = cm))
   float CapsuleRadius = 35.56f; 

   /// Capsule half height (182.88cm = 6'0")
   UPROPERTY(EditDefaultsOnly, meta = (Units = cm))
   float CapsuleHalfHeight = 91.44f;

   /// Capsule half height when crouched (121.92cm = 4'0")
   UPROPERTY(EditDefaultsOnly, meta = (Units = cm))
   float CrouchedHalfHeight = 60.96f; 

   // If true, will not set BaseEyeHeight and CrouchedEyeHeight based on the capsule height
   UPROPERTY(EditDefaultsOnly)
   bool UseManualEyeHeights = false;
};


//--------------------------------------------------------------------------------------------------
/// A character suitable for use as a player
//--------------------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API AOSEPlayerCharacter : public AOSECharacterBase
{
   GENERATED_BODY()

public:

   /// Sets default values for this character's properties
   AOSEPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

   /// Returns CameraComponent subobject
   UFUNCTION(BlueprintGetter)
   FORCEINLINE const class UCameraComponent* GetCameraComponent() const { return _cameraComponent; }
   FORCEINLINE class UCameraComponent* GetCameraComponent() { return _cameraComponent; }

   virtual void BeginPlay() override;

   virtual void Tick(float deltaTime) override;

   /// Called when this actor becomes the given PlayerController's ViewTarget. Triggers the Blueprint event K2_OnBecomeViewTarget.
   virtual void BecomeViewTarget(class APlayerController* PC) override;

   /// Called when this actor is no longer the given PlayerController's ViewTarget. Also triggers the Blueprint event K2_OnEndViewTarget.
   virtual void EndViewTarget(class APlayerController* PC) override;

   /// Overriding to update the camera parent position when the capsule size changes
   virtual void RecalculateBaseEyeHeight() override;

   /// Handle player state set logic, on both client and server
   virtual void HandleSetPlayerState();

   /// Handle player controller set logic, on both client and server. This will happen after possess or replicate
   virtual void HandleSetPlayerController();

protected:

   /// Networking overrides
   virtual class UNetConnection* GetNetConnection() const override;
   virtual void PossessedBy(class AController* C) override;
   virtual void OnRep_PlayerState() override;
   virtual void OnRep_Controller() override;

#if WITH_EDITOR
   virtual bool CanMovementComponentEditCrouchHeight() const override { return false; }
   virtual bool CanEditChange(const FProperty* inProperty) const override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:

   virtual FTransform _ApplyCameraZSmoothing_World(const FTransform& input, const float deltaTime);
   virtual FTransform _ApplyCameraZSmoothing_EyeHeight(const FTransform& input, const float deltaTime);

   /// Player controller that owns us, will be valid even if not currently possessed
   UPROPERTY(BlueprintReadOnly, Category = Player)
   class AOSEPlayerController* OwningPlayerController;

   /// Player state that owns us, will be valid even if not currently possessed
   UPROPERTY(BlueprintReadOnly, Category = Player)
   class AOSEPlayerState* OwningPlayerState;

private:

   /// Camera parent position (used to modify camera origin)
   UPROPERTY(VisibleInstanceOnly, Category = Camera)
   class USceneComponent* _cameraParentXfm;

   /// The actual camera component
   UPROPERTY(VisibleInstanceOnly, Category = Camera, BlueprintGetter = GetCameraComponent)
   class UCameraComponent* _cameraComponent;

   /// Camera modifier class that is added when this becomes the view target. Added to the camera manager.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = CameraModifier, meta = (AllowPrivateAccess = "true"))
   TSubclassOf<class UOSECameraModifier_VelocityFOV> CameraModifierClass;

   /// Optional camera modifier object that is created. Used to remove the modifier.
   UPROPERTY(Transient)
   TWeakObjectPtr<class UOSECameraModifier_VelocityFOV> CameraModifierObject;

   UPROPERTY(EditDefaultsOnly, Category = Camera)
   FOSEPlayerCapsuleParams _capsuleParams;

   void ApplyCapsuleParams();
   void _UpdateCameraParentToEyeHeight();

private:

   // camera offset from Z smoothing the world
   float _cameraZPositionOffset = 0.f;

   // Last ticked Z position.
   float _cameraPreviousZPosition = 0.0f;

   /// Eye height offset used to locally smooth changes in the base eye height
   float _cameraEyeHeightOffset = 0.0f;
};
