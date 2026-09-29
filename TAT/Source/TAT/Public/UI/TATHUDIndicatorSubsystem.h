// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATHUDIndicatorTypes.h"

// ue
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"

#include "TATHUDIndicatorSubsystem.generated.h"

class ATATHUD;
class UOSEAbilitySystemComponent;
class UTATGameplayEffectUIData_HUDIndicator;
struct FGameplayEffectSpec;
struct FActiveGameplayEffect;

UCLASS()
class TAT_API UTATHUDIndicatorSubsystem : public ULocalPlayerSubsystem
{
   GENERATED_BODY()

public:
   // From USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

   // From ULocalPlayerSubsystem
   virtual void PlayerControllerChanged(APlayerController* newPlayerController) override;

   /// Called from TATPlayerState once the player state has been initialized and save data has been loaded.
   void OnLocalPlayerStateReady(APlayerState* newPlayerState);

   UFUNCTION(BlueprintPure, Category = "HUD Indicator Subsystem")
   int32 NumIndicators() const { return _indicators.Num(); }

   UFUNCTION(BlueprintPure, Category = "HUD Indicator Subsystem")
   bool GetIndicatorByIndex(int32 index, FTATHUDIndicatorInfo& indicatorInfo) const;

   void ForEachIndicator(TFunctionRef<void(int32, const FTATHUDIndicatorInfo&)> callback) const;

   UFUNCTION(BlueprintPure, Category = "HUD Indicator Subsystem", Meta = (WorldContext = "contextObject"))
   static bool GetHUDIndicatorTimer(const UObject* contextObject, const FTATHUDIndicatorState& state, float& elapsedSeconds, float& normalizedValue);

private:
   void _InitSubsystemWithAbilitySystemComponent(UOSEAbilitySystemComponent* newAsc);

   ATATHUD* _GetHUD() const;

   bool _GameplayEffectToIndicatorState(const UTATGameplayEffectUIData_HUDIndicator& indicatorData, const FActiveGameplayEffect& effect, FTATHUDIndicatorState& outState) const;
   bool _GameplayEffectToIndicatorState(FActiveGameplayEffectHandle handle, FTATHUDIndicatorState& outState) const;

   /// Called after a map is loaded
   void _PostLoadMap(UWorld* world);
   void _OnWorldBeginPlay();

   int32 _AddIndicatorFromGameplayEffect(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle handle);
   void _UpdateIndicatorByIndex(int32 index);
   void _RemoveIndicatorByIndex(int32 index, bool removeFromArray);
   void _RemoveAllIndicators();

   int32 _FindExistingIndicatorIndex(FActiveGameplayEffectHandle handle) const;

   void _RegisterForPlayerEvents(UOSEAbilitySystemComponent* asc);
   void _UnregisterFromPlayerEvents(UOSEAbilitySystemComponent* asc);

   void _OnGameplayEffectWithDurationAdded(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle handle);
   void _OnAnyGameplayEffectAdded(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle handle);
   void _OnGameplayEffectRemoved(const FActiveGameplayEffect& activeEffect);

   void _OnGameplayEffectTimeChange(FActiveGameplayEffectHandle handle, float newStartTime, float newDuration);
   void _OnGameplayEffectStackChange(FActiveGameplayEffectHandle handle, int32 newStackCount, int32 previousStackCount);

   UPROPERTY(Transient)
   TArray<FTATHUDIndicatorInfo> _indicators;

   TWeakObjectPtr<UOSEAbilitySystemComponent> _abilitySystemComponent;
};
