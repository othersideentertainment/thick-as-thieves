// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat 
#include "AI/Alertness/TATAlertnessComponent.h"

// ose

// ue4
#include "GameplayTagContainer.h"

#include "TATCharacterAIAlertnessComponent.generated.h"

class ATATCharacterAIBase;
class UGameplayEffect;
class UTATAlertnessSettingsAsset;
struct FOnAttributeChangeData;
struct FStimInfo;

UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class TAT_API UTATCharacterAIAlertnessComponent : public UTATAlertnessComponent
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStimIncreasedAlertnessLevel, FName, stimTag, EStimSeverity, severity);

   UTATCharacterAIAlertnessComponent();

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
   
   UPROPERTY(BlueprintAssignable, Category = "AI|Alertness")
   FOnStimIncreasedAlertnessLevel OnStimIncreasedAlertnessLevel;
   // called from owning stim database
   void OnStimReceived(const FStimInfo& stimInfo);

protected:
   // from UTATAlertnessComponent
   virtual float _AuthorityGetAlertnessDecayMultiplier() const override;
   virtual bool _AuthorityCanDecayAlertnessToNeutral() const override;
   virtual float _AuthorityGetSecondsUntilAlertnessDecay() const override;
   virtual float _AuthorityGetTotalSecondsForAlertnessDecay() const override;
 
   // from UOSEAlertnessComponent
   virtual void _OnAlertnessLevelChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel) override;

protected:
   // asc callbacks
   void _OnIsUnconsciousTagChanged(const FGameplayTag tag, int32 newTagCount);

private:
   void _AuthorityTickPlayerDetection();
   ATATCharacterAIBase& _GetOwningCharacter() const;

protected:
   UPROPERTY(EditDefaultsOnly, Category = "")
   float MaxActorDistanceForSuspiciousOnVisibility = 1000.0f;

   UPROPERTY(EditDefaultsOnly, Category = "Revive")
   bool RaiseAlertnessOnRevive = false;

   UPROPERTY(EditDefaultsOnly, Category = "Revive", meta = (EditCondition = "RaiseAlertnessOnRevive"))
   EAlertnessLevel AlertnessOnRevive = EAlertnessLevel::Neutral;

private:
   FDelegateHandle _unconsciousDelegateHandle;
};
