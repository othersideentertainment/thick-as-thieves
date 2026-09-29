// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATFirstPersonViewModifier.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Math/Interval.h"


#include "TATCameraBob.generated.h"


USTRUCT()
struct FTATCameraBobScales
{
   GENERATED_BODY()
   // How much to translate the camera left/right
   UPROPERTY(EditAnywhere, DisplayName="Left-Right Translation (Y) Scale", meta=(Units="cm"))
   float LeftRightTranslationScale = 0;

   // How much to translate the camera up/down
   UPROPERTY(EditAnywhere, DisplayName="Up-Down Translation (Z) Scale", meta=(Units="cm"))
   float UpDownTranslationScale = 3;

   // How much to rotate the camera roll
   UPROPERTY(EditAnywhere, DisplayName="Roll Rotation (X) Scale", meta=(Units="degrees"))
   float RollRotationScale = 0;
};

// Data asset with parameters for procedural camera bob
UCLASS()
class TAT_API UTATCameraBobConfig : public UDataAsset
{
   GENERATED_BODY()
   
public:
   UPROPERTY(EditAnywhere, Category=Config)
   bool Enabled = true;

   /// If the character has any of these tags, disable the camera bob
   UPROPERTY(EditAnywhere, Category=Config)
   FGameplayTagContainer DisabledGameplayTags;

   UPROPERTY(EditAnywhere, Category=Curves, DisplayName="Camera Left-Right Translation (Y) Curve", meta = (YAxisName="Left-Right Translation (Y)"))
   FRuntimeFloatCurve LeftRightCurve;

   UPROPERTY(EditAnywhere, Category=Curves, DisplayName="Camera Up-Down Translation (Z) Curve", meta = (YAxisName="Up-Down Translation (Z)"))
   FRuntimeFloatCurve UpDownCurve;

   UPROPERTY(EditAnywhere, Category=Curves, DisplayName="Camera Roll Rotation (X) Curve", meta = (YAxisName="Roll Rotation (X)"))
   FRuntimeFloatCurve RollCurve;
   
   // How quickly to play the curves
   UPROPERTY(EditAnywhere, Category=Bob)
   float PlayRateMultiplier = 1.43;
   

   // How much faster than the walk speed the player has to be than the max walk speed to use the sprint versions
   UPROPERTY(EditAnywhere, Category=Bob)
   float SprintSpeedThreshold = 1.1f;

   // How quickly changes to the camera offset are interpolated
   UPROPERTY(EditAnywhere, Category=Bob)
   float InterpolationSpeed = 5;

   UPROPERTY(EditAnywhere, Category=Bob)
   FTATCameraBobScales StandardScales = {
      .LeftRightTranslationScale = 0,
      .UpDownTranslationScale = 3,
      .RollRotationScale = 0
   };

   UPROPERTY(EditAnywhere, Category=Bob)
   FTATCameraBobScales SprintScales = {
      .LeftRightTranslationScale = 5,
      .UpDownTranslationScale = 3,
      .RollRotationScale = 0.2f
   };
   
   // Multiplier on camera translation that is applied to the view of the 1p mesh
   UPROPERTY(EditAnywhere, Category="1p Mesh View", DisplayName="Translation (YZ) Scale")
   float FirstPersonMeshViewTranslationScale = -0.15f;


   // Multiplier on camera rotation that is applied to the view of the 1p mesh
   UPROPERTY(EditAnywhere, Category="1p Mesh View", DisplayName="Roll Rotation (X) Scale")
   float FirstPersonMeshViewRotationScale = 0.5f;
};

// Bobs the camera + 1p mesh view based on movement speed
UCLASS(DisplayName="CameraBob (Camera + 1p Mesh View)")
class TAT_API UTATFirstPersonViewModifier_CameraBob : public UTATFirstPersonViewModifier
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, Category=Config)
   TObjectPtr<UTATCameraBobConfig> Config;

   virtual void ModifyCamera(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) override;
   virtual void ModifyFirstPersonMeshView(const FTATFirstPersonViewModifierContext& context, float deltaTime, FTransform& transform) override;

   virtual void Reset(const FTATFirstPersonViewModifierContext& context) override;

private:
   float _alpha = 0;
   float _interpolatedAlpha = 0;
   float _curveTime = 0;
   FVector _translation;
   FRotator _rotation;
};
