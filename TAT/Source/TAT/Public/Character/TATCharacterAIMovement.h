// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Misc/Optional.h"

// tat
#include "Character/TATCharacterMovement.h"

#include "TATCharacterAIMovement.generated.h"

UCLASS()
class TAT_API UTATCharacterAIMovement : public UTATCharacterMovement
{
   GENERATED_BODY()

public:
   void SetOverrideBaseMaxWalkSpeed(float baseMaxSpeed);
   float GetOverrideBaseMaxWalkSpeed() const;
   void ClearOverrideBaseMaxWalkSpeed();

protected:
   // from OSECharacterMovement
   virtual float GetBaseMaxSpeed() const override;

   UPROPERTY(EditAnywhere, Category="TAT|Movement Speed Overrides")
   bool _onlyOverrideSpeedIfMovingOnFloor { true };
private:
   TOptional<float> _baseMaxWalkSpeedOverride;
};
