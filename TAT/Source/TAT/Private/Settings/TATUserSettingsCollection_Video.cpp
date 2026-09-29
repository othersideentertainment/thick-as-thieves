// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Settings/TATUserSettingsCollection_Video.h"

// UE
#include <Engine/GameViewportClient.h>
#include <Framework/Application/SlateApplication.h>
#include <GameFramework/GameUserSettings.h>
#include <Kismet/KismetSystemLibrary.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUserSettingsCollection_Video)

namespace TAT::Settings
{
   static TSharedPtr<SWindow> GetGameWindow()
   {
      return GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
   }

   static TArray<FMonitorInfo> GetDisplayMonitors()
   {
      FDisplayMetrics displayMetrics;
      if (FSlateApplication::IsInitialized())
      {
         FSlateApplication::Get().GetInitialDisplayMetrics(displayMetrics);
      }
      else
      {
         FDisplayMetrics::RebuildDisplayMetrics(displayMetrics);
      }

      return displayMetrics.MonitorInfo;
   }

   static TOptional<FMonitorInfo> FindPrimaryDisplayMonitor()
   {
      TOptional<FMonitorInfo> result;

      const TArray<FMonitorInfo> displayMonitors = GetDisplayMonitors();
      for (const FMonitorInfo& displayMonitorInfo : displayMonitors)
      {
         if (displayMonitorInfo.bIsPrimary)
         {
            result = displayMonitorInfo;
            break;
         }
      }

      return result;
   }

   static TOptional<FMonitorInfo> FindDisplayMonitor(const FString& displayMonitor, bool bFallbackToPrimary)
   {
      TOptional<FMonitorInfo> result;

      const TArray<FMonitorInfo> displayMonitors = GetDisplayMonitors();
      for (const FMonitorInfo& displayMonitorInfo : displayMonitors)
      {
         if (displayMonitorInfo.ID == displayMonitor)
         {
            result = displayMonitorInfo;
            break;
         }

         if (displayMonitorInfo.bIsPrimary && bFallbackToPrimary && !result)
         {
            // Fallback to first primary monitor in case target is not found
            result = displayMonitorInfo;
         }
      }

      return result;
   }
}

