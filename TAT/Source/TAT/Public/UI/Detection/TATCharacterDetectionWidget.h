// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Alertness/AlertnessEnums.h"
#include "UI/TATUserWidget.h"

#include "TATCharacterDetectionWidget.generated.h"

class IGameplayTagAssetInterface;
class IOSELightDetectionInterface;
class ITraversalInterface;
class UTATDetectionSettingsAsset;
class UToolSetComponent;

UCLASS()
class TAT_API UTATCharacterDetectionWidget : public UTATUserWidget
{
	GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeTick(const FGeometry& myGeometry, float inDeltaTime) override;

   // from UTATUserWidget
   virtual void _OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character) override;

   UFUNCTION(BlueprintImplementableEvent)
   void OnLightDetectionValueChanged(float newValue);

   UFUNCTION(BlueprintImplementableEvent)
   void OnSettingsValueChanged(float newValue);

   UFUNCTION(BlueprintImplementableEvent)
   void OnTotalDetectionValueChanged(float totalValue);

   UFUNCTION(BlueprintCallable) 
   FORCEINLINE bool IsPerformingLightChecks() const { return ShouldUpdateLightValue(); }

   UFUNCTION(BlueprintCallable)
   FORCEINLINE bool IsPerformingDetectionSettingsChecks() const { return ShouldUpdateDetectionSettingsValue(); }

protected:
   UPROPERTY(EditAnywhere)
   UTATDetectionSettingsAsset* DetectionAsset = nullptr;

   UPROPERTY(EditAnywhere)
   EAlertnessLevel DetectionAssetAlertness = EAlertnessLevel::Neutral;

   FORCEINLINE float GetLightDetectionValue() const { return _actualLightDetectionValue; }
   FORCEINLINE float GetDetectionSettingsValue() const { return _detectionSettingsValue * _detectionSettingsWeaponValue; }
   FORCEINLINE float GetTotalDetectionValue() const { return _lightDetectionValuePlusMinimum * GetDetectionSettingsValue(); }

   FORCEINLINE bool ShouldUpdateLightValue() const { return (_lightDetectionInterface != nullptr); }
   FORCEINLINE bool ShouldUpdateDetectionSettingsValue() const { return (DetectionAsset != nullptr) && (_traversalInterface != nullptr) && (_gameplayTagInterface != nullptr); }
   FORCEINLINE bool ShouldUpdateDetectionSettingsWeaponValue() const { return (DetectionAsset != nullptr) && (_toolSetComponent != nullptr); }

private:
   UPROPERTY(Transient)
   TScriptInterface<IOSELightDetectionInterface> _lightDetectionInterface = nullptr;

   UPROPERTY(Transient)
   TScriptInterface<ITraversalInterface> _traversalInterface = nullptr;

   UPROPERTY(Transient)
   TScriptInterface<IGameplayTagAssetInterface> _gameplayTagInterface = nullptr;

   UPROPERTY(Transient)
   UToolSetComponent* _toolSetComponent = nullptr;

   float _actualLightDetectionValue = 1.0f;
   float _lightDetectionValuePlusMinimum = 1.0f;

   float _detectionSettingsValue = 1.0f;
   float _detectionSettingsWeaponValue = 1.0f;

   float _totalDetectionValue = 1.0f;

   UFUNCTION()
   void _OnEquippedToolChanged();

   // Updates cached detection values, calls blueprint notifiers,
   // and returns true if the cached value actually changed.
   bool _TryUpdateLightDetectionValue();
   bool _TryUpdateDetectionSettingsValue();
   bool _TryUpdateWeaponDetectionValue();
   bool _TryUpdateTotalDetectionValue();
	
};
