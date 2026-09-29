// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "TATPlayerPostProc.generated.h"

class UPostProcessComponent;

UCLASS()
class TAT_API ATATPlayerPostProc : public AActor
{
   GENERATED_BODY()
public:
   ATATPlayerPostProc();

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   UFUNCTION(BlueprintImplementableEvent)
   void OnVisibilityChange(bool isVisible);

   UFUNCTION(BlueprintImplementableEvent)
   void OnAbilitiesReady(UAbilitySystemComponent* abilitySystemComponent);

   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   UPostProcessComponent* PostProcComponent;

   UFUNCTION(BlueprintPure)
   bool GetVisibility() const { return _isVisible; }

   void SetVisibility(bool shouldBeVisible);

private:
   bool _isVisible = false;
};