UTATUserSettingsCollection_Video::UTATUserSettingsCollection_Video()
{
   CollectionTag = Tag_Settings_Video;
   FName tableId = GetSettingsStringTableId();

   CreateSetting<FString>(
      Tag_Settings_Video_DisplayMonitor, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Display.DisplayMonitor")),
         .GetOptions = TTATUserSettingOptionsDelegate<FString>::CreateUObject(this, &ThisClass::GetDisplayMonitorOptions),
         .OnApplyValue = TTATUserSettingValueDelegate<FString>::CreateUObject(this, &ThisClass::ApplyDisplayMonitor),
      });

   CreateSetting<EWindowMode::Type>(
      Tag_Settings_Video_WindowMode, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Display.WindowMode")),
         .GetOptions = TTATUserSettingOptionsDelegate<EWindowMode::Type>::CreateUObject(this, &ThisClass::GetWindowModeOptions),
         .OnApplyValue = TTATUserSettingValueDelegate<EWindowMode::Type>::CreateUObject(this, &ThisClass::ApplyWindowMode),
      });

   CreateSetting<FIntPoint>(
      Tag_Settings_Video_Resolution, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Display.Resolution")),
         .GetOptions = TTATUserSettingOptionsDelegate<FIntPoint>::CreateUObject(this, &ThisClass::GetResolutionOptions),
         .OnApplyValue = TTATUserSettingValueDelegate<FIntPoint>::CreateUObject(this, &ThisClass::ApplyResolution),
      });

   CreateSetting<bool>(
      Tag_Settings_Video_VSync, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Display.VSync")),
         .OnApplyValue = TTATUserSettingValueDelegate<bool>::CreateUObject(this, &ThisClass::SetVSync),
      });

   CreateSetting<float>(
      Tag_Settings_Video_FOV, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Camera.FOV")),
      });

   CreateSetting<bool>(
      Tag_Settings_Video_MotionBlur, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Camera.MotionBlur")),
         .OnApplyValue = TTATUserSettingValueDelegate<bool>::CreateUObject(this, &ThisClass::SetMotionBlur),
      });
	  
   CreateSetting<FIntPoint>(
      Tag_Settings_Video_FPSLimit, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.Display.FPSLimit")),
         .GetOptions = TTATUserSettingOptionsDelegate<FIntPoint>::CreateUObject(this, &ThisClass::GetFPSLimitOptions),
         .OnApplyValue = TTATUserSettingValueDelegate<FIntPoint>::CreateUObject(this, &ThisClass::ApplyFPSLimit),
      });
	  
   CreateSetting<int32>(
      Tag_Settings_Video_OverallQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.QualityPreset")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_OverallQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_OverallQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_OverallQuality.GetTag()),
      });

   CreateSetting<float>(
      Tag_Settings_Video_ResolutionQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.ResolutionScale")),
         .OnSetValue = TTATUserSettingValueDelegate<float>::CreateUObject(this, &ThisClass::SetResolutionQuality),
         .OnApplyValue = TTATUserSettingValueDelegate<float>::CreateUObject(this, &ThisClass::ApplyResolutionQuality),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_ViewDistanceQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.ViewDistance")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_ViewDistanceQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_ViewDistanceQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_ViewDistanceQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_AntiAliasingQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.AntiAliasing")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_AntiAliasingQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_AntiAliasingQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_AntiAliasingQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_ShadowQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.Shadows")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_ShadowQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_ShadowQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_ShadowQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_GlobalIlluminationQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.GlobalIllumination")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_GlobalIlluminationQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_GlobalIlluminationQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_GlobalIlluminationQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_ReflectionQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.Reflections")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_ReflectionQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_ReflectionQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_ReflectionQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_PostProcessQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.PostProcessing")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_PostProcessQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_PostProcessQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_PostProcessQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_TextureQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.Textures")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_TextureQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_TextureQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_TextureQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_EffectsQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.Effects")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_EffectsQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_EffectsQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_EffectsQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_FoliageQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.Foliage")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_FoliageQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_FoliageQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_FoliageQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_ShadingQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.Shading")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_ShadingQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_ShadingQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_ShadingQuality.GetTag()),
      });

   CreateSetting<int32>(
      Tag_Settings_Video_LandscapeQuality, {
         .DisplayName = FText::FromStringTable(tableId,TEXT("Settings.GraphicsQuality.Landscape")),
         .GetOptions = TTATUserSettingOptionsDelegate<int32>::CreateUObject(this, &ThisClass::GetGroupQualityOptions, Tag_Settings_Video_LandscapeQuality.GetTag()),
         .OnSetValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::SetGroupQuality, Tag_Settings_Video_LandscapeQuality.GetTag()),
         .OnApplyValue = TTATUserSettingValueDelegate<int32>::CreateUObject(this, &ThisClass::ApplyGroupQuality, Tag_Settings_Video_LandscapeQuality.GetTag()),
      });

   Scalability::OnScalabilitySettingsChanged.AddUObject(this, &ThisClass::RefreshQualityLevels);
   UGameViewportClient::OnViewportCreated().AddUObject(this, &ThisClass::OnGameViewportCreated);
}

