// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "InputTriggers.h"

#include "OSEEnhancedInputTriggers.generated.h"

//---------------------------------------------------------------------------------------
// UOSEInputTriggerDirection
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Direction"))
class UOSEInputTriggerDirection final : public UInputTrigger
{
   GENERATED_BODY()

public:
   UOSEInputTriggerDirection();

   // Which direction do we consider up?  This is the direction we test MaxAngleDistance against
   UPROPERTY(EditAnywhere,  BlueprintReadWrite, Category = "Trigger Settings")
   FVector2D UpDirection;

   // How far to the left/right of this direction are we triggering for?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Settings")
   float MaxAngleDistance = 0.0f;

   // Does this trigger at zero value?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Settings")
   bool TriggersAtZero = false;

protected:
   virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime) override;
   virtual FString GetDebugState() const { return _GetIsTriggeredFromValue(LastValue) ? FString(TEXT("Triggered")) : FString(); }

   bool _GetIsTriggeredFromValue(const FInputActionValue& value) const;
};


//---------------------------------------------------------------------------------------
// UOSEInputTriggerDoublePress
//---------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Double-Press"))
class UOSEInputTriggerDoublePress final : public UInputTrigger
{
   GENERATED_BODY()

public:
   // Maximum amount of seconds between presses to be considered a double-press input
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger Settings", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
   float DoublePressThresholdSeconds = 0.25f;

protected:
   virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* playerInput, FInputActionValue modifiedValue, float deltaTime) override;

private:
   float _timeLastPressed = 0.f;
   FInputActionValue _lastInputActionValue;
};


UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "[OSE] Modify Key Trigger"))
class UOSEModifierKeyTrigger final : public UInputTrigger
{
   GENERATED_BODY()
 
 public:
    UPROPERTY(EditAnywhere, Config, Category = "Trigger Settings")
    FKey ModifierKey = EKeys::LeftControl;
  
 protected:
    virtual ETriggerType GetTriggerType_Implementation() const override { return ETriggerType::Implicit; }
    virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime) override;
 
#if WITH_EDITOR
   virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
 
};
