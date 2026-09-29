// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATFirstPersonViewModifier.generated.h"


struct FTATFirstPersonViewModifierContext
{
   const ACharacter* Character = nullptr;
   bool AllowBouncing = true;
};

// A modifier that can modify both the 1p camera view and 1p mesh
// 
// Sorta like engine UCameraModifiers, but also has hooks for 1p
// mesh positions
//
// Initially used to reproduce the old BP procedural camera animations
// in BP_Player_Base
UCLASS(Abstract, EditInlineNew)
class TAT_API UTATFirstPersonViewModifier : public UObject
{
   GENERATED_BODY()

public:
   virtual void ModifyCamera(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) {}
   // Modifies the view that the 1p mesh is seen relative to
   // So moving it up will make the 1p hands appear lower
   virtual void ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) {}

   // Reset state
   virtual void Reset(const FTATFirstPersonViewModifierContext& context) {}
};


UCLASS()
class TAT_API UTATCameraDipConfig : public UDataAsset
{
   GENERATED_BODY()
   
public:
   UPROPERTY(EditAnywhere, Category=Config)
   bool Enabled = true;
   
   UPROPERTY(EditAnywhere, Category=Curves)
   FRuntimeFloatCurve DipCurve;

   // Base cm to translate down when dip curve is 1, multiplied by strength
   UPROPERTY(EditAnywhere, Category=Values, DisplayName="Up-Down Translation (Z) Scale", meta = (Units="cm"))
   float ZTranslationScale = 20.0f;
   
   // Base degrees to pitch down when dip curve is 1, multiplied by strength
   UPROPERTY(EditAnywhere, Category=Values, DisplayName="Pitch Rotation (Y) Scale", meta = (Units="Degrees"))
   float PitchScale = 3.f;
};

// Plays a dip animation on the camera
//
// NB: This only modifies the camera so could technically be a regular
//     CameraModifier, but the original version suppressed camera bob
//     when playing, which might still be a thing.
UCLASS(DisplayName="Dip (Camera)")
class TAT_API UTATFirstPersonViewModifier_Dip : public UTATFirstPersonViewModifier
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category = Config)
   TObjectPtr<UTATCameraDipConfig> Config;

   void PlayDip(float speed, float strength);
   
   virtual void ModifyCamera(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) override;
   virtual void Reset(const FTATFirstPersonViewModifierContext& context) override;

private:
   float _position = 0.0f;
   float _speed = 0.0f;
   float _strength = 0.0f;
   bool _playing = false;
};

UCLASS()
class TAT_API UTATLocationLagConfig : public UDataAsset
{
   GENERATED_BODY()
   
public:
   UPROPERTY(EditAnywhere, Category=Config)
   bool Enabled = true;

   // General multiplier on magnitude
   UPROPERTY(EditAnywhere, DisplayName="Translation Lag Scale (XYZ)", Category="First Person Mesh View (1P Camera)")
   float LagScale = 2.0f;

   // How fast the lag is smoothed
   UPROPERTY(EditAnywhere, Category="First Person Mesh View (1P Camera)")
   float InterpolateSpeed = 3.f;

   // The max displacement of the mesh view in cm
   UPROPERTY(EditAnywhere, Category = "First Person Mesh View (1P Camera)", meta = (Units="cm"))
   float MaxLagDelta = 4.f;

   // additional Z translation to 1p mesh view based on Left/Right movement
   UPROPERTY(EditAnywhere, DisplayName="Lag (Y) to Up-Down Translation (Z) Scale", Category="Tilt Offset: First Person Mesh View (1P Camera) Values")
   float TiltZMultiplier = 0.5f;

   // additional Roll rotation to 1p mesh view based on Left/Right movement
   UPROPERTY(EditAnywhere, DisplayName="Lag (Y) to Roll Rotation (X) Scale", Category = "Tilt Offset: First Person Mesh View (1P Camera) Values")
   float TiltRollMultiplier = 2.f;

};

// Applies lag to the 1p mesh view based on movement
UCLASS(DisplayName="LocationLag (1p Mesh View)")
class TAT_API UTATFirstPersonViewModifier_LocationLag : public UTATFirstPersonViewModifier
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category = Config)
   TObjectPtr<UTATLocationLagConfig> Config;
   
   virtual void ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) override;
   virtual void Reset(const FTATFirstPersonViewModifierContext& context) override;

private:
   FVector _previousOffset { ForceInit };
};


UCLASS()
class TAT_API UTATPitchOffsetConfig : public UDataAsset
{
   GENERATED_BODY()
   
public:
   UPROPERTY(EditAnywhere, Category=Config)
   bool Enabled = true;

