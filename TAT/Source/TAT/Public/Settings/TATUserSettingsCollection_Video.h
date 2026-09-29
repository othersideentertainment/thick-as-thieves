// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// TAT
#include "Settings/TATUserSettingsCollection.h"

#include "TATUserSettingsCollection_Video.generated.h"

class SWindow;
struct FMonitorInfo;

UCLASS(MinimalAPI)
class UTATUserSettingsCollection_Video : public UTATUserSettingsCollection
{
   GENERATED_BODY()

public:
   UTATUserSettingsCollection_Video();

private:
   virtual void LoadSettings() override;

   void RefreshQualityLevels(const Scalability::FQualityLevels& qualityLevels);

   void OnGameViewportCreated();
   void OnGameWindowMoved(const TSharedRef<SWindow>& gameWindow);

   void GetDisplayMonitorOptions(TMap<FString, FText>& options);
   void ApplyDisplayMonitor(FString& displayMonitor);

   void GetWindowModeOptions(TMap<EWindowMode::Type, FText>& options);
   void ApplyWindowMode(EWindowMode::Type& windowMode);

   void GetResolutionOptions(TMap<FIntPoint, FText>& options);
   void ApplyResolution(FIntPoint& resolution);

   void SetVSync(bool& bEnabled);

   void SetMotionBlur(bool& enabled);

   void GetFPSLimitOptions(TMap<FIntPoint, FText>& options);
   void ApplyFPSLimit(FIntPoint& fpsLimit);
   
   void GetGroupQualityOptions(TMap<int32, FText>& options, FGameplayTag tag);
   void SetGroupQuality(int32& value, FGameplayTag tag);
   void ApplyGroupQuality(int32& value, FGameplayTag tag);
   void SetResolutionQuality(float& value);
   void ApplyResolutionQuality(float& value);

   FVector2d lastWindowMovedPos;
   FString lastWindowMovedMonitor;

   bool bIsLoadingSettings = false;
};
