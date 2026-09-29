// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "GameFramework/Actor.h"

#include "TATElectricalDevice.generated.h"

class ATATPowerSource;

DECLARE_LOG_CATEGORY_EXTERN(LogTATElectricalDevice, Log, All);

UCLASS(Abstract, Blueprintable)
class TAT_API ATATElectricalDevice : public AActor
{
   GENERATED_BODY()

public:
   virtual void BeginPlay() override;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
   bool MigrateProperties();
#endif

   // Is power supplied to the device?
   UFUNCTION(BlueprintPure, Category="TAT|Electricity")
   bool IsPowered() const;

   UFUNCTION(BlueprintImplementableEvent, Category="TAT|Electricity")
   void OnPowerToggled(bool bIsOn);

private:
   UFUNCTION()
   void _OnPowerStateChanged(bool bIsOn);

   UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="TAT|Electricity", Meta=(AllowPrivateAccess="true"))
   TObjectPtr<ATATPowerSource> _powerSource = nullptr;

   // whether a power source is required
   UPROPERTY(EditAnywhere, Category="TAT|Electricity")
   bool _requirePowerSource = true;
};
