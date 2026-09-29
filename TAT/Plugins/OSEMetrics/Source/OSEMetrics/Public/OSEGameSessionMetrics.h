// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

class AGameModeBase;
class AGameStateBase;
class APlayerState;

class FOSEMetricsSystem;

class OSEMETRICS_API FOSEGameSessionMetrics : public TSharedFromThis<FOSEGameSessionMetrics>
{
   TWeakObjectPtr<UGameInstance> _gameInstance;
   TSharedPtr<FOSEMetricsSystem> _metricsSystem;
   
   struct FMapLoadMetrics
   {
      bool IsMapLoading = false;
      FString MapName;
      double LoadStartTime = 0.0;
      double LoadDuration = 0.0;
   };
   TArray<FMapLoadMetrics> _mapLoadMetrics;

   TFunction<void(const AGameModeBase*, const TSharedPtr<FJsonObject>&)> _extendGameModeMetrics;
   TFunction<void(const AGameStateBase*, const TSharedPtr<FJsonObject>&)> _extendGameStateMetrics;
   TFunction<void(const APlayerState*, const TSharedPtr<FJsonObject>&)> _extendPlayerStateMetrics;

public:
   FOSEGameSessionMetrics(UGameInstance* gameInstance, const TSharedPtr<FOSEMetricsSystem>& metricsSystem);
   ~FOSEGameSessionMetrics();

   void InstallApplicationMetrics();
   void InstallGameSessionMetrics();

   void ExtendGameModeMetrics(TFunction<void(const AGameModeBase*, const TSharedPtr<FJsonObject>&)>&& callback)
   {
      _extendGameModeMetrics = MoveTemp(callback);
   }
   void ExtendGameStateMetrics(TFunction<void(const AGameStateBase*, const TSharedPtr<FJsonObject>&)>&& callback)
   {
      _extendGameStateMetrics = MoveTemp(callback);
   }
   void ExtendPlayerStateMetrics(TFunction<void(const APlayerState*, const TSharedPtr<FJsonObject>&)>&& callback)
   {
      _extendPlayerStateMetrics = MoveTemp(callback);
   }

private:
   static UGameInstance* GetGameInstance(const TWeakPtr<FOSEGameSessionMetrics>& weakThis);

};
