// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Settings/TATUserSettingsCollection.h"

#include "TATUserSettingsCollection_General.generated.h"

enum class EOSEInputSensitivityType : uint8;

UCLASS(MinimalAPI)
class UTATUserSettingsCollection_General : public UTATUserSettingsCollection
{
   GENERATED_BODY()

public:
   UTATUserSettingsCollection_General();
   virtual void LoadSettings() override;

private:
   void GetLanguageOptions(TMap<FString, FText>& options);
   void ApplyLanguage(FString& language);

   void GetSensitivityOptions(TMap<EOSEInputSensitivityType, FText>& options);

   void ApplyMouseSensitivity(float& curveTime);
   void ApplyMouseInvertYAxis(bool& bValue);

   void ApplyGamepadSensitivity(float& curveTime);
   void ApplyGamepadInvertYAxis(bool& bValue);

   void ApplySprintToggle(bool& bValue);
};
