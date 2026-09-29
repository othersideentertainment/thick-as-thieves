// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/AlertnessEnums.h"
#include "AI/Alertness/DetectionEnums.h"
#include "AI/Alertness/OSEAlertnessAsset.h"

// ue4
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

// self
#include "TATAlertnessAsset.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct TAT_API FTATDetectionValueAlertness
{
   GENERATED_BODY()

public:
   // When a player is in this alertness level, with max detection value
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness")
   EAlertnessLevel CurrentAlertnessLevel = EAlertnessLevel::Neutral;

   // We should transition into this alertness level
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness")
   EAlertnessLevel TargetAlertnessLevel = EAlertnessLevel::Neutral;
};

USTRUCT(BlueprintType)
struct TAT_API FTATDetectionAlertnessInfluenceSettings
{
   GENERATED_BODY()

public:

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness")
   TArray<FTATDetectionValueAlertness> DetectionValueEffects;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness")
   EAlertnessLevel IdentifiedAlertnessLevel = EAlertnessLevel::Combat;
};

USTRUCT(BlueprintType)
struct TAT_API FTATBamboozledSettings
{
   GENERATED_BODY()

public:

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness", meta=(Units="Percent"))
   float IncreaseDecayRatePercentPerAI = 200.0f;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness", meta = (Units = "Percent"))
   float MaxDecayRatePercent = 600.0f;

   float GetBamboozledDecayRateMultiplier(int numBamboozledCharacters) const;
};

UCLASS(Blueprintable)
class TAT_API UTATAlertnessSettingsAsset : public UOSEAlertnessSettingsAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, Category = "Alertness")
   FTATDetectionAlertnessInfluenceSettings DetectionAlertnessInfluences;

   UPROPERTY(EditAnywhere, Category = "Alertness")
   FTATBamboozledSettings BamboozledSettings;
};
