// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"

#include "TATElectricalDeviceComponent.generated.h"

class ATATPowerSource;

UCLASS(BlueprintType, HideCategories=(Tags,Activation,Cooking,AssetUserData,Navigation), meta = (BlueprintSpawnableComponent))
class TAT_API UTATElectricalDeviceComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   // From UActorComponent
   virtual void BeginPlay() override;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
   bool MigrateProperties(TObjectPtr<ATATPowerSource> powerSource);
#endif
   
   // Is power supplied to the device?
   UFUNCTION(BlueprintPure, Category="TAT|Electricity")
   bool IsPowered() const;

   // Should the device be powered? This will return true if the device is powered, or it does not have power but does not require a power source.
   UFUNCTION(BlueprintPure, Category="TAT|Electricity")
   bool GetEffectivePoweredState() const;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPoweredChanged, bool, isOn);
   UPROPERTY(BlueprintAssignable)
   FOnPoweredChanged OnPoweredChanged;

   // Was already BP exposed
   ATATPowerSource* GetPowerSource() const { return _powerSource; }
   
   UFUNCTION(BlueprintPure, Category = "TAT|Electrical")
   bool HasFinishedBeginPlay() const { return _hasFinishedBeginPlay; }

   UFUNCTION(BlueprintCallable, Category = "TAT|Electrical")
   void TEMP_MigrateFrom(UTATElectricalDeviceComponent* other);

   void SetRequiresPowerSource(bool requiresPowerSource) { _requirePowerSource = requiresPowerSource; }
private:
   UFUNCTION()
   void _OnPowerStateChanged(bool bIsOn);

private:
   UPROPERTY(EditInstanceOnly, Category="Electricity", BlueprintReadOnly, Meta = (AllowPrivateAccess = "true"))
   TObjectPtr<ATATPowerSource> _powerSource = nullptr;

   // whether a power source is required
   UPROPERTY(EditDefaultsOnly)
   bool _requirePowerSource = true;

   bool _hasFinishedBeginPlay { false };
};
