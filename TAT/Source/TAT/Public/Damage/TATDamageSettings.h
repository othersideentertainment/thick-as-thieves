// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"

#include "TATDamageSettings.generated.h"


class UGameplayEffect;

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Damage Settings"))
class TAT_API UTATDamageSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // C++ access
   static const UTATDamageSettings& Get() { return *GetDefault<UTATDamageSettings>(); }

   // Instant damage effect to be applied for each damage type
   UPROPERTY(Config, EditAnywhere, Category = "Damage Effects", meta = (Categories = "DamageType"))
   TMap<FGameplayTag, TSoftClassPtr<UGameplayEffect>> DamageEffectByType;

   void AppendEffectsToPreload(TArray<FSoftObjectPath>& outPathsToLoad) const;
};
