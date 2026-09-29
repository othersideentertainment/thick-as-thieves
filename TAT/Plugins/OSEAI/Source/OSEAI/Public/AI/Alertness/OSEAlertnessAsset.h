// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/AlertnessEnums.h"

// ue4
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

// self
#include "OSEAlertnessAsset.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct OSEAI_API FOSEAlertnessDecaySettings
{
   GENERATED_BODY()

   FOSEAlertnessDecaySettings()
   {
      for(int idx = 0; idx < static_cast<int>(EAlertnessLevel::MAX); ++idx)
      {
         DecayTimeSeconds.FindOrAdd(static_cast<EAlertnessLevel>(idx));
      }
   }

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness")
   TMap<EAlertnessLevel, float> DecayTimeSeconds;

   float GetSecondsToDecayAlertnessLevel(EAlertnessLevel alertnessLevel) const
   {
      if (const float* seconds = DecayTimeSeconds.Find(alertnessLevel))
         return *seconds;
      return 0.0f;
   }
};

UCLASS(Blueprintable)
class OSEAI_API UOSEAlertnessSettingsAsset : public UDataAsset
{
   GENERATED_BODY()

public:

   UPROPERTY(EditAnywhere, Category = "Alertness")
   FOSEAlertnessDecaySettings DecaySettings;

};
