// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Engine/DeveloperSettings.h"

#include "OSEAIEditorSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "[OSE] AI Per User Editor Settings"))
class OSECORE_API UOSEAIEditorSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UOSEAIEditorSettings& GetOSEAIEditorSettings() { return *GetDefault<UOSEAIEditorSettings>(); }
};
