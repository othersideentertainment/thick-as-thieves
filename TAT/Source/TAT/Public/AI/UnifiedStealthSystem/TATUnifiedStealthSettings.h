// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/StimInfo.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"

#include "TATUnifiedStealthSettings.generated.h"

USTRUCT()
struct FTATUnifiedStealthTagSetting
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery RequiredQuery;

   UPROPERTY(EditDefaultsOnly)
   float ValueToAdd { 1.f };

   UPROPERTY(EditDefaultsOnly)
   float MultiplierToTotalScore { 1.f };
};

USTRUCT()
struct FTATUnifiedStealthLightValueSetting
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   float MinValueForModifier { 1.f };
   
   UPROPERTY(EditDefaultsOnly)
   float MaxValueForModifier { 1.f };

   UPROPERTY(EditDefaultsOnly)
   float Value { 0.f };
};

USTRUCT()
struct FTATUnifiedStealthOwnStimSetting
{
   GENERATED_BODY()
   UPROPERTY(EditDefaultsOnly)
   float ValueToAdd { 0.5f };
};

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Unified Stealth Settings"))
class TAT_API UTATUnifiedStealthSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UTATUnifiedStealthSettings& Get() { return *GetDefault<UTATUnifiedStealthSettings>(); }
   
   UPROPERTY(EditDefaultsOnly, Config, Category="Config|Stealth Score")
   TArray<FTATUnifiedStealthTagSetting> TagSettings;

   UPROPERTY(EditDefaultsOnly, Config, Category="Config|Stealth Score")
   bool ShouldUseSteppedLighting { false };
   
   UPROPERTY(EditDefaultsOnly, Config, Category="Config|Stealth Score")
   TArray<FTATUnifiedStealthLightValueSetting> LightValueSettings;
   
   UPROPERTY(EditDefaultsOnly, Config, Category="Config|Stealth Score")
   float IncreasingSpeedMultiplier { 1.f };

   UPROPERTY(EditDefaultsOnly, Config, Category="Config|Stealth Score")
   float DecreasingSpeedMultiplier { 1.f };
   
   UPROPERTY(EditDefaultsOnly, Config, Category="Config|Stim Score")
   float DecreasingStimSpeedMultiplier { 1.f };
   
   UPROPERTY(EditDefaultsOnly, Config, Category="Config|Stealth Score|Perception", meta=(ClampMin=0.f, ClampMax=1.f))
   float MaterialShowingThreshold { 0.5 };
   
   UPROPERTY(EditDefaultsOnly, Config, Category="Config|Stealth Score|Perception", meta=(ClampMin=0.f, ClampMax=1.f))
   float FireFlyThreshold { 0.5f };

   UPROPERTY(EditDefaultsOnly, Category="Config|Stealth Score|Equipment", Config)
   float BuffForNothingEquipped { 1.f };
   
   UPROPERTY(EditDefaultsOnly, Category="Config|Stealth Score|Equipment", Config)
   float DeBuffForSomethingEquipped { -1.f };
   
   UPROPERTY(EditDefaultsOnly, Config, Category="Config|Stim Score")
   TMap<EStimSeverity, FTATUnifiedStealthOwnStimSetting> OwnStimSettings;
};
