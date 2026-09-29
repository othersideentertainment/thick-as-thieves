// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

// OSE
#include "OSEAnimSettings.generated.h"


//--------------------------------------------------------------------------------------------------
/// OSE animation settings.
//--------------------------------------------------------------------------------------------------

UCLASS(Config = Game, DefaultConfig, Const, Meta = (DisplayName = "[OSE] Animation Settings"))
class OSECORE_API UOSEAnimSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:

   static const UOSEAnimSettings& Get() { return *(GetDefault<UOSEAnimSettings>()); }

   UFUNCTION(BlueprintGetter)
   float GetDampingInterpolationSpeed() const { return _dampingInterpolationSpeed; }

   UFUNCTION(BlueprintGetter)
   bool GetUseMovingAsStateChange() const { return _useMovingAsStateChange; }

   UFUNCTION(BlueprintGetter)
   float GetMovingStateChangeDelay() const { return _movingStateChangeDelay; }

private:

   /// Default damping rate for smoothing various rates of rotation in animation.
   /// Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag).
   UPROPERTY(Config, EditAnywhere, Category = Smoothing, BlueprintGetter = GetDampingInterpolationSpeed, meta = (ClampMin = 0, UIMin = 0, ClampMax = 100, UIMax = 100))
   float _dampingInterpolationSpeed = 0.0f;

   /// When enabled, starting or stopping moving will be considered a new "state" in the animation instance update.
   /// This can make it easier to detect and handle start/stop animations, at the slight risk of indavertenly
   /// losing previous state info.
   UPROPERTY(Config, EditAnywhere, Category = StartAndStop, BlueprintGetter = GetUseMovingAsStateChange)
   bool _useMovingAsStateChange = true;

   /// Specifies a minimum amount of time in a new state before considering starting or stopping moving as a new state.
   /// \see _useMovingAsStateChange
   UPROPERTY(Config, EditAnywhere, Category = StartAndStop, BlueprintGetter = GetMovingStateChangeDelay, meta = (editcondition = "_useMovingAsStateChange", ClampMin = 0, UIMin = 0))
   float _movingStateChangeDelay = 0.15f;
};