void UTATUserSettingsCollection_Video::LoadSettings()
{
   // Some logic needs to be skipped during loading
   TGuardValue scopeGuard(bIsLoadingSettings, true);

   Super::LoadSettings();

   FString displayMonitor;
   if (GetSetting(Tag_Settings_Video_DisplayMonitor, displayMonitor) && displayMonitor.IsEmpty())
   {
      TOptional<FMonitorInfo> primaryDisplayMonitorInfo = TAT::Settings::FindPrimaryDisplayMonitor();
      if (primaryDisplayMonitorInfo)
      {
         SetSetting(Tag_Settings_Video_DisplayMonitor, primaryDisplayMonitorInfo->ID);
      }
   }

   FIntPoint resolution;
   if (GetSetting(Tag_Settings_Video_Resolution, resolution) && (resolution.X <= 0 || resolution.Y <= 0))
   {
      resolution = FIntPoint(GSystemResolution.ResX, GSystemResolution.ResY);
      SetSetting(Tag_Settings_Video_Resolution, resolution);
   }

   UGameUserSettings* settings = UGameUserSettings::GetGameUserSettings();
   RefreshQualityLevels(settings->ScalabilityQuality);
}

void UTATUserSettingsCollection_Video::RefreshQualityLevels(const Scalability::FQualityLevels& qualityLevels)
{
   // #TODO: Should really avoid processing in SetSetting if value is identical
   auto RefreshGroupQuality = [this]<typename T>(FGameplayTag tag, T value)
   {
      T currentValue;
      if (GetSetting(tag, currentValue) && currentValue != value)
      {
         SetSetting(tag, value);
      }
   };

   RefreshGroupQuality(Tag_Settings_Video_OverallQuality, qualityLevels.GetSingleQualityLevel());
   RefreshGroupQuality(Tag_Settings_Video_ResolutionQuality, qualityLevels.ResolutionQuality);
   RefreshGroupQuality(Tag_Settings_Video_ViewDistanceQuality, qualityLevels.ViewDistanceQuality);
   RefreshGroupQuality(Tag_Settings_Video_AntiAliasingQuality, qualityLevels.AntiAliasingQuality);
   RefreshGroupQuality(Tag_Settings_Video_ShadowQuality, qualityLevels.ShadowQuality);
   RefreshGroupQuality(Tag_Settings_Video_GlobalIlluminationQuality, qualityLevels.GlobalIlluminationQuality);
   RefreshGroupQuality(Tag_Settings_Video_ReflectionQuality, qualityLevels.ReflectionQuality);
   RefreshGroupQuality(Tag_Settings_Video_PostProcessQuality, qualityLevels.PostProcessQuality);
   RefreshGroupQuality(Tag_Settings_Video_TextureQuality, qualityLevels.TextureQuality);
   RefreshGroupQuality(Tag_Settings_Video_EffectsQuality, qualityLevels.EffectsQuality);
   RefreshGroupQuality(Tag_Settings_Video_FoliageQuality, qualityLevels.FoliageQuality);
   RefreshGroupQuality(Tag_Settings_Video_ShadingQuality, qualityLevels.ShadingQuality);
   RefreshGroupQuality(Tag_Settings_Video_LandscapeQuality, qualityLevels.LandscapeQuality);
}

void UTATUserSettingsCollection_Video::OnGameViewportCreated()
{
   const TSharedPtr<SWindow> gameWindow = TAT::Settings::GetGameWindow();
   if (gameWindow.IsValid())
   {
      gameWindow->SetOnWindowMoved(FOnWindowMoved::CreateUObject(this, &ThisClass::OnGameWindowMoved));

      FString displayMonitor;
      if (GetSetting(Tag_Settings_Video_DisplayMonitor, displayMonitor))
      {
         ApplyDisplayMonitor(displayMonitor);
      }
   }
}

void UTATUserSettingsCollection_Video::OnGameWindowMoved(const TSharedRef<SWindow>& gameWindow)
{
   const FVector2d windowPos = gameWindow->GetPositionInScreen();
   if (lastWindowMovedPos == windowPos)
   {
      return;
   }

   lastWindowMovedPos = windowPos;

   const TArray<FMonitorInfo> displayMonitors = TAT::Settings::GetDisplayMonitors();
   for (const FMonitorInfo& displayMonitorInfo : displayMonitors)
   {
      const FPlatformRect& rect = displayMonitorInfo.DisplayRect;
      if (windowPos.X >= rect.Left && windowPos.X < rect.Right && windowPos.Y >= rect.Top && windowPos.Y < rect.Bottom)
      {
         if (lastWindowMovedMonitor != displayMonitorInfo.ID)
         {
            lastWindowMovedMonitor = displayMonitorInfo.ID;

            FString currentDisplayMonitor;
            if (GetSetting(Tag_Settings_Video_DisplayMonitor, currentDisplayMonitor) && currentDisplayMonitor != displayMonitorInfo.ID)
            {
               SetSetting(Tag_Settings_Video_DisplayMonitor, displayMonitorInfo.ID);
               ApplySetting(Tag_Settings_Video_DisplayMonitor);
            }
         }

         break;
      }
   }
}

