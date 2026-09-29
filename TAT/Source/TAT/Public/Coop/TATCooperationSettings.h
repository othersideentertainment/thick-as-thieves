// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "TATCooperationSettings.generated.h"

// CONSIDER: Delete if unused?
UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Cooperation Settings"))
class TAT_API UTATCooperationSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UTATCooperationSettings& Get() { return *GetDefault<UTATCooperationSettings>(); }
   
   UFUNCTION(BlueprintPure, Category = "[TAT] Cooperation Settings")
   static const UTATCooperationSettings* GetSettings() { return GetDefault<UTATCooperationSettings>(); }
};
