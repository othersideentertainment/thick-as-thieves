// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATEscalationModifier.h"
#include "TATEscalationModifier_DebugColors.generated.h"

UCLASS()
class TAT_API UTATEscalationModifier_DebugColors : public UTATEscalationModifier
{
   GENERATED_BODY()
public:
   virtual void Apply(ATATAIController* aiController) const override;
   virtual void Revert(ATATAIController* aiController) const override;

protected:
   UPROPERTY(EditAnywhere)
   FColor ColorToSetOnApply { FColor::White };
   UPROPERTY(EditAnywhere)
   FColor ColorToSetOnRevert { FColor::White };
   
};