void UTATUserSettingsCollection_Video::GetDisplayMonitorOptions(TMap<FString, FText>& options)
{
   const TArray<FMonitorInfo> displayMonitors = TAT::Settings::GetDisplayMonitors();

   options.Reserve(displayMonitors.Num());
   for (const FMonitorInfo& displayMonitorInfo : displayMonitors)
   {
      options.Add(displayMonitorInfo.ID, FText::FromString(displayMonitorInfo.Name));
   }
}

void UTATUserSettingsCollection_Video::ApplyDisplayMonitor(FString& displayMonitor)
{
   if (GIsEditor)
   {
      return;
   }

   const TSharedPtr<SWindow> gameWindow = TAT::Settings::GetGameWindow();
   if (!gameWindow)
   {
      return;
   }

   const TOptional<FMonitorInfo> targetMonitorInfo = TAT::Settings::FindDisplayMonitor(displayMonitor, true);
   if (!targetMonitorInfo)
   {
      return;
   }

   displayMonitor = targetMonitorInfo->ID;

   const FPlatformRect& monitorRect = targetMonitorInfo->DisplayRect;
   const FPlatformRect& monitorWorkArea = targetMonitorInfo->WorkArea;
   const EWindowMode::Type windowMode = gameWindow->GetWindowMode();

   const FVector2d oldWindowPos = gameWindow->GetPositionInScreen();
   const FVector2d oldWindowSize = gameWindow->GetClientSizeInScreen();

   FVector2d newWindowPos(monitorRect.Left, monitorRect.Top);
   FVector2d newWindowSize = oldWindowSize;

   switch (windowMode)
   {
      case EWindowMode::Fullscreen:
      case EWindowMode::WindowedFullscreen:
      {
         newWindowSize.X = monitorRect.Right - monitorRect.Left;
         newWindowSize.Y = monitorRect.Bottom - monitorRect.Top;

         break;
      }

      case EWindowMode::Windowed:
      {
         const FVector2d monitorWorkAreaSize(monitorWorkArea.Right - monitorWorkArea.Left, monitorWorkArea.Bottom - monitorWorkArea.Top);

         // Ensure the window size fits within the monitor work area
         newWindowSize.X = FMath::Min(newWindowSize.X, monitorWorkAreaSize.X);
         newWindowSize.Y = FMath::Min(newWindowSize.Y, monitorWorkAreaSize.Y);

         if (oldWindowPos.X >= monitorWorkArea.Left && oldWindowPos.X <= monitorWorkArea.Right && oldWindowPos.Y >= monitorWorkArea.Top && oldWindowPos.Y <= monitorWorkArea.Bottom)
         {
            // Display monitor hasn't changed, keep existing position
            newWindowPos = oldWindowPos;
         }
         else
         {
            newWindowPos.X = monitorWorkArea.Left + FMath::Max(0.0f, (monitorWorkAreaSize.X - newWindowSize.X) * 0.5f);
            newWindowPos.Y = monitorWorkArea.Top + FMath::Max(0.0f, (monitorWorkAreaSize.Y - newWindowSize.Y) * 0.5f);
         }

         break;
      }

      default: break;
   }

   const bool bMoveWindow = (newWindowPos != oldWindowPos);
   const bool bResizeWindow = (newWindowSize != oldWindowSize);

   if (bMoveWindow && bResizeWindow)
   {
      gameWindow->ReshapeWindow(newWindowPos, newWindowSize);
   }
   else if (bMoveWindow)
   {
      gameWindow->MoveWindowTo(newWindowPos);
   }
   else if (bResizeWindow)
   {
      gameWindow->Resize(newWindowSize);
   }
}

