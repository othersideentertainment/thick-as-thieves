// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// TAT
#include "Settings/TATUserSettingsCollection.h"

#include "TATUserSettingsCollection_Audio.generated.h"

UCLASS(MinimalAPI)
class UTATUserSettingsCollection_Audio : public UTATUserSettingsCollection
{
   GENERATED_BODY()

public:
   UTATUserSettingsCollection_Audio();

private:
   void SetVolume(float& volume, const TCHAR* rtpcName);
};