   // X offset of view of 1p mesh over (-90, 90) view pitch
   UPROPERTY(EditAnywhere, Category="View Pitch Local Rotation (Y) Curves", meta = (XAxisName="Pitch", YAxisName="X Offset Alpha (-1, 1)"))
   FRuntimeFloatCurve PitchToFrontBack;
   
   // Z offset of view of 1p mesh over (-90, 90) view pitch
   UPROPERTY(EditAnywhere, Category="View Pitch Local Rotation (Y) Curves", meta = (XAxisName="Pitch", YAxisName="Z Offset Alpha (-1, 1)"))
   FRuntimeFloatCurve PitchToZ;

   // X offset of view of 1p mesh over (-90, 90) view pitch
   UPROPERTY(EditAnywhere, DisplayName="Pitch (Y) to Forward-Back Translation (X) Scale", Category="Pitch Offset: First Person Mesh View (1P Camera) Values", meta = (AllowInvertedInterval))
   float FrontBackOffsetScale = 2;
   
   // Z offset of view of 1p mesh over (-90, 90) view pitch
   UPROPERTY(EditAnywhere, DisplayName="Lag (Y) to Roll Rotation (X) Scale", Category="Pitch Offset: First Person Mesh View (1P Camera) Values", meta = (AllowInvertedInterval))
   float ZOffsetScale = 3;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(class FDataValidationContext& context) const override;
#endif
};

// Translates the 1p mesh view based on the view pitch
UCLASS(DisplayName="PitchOffset (1p Mesh View)")
class TAT_API UTATFirstPersonViewModifier_PitchOffset : public UTATFirstPersonViewModifier
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category = Config)
   TObjectPtr<UTATPitchOffsetConfig> Config;
   
   virtual void ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) override;
};


UCLASS()
class TAT_API UTATRotationLagConfig : public UDataAsset
{
   GENERATED_BODY()
   
public:
   UPROPERTY(EditAnywhere, Category=Config)
   bool Enabled = true;

   // General multiplier
   UPROPERTY(EditAnywhere, Category=Rate)
   float LagScale = 1.0f;

   // How quickly changes in rotation are interpolated/smoothed
   UPROPERTY(EditAnywhere, Category=Rate)
   float InterpolateSpeed = 8.f;
   
   // The maximum change in pitch to rotate by
   UPROPERTY(EditAnywhere, Category=Rate, meta = (Units="degrees"))
   float MaxPitchDelta = 5.f;

   // The maximum change in yaw to rotate by
   UPROPERTY(EditAnywhere, Category = Rate, meta = (Units="degrees"))
   float MaxYawDelta = 4.f;

   // How the pitch rotation rate translates to Z offset on the 1p mesh view
   UPROPERTY(EditAnywhere, Category="Offset", meta = (XAxisName="Pitch Delta", YAxisName="Z Translation"))
   FRuntimeFloatCurve PitchDeltaToZ;

   // How the yaw rotation rate translates to Y offset on the 1p mesh view
   UPROPERTY(EditAnywhere, Category="Offset", meta = (XAxisName="Yaw Delta", YAxisName="Y Translation"))
   FRuntimeFloatCurve YawDeltaToLeftRight;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(class FDataValidationContext& context) const override;
#endif
};

// Lags the 1p mesh based on changes in view rotation
UCLASS(DisplayName="RotationLag (1p Mesh View)")
class TAT_API UTATFirstPersonViewModifier_RotationLag : public UTATFirstPersonViewModifier
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category = Config)
   TObjectPtr<UTATRotationLagConfig> Config;
   
   virtual void ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) override;
   virtual void Reset(const FTATFirstPersonViewModifierContext& context) override;

private:
   FRotator _previousRotation { ForceInit };
   // NB: Not actually a rate, and framerate dependent, but parity first
   FRotator _rotationRate { ForceInit };
};

// Blueprint base class for 1p view modifiers
//
// Initially only adds to transform, rather than in-place modification
UCLASS(Abstract, Blueprintable)
class TAT_API UTATFirstPersonViewModifier_BlueprintBase : public UTATFirstPersonViewModifier
{
   GENERATED_BODY()

public:
   virtual void ModifyCamera(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) override;
   virtual void ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) override;

protected:
   UFUNCTION(BlueprintImplementableEvent)
   void BP_CalculateCameraModifier(const ACharacter* character, float deltaTime, FTransform& transform);
   // Modifies the view that the 1p mesh is seen relative to
   // So moving it up will make the 1p hands appear lower
   UFUNCTION(BlueprintImplementableEvent)
   void BP_CalculateFirstPersonMeshViewModifier(const ACharacter* character, float deltaTime, FTransform& transform);
};