void UTATUserSettingsCollection_Video::GetWindowModeOptions(TMap<EWindowMode::Type, FText>& options)
{
   FName tableId = GetSettingsStringTableId();
   options.Add(EWindowMode::WindowedFullscreen, FText::FromStringTable(tableId, TEXT("Settings.Display.WindowMode.WindowedFullscreen")));

   // [TVT-9163] Exclusive fullscreen option disabled for now
   //options.Add(EWindowMode::Fullscreen, FText::FromStringTable(tableId,TEXT("Settings.Display.WindowMode.Fullscreen")));

   options.Add(EWindowMode::Windowed, FText::FromStringTable(tableId,TEXT("Settings.Display.WindowMode.Windowed")));
}

void UTATUserSettingsCollection_Video::ApplyWindowMode(EWindowMode::Type& windowMode)
{
   UGameUserSettings* settings = UGameUserSettings::GetGameUserSettings();
   settings->SetFullscreenMode(windowMode);

   if (!bIsLoadingSettings)
   {
      // Skip when loading as UGameUserSettings::ValidateSettings causes reentrance
      settings->ApplyResolutionSettings(false);
   }
}

void UTATUserSettingsCollection_Video::GetResolutionOptions(TMap<FIntPoint, FText>& options)
{
   TArray<FIntPoint> resolutions;
   UKismetSystemLibrary::GetSupportedFullscreenResolutions(resolutions);
   FName tableId = GetSettingsStringTableId();

   options.Reserve(resolutions.Num());
   for (const FIntPoint resolution : resolutions)
   {
      FFormatNamedArguments formatArgs;
      formatArgs.Add(TEXT("Width"), FText::AsNumber(resolution.X, &FNumberFormattingOptions::DefaultNoGrouping()));
      formatArgs.Add(TEXT("Height"), FText::AsNumber(resolution.Y, &FNumberFormattingOptions::DefaultNoGrouping()));

      options.Add(resolution, FText::Format(FText::FromStringTable(tableId,TEXT("Settings.Display.Resolution.Option")), formatArgs));
   }
}

void UTATUserSettingsCollection_Video::ApplyResolution(FIntPoint& resolution)
{
   UGameUserSettings* settings = UGameUserSettings::GetGameUserSettings();

   settings->SetScreenResolution(resolution);

   if (!bIsLoadingSettings)
   {
      // Skip when loading as UGameUserSettings::ValidateSettings causes reentrance
      settings->ApplyResolutionSettings(false);
   }
}

void UTATUserSettingsCollection_Video::SetVSync(bool& bEnabled)
{
   static IConsoleVariable* cvarVSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
   if (cvarVSync)
   {
      cvarVSync->Set(bEnabled, ECVF_SetByGameSetting);
   }
}

void UTATUserSettingsCollection_Video::SetMotionBlur(bool& enabled)
{
   static IConsoleVariable* cvarMotionBlur = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlur.Amount"));
   if (cvarMotionBlur)
   {
      // A value of -1 indicates the default setting (enabled)
      // A value of 0 indicates zero motion blur (disabled)
      // You could also set a positive value for configuring the amount of motion blur to use.
      const float newAmount = enabled ? -1.0f : 0.0f;

      cvarMotionBlur->Set(newAmount, ECVF_SetByGameSetting);
   }
}

void UTATUserSettingsCollection_Video::GetFPSLimitOptions(TMap<FIntPoint, FText>& options)
{
   const TArray<FIntPoint> fpsLimits{ 30, 45, 60, 120, 144, 165, 240 };
   const FName tableId = GetSettingsStringTableId();

   options.Reserve(fpsLimits.Num());
   // 0 == no FPS limit
   options.Add(0, FText::FromStringTable(tableId, TEXT("Settings.FPSLimit.Unlimited")));
   for (const FIntPoint fpsLimit : fpsLimits)
   {
      options.Add(fpsLimit, FText::AsNumber(fpsLimit.X));
   }
 }

