// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "AI/UnifiedStealthSystem/TATUnifiedStealthSettings.h"
#include "Components/ActorComponent.h"

#include "TATStealthScoreComponent.generated.h"

class UGameplayEffect;
class UTATDetectionSettingsAsset;
class UToolSetComponent;
class UCurveFloat;
enum class EAlertnessLevel : uint8;

// A component on Player characters that computes a score the indicates how stealthy the player is
// 1 = Fully Stealthy
// 0 = Not Stealthy
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATStealthScoreComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATStealthScoreComponent();

   // Not BP exposed for now.
   DECLARE_MULTICAST_DELEGATE(FOnStealthScoreChanged);
   DECLARE_MULTICAST_DELEGATE_TwoParams(FOnOwnHearingStimEvent, EStimSeverity, float)
protected:
   virtual void BeginPlay() override;

public:
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
   void HandleOwnStimReaction(const FGameplayTag& stimTag, float loudness);

   float GetStealthDetectionScore() const;

   FOnStealthScoreChanged OnStealthScoreChanged;
   FOnOwnHearingStimEvent OnOwnHearingStimEvent;
private:
   void _HandleStealthScoreChanged() const;
   bool _CalculateStealthScore(const UTATUnifiedStealthSettings& settings, const float deltaTime, float& interpScore) const;
   void _UpdateStealthyEffect();
   
   UFUNCTION(Unreliable, Client)
   void ClientHearingStimTriggered(FGameplayTag stimTag, float loudness);

   UPROPERTY(EditAnywhere, Category = Effect)
   TSubclassOf<UGameplayEffect> _stealthyEffect;

   UPROPERTY(EditAnywhere, Category = Effect)
   float _stealthyEffectThreshold = 0.25f;

   UFUNCTION()
   void OnRep_ReplicatedStealthScore();
   
   // replicated score normalized
   UPROPERTY(Transient, Replicated, ReplicatedUsing=OnRep_ReplicatedStealthScore)
   uint8 _replicatedStealthInterpScore = 0xFF;

   
   float _authorityStealthScore = 1.f;
   
   FActiveGameplayEffectHandle _stealthyEffectHandle;

   UPROPERTY(Transient)
   UToolSetComponent* _toolSetComponent { nullptr };
};