void UTATUserSettingsCollection_Video::ApplyFPSLimit(FIntPoint& fpsLimit)
{
   UGameUserSettings* settings = UGameUserSettings::GetGameUserSettings();
   settings->SetFrameRateLimit(fpsLimit.X);
   settings->ApplyNonResolutionSettings();
}

void UTATUserSettingsCollection_Video::GetGroupQualityOptions(TMap<int32, FText>& options, FGameplayTag tag)
{
   const Scalability::FQualityLevels qualityLevelCounts = Scalability::GetQualityLevelCounts();

   int32 maxQualityLevel = 5;

   if (tag == Tag_Settings_Video_OverallQuality)
   {
      const FName tableId = GetSettingsStringTableId();
      options.Add(-1, FText::FromStringTable(tableId, TEXT("Settings.GraphicsQuality.QualityPreset.Custom")));

      maxQualityLevel = FMath::Max(TArray{
         qualityLevelCounts.ViewDistanceQuality,
         qualityLevelCounts.AntiAliasingQuality,
         qualityLevelCounts.ShadowQuality,
         qualityLevelCounts.GlobalIlluminationQuality,
         qualityLevelCounts.ReflectionQuality,
         qualityLevelCounts.PostProcessQuality,
         qualityLevelCounts.TextureQuality,
         qualityLevelCounts.EffectsQuality,
         qualityLevelCounts.FoliageQuality,
         qualityLevelCounts.ShadingQuality,
         qualityLevelCounts.LandscapeQuality,
      });
   }
   else if (tag == Tag_Settings_Video_ViewDistanceQuality)
   {
      maxQualityLevel = qualityLevelCounts.ViewDistanceQuality;
   }
   else if (tag == Tag_Settings_Video_AntiAliasingQuality)
   {
      maxQualityLevel = qualityLevelCounts.AntiAliasingQuality;
   }
   else if (tag == Tag_Settings_Video_ShadowQuality)
   {
      maxQualityLevel = qualityLevelCounts.ShadowQuality;
   }
   else if (tag == Tag_Settings_Video_GlobalIlluminationQuality)
   {
      maxQualityLevel = qualityLevelCounts.GlobalIlluminationQuality;
   }
   else if (tag == Tag_Settings_Video_ReflectionQuality)
   {
      maxQualityLevel = qualityLevelCounts.ReflectionQuality;
   }
   else if (tag == Tag_Settings_Video_PostProcessQuality)
   {
      maxQualityLevel = qualityLevelCounts.PostProcessQuality;
   }
   else if (tag == Tag_Settings_Video_TextureQuality)
   {
      maxQualityLevel = qualityLevelCounts.TextureQuality;
   }
   else if (tag == Tag_Settings_Video_EffectsQuality)
   {
      maxQualityLevel = qualityLevelCounts.EffectsQuality;
   }
   else if (tag == Tag_Settings_Video_FoliageQuality)
   {
      maxQualityLevel = qualityLevelCounts.FoliageQuality;
   }
   else if (tag == Tag_Settings_Video_ShadingQuality)
   {
      maxQualityLevel = qualityLevelCounts.ShadingQuality;
   }
   else if (tag == Tag_Settings_Video_LandscapeQuality)
   {
      maxQualityLevel = qualityLevelCounts.LandscapeQuality;
   }

   for (int32 i = 0; i < maxQualityLevel; ++i)
   {
      options.Add(i, Scalability::GetScalabilityNameFromQualityLevel(i));
   }
}

void UTATUserSettingsCollection_Video::SetGroupQuality(int32& value, FGameplayTag tag)
{
   if (bIsLoadingSettings)
   {
      // Skip when loading as we want to source these from UGameUserSettings
      return;
   }

   if (value < 0)
   {
      // Overall quality can be set to -1 to indicate custom quality but we don't
      // want to actually propagate and apply that as it's not a concrete state
      return;
   }

   UGameUserSettings* settings = UGameUserSettings::GetGameUserSettings();

   if (tag == Tag_Settings_Video_OverallQuality)
   {
      settings->ScalabilityQuality.SetFromSingleQualityLevel(value);
      value = settings->ScalabilityQuality.GetSingleQualityLevel();
   }
   else if (tag == Tag_Settings_Video_ViewDistanceQuality)
   {
      settings->ScalabilityQuality.SetViewDistanceQuality(value);
      value = settings->ScalabilityQuality.ViewDistanceQuality;
   }
   else if (tag == Tag_Settings_Video_AntiAliasingQuality)
   {
      settings->ScalabilityQuality.SetAntiAliasingQuality(value);
      value = settings->ScalabilityQuality.AntiAliasingQuality;
   }
   else if (tag == Tag_Settings_Video_ShadowQuality)
   {
      settings->ScalabilityQuality.SetShadowQuality(value);
      value = settings->ScalabilityQuality.ShadowQuality;
   }
   else if (tag == Tag_Settings_Video_GlobalIlluminationQuality)
   {
      settings->ScalabilityQuality.SetGlobalIlluminationQuality(value);
      value = settings->ScalabilityQuality.GlobalIlluminationQuality;
   }
   else if (tag == Tag_Settings_Video_ReflectionQuality)
   {
      settings->ScalabilityQuality.SetReflectionQuality(value);
      value = settings->ScalabilityQuality.ReflectionQuality;
   }
   else if (tag == Tag_Settings_Video_PostProcessQuality)
   {
      settings->ScalabilityQuality.SetPostProcessQuality(value);
      value = settings->ScalabilityQuality.PostProcessQuality;
   }
   else if (tag == Tag_Settings_Video_TextureQuality)
   {
      settings->ScalabilityQuality.SetTextureQuality(value);
      value = settings->ScalabilityQuality.TextureQuality;
   }
   else if (tag == Tag_Settings_Video_EffectsQuality)
   {
      settings->ScalabilityQuality.SetEffectsQuality(value);
      value = settings->ScalabilityQuality.EffectsQuality;
   }
   else if (tag == Tag_Settings_Video_FoliageQuality)
   {
      settings->ScalabilityQuality.SetFoliageQuality(value);
      value = settings->ScalabilityQuality.FoliageQuality;
   }
   else if (tag == Tag_Settings_Video_ShadingQuality)
   {
      settings->ScalabilityQuality.SetShadingQuality(value);
      value = settings->ScalabilityQuality.ShadingQuality;
   }
   else if (tag == Tag_Settings_Video_LandscapeQuality)
   {
      settings->ScalabilityQuality.SetLandscapeQuality(value);
      value = settings->ScalabilityQuality.LandscapeQuality;
   }

   RefreshQualityLevels(settings->ScalabilityQuality);
}

void UTATUserSettingsCollection_Video::ApplyGroupQuality(int32& value, FGameplayTag tag)
{
   const UGameUserSettings* settings = UGameUserSettings::GetGameUserSettings();
   Scalability::SetQualityLevels(settings->ScalabilityQuality);
}

void UTATUserSettingsCollection_Video::SetResolutionQuality(float& value)
{
   if (bIsLoadingSettings)
   {
      // Skip when loading as we want to source these from UGameUserSettings
      return;
   }

   UGameUserSettings* settings = UGameUserSettings::GetGameUserSettings();

   settings->SetResolutionScaleValueEx(value);
   value = settings->ScalabilityQuality.ResolutionQuality;

   RefreshQualityLevels(settings->ScalabilityQuality);
}

void UTATUserSettingsCollection_Video::ApplyResolutionQuality(float& value)
{
   const UGameUserSettings* settings = UGameUserSettings::GetGameUserSettings();
   Scalability::SetQualityLevels(settings->ScalabilityQuality);
}
