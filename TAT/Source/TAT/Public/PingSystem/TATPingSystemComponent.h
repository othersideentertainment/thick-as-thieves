// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "PingSystem/OSEPingSystemComponent.h"
#include "TATPingSystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATPingSystemComponent : public UOSEPingSystemComponent
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   void GatherPingableTargets();
   
   UFUNCTION(BlueprintCallable)
   FGameplayTagContainer TagsFromGatheredTargets() const;


   UFUNCTION(BlueprintCallable)
   void TriggerDefaultPingOnGatheredTargets();
   bool _AttemptTriggeringPingOnFocusedActor(FGameplayTag pingTagToTrigger);

   UFUNCTION(BlueprintCallable)
   void TriggerPingOnGatheredTargets(FGameplayTag pingTagToTrigger);

protected:
   void _AttemptSonarPingFromHitActor(const AActor* sourceActor, AActor* hitResultActor, TArray<FHitResult>& outTraces) const;
   bool _HandleHitResultsForPing(const AActor* sourceActor, const TArray<FHitResult>& hitResults, TArray<FHitResult>& finalizedHitResults) const;
   void _DrawDebugSweepTrace(const TArray<FHitResult>& hitResults, const FVector& traceStartLocation, const FVector& traceEndLocation) const;

   bool _ShouldUseLocallyFocusedPing() const;

   // NOTE: Disabled for now until we're happy with a UI flow
   bool _ShouldTriggerResponses() const { return false; };

   bool _SweepTraceForPing(const AActor* sourceActor,
                           const FCollisionQueryParams& params,
                           const UWorld* world,
                           const FCollisionShape traceShape,
                           const FVector& traceStartLocation,
                           const FVector& traceEndLocation,
                           TArray<FHitResult>& finalizedHitResults) const;
   
   bool _LineTraceForPing(const AActor* sourceActor,
                          const FCollisionQueryParams& params,
                          const UWorld* world,
                          const FVector& traceStartLocation,
                          const FVector& traceEndLocation,
                          FHitResult& blockingHitResult,
                          TArray<FHitResult>& finalizedHitResults) const;

   UPROPERTY(EditDefaultsOnly, Category="TAT|Gather")
   int _horizontalTraces { 3 };
   UPROPERTY(EditDefaultsOnly, Category="TAT|Gather")
   int _verticalTraces { 3 };

   UPROPERTY(EditDefaultsOnly, Category="TAT|Gather")
   int _numberOfTraceRings { 3 };
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Gather")
   float _traceWidth { 30.f };
   UPROPERTY(EditDefaultsOnly, Category="TAT|Gather")
   float _sonarPingSize { 100.f };

   UPROPERTY(EditDefaultsOnly, Category="TAT|Cooldown")
   float _cooldownDuration { 1.f };
   
   float _cooldownExpiringTime { -1.f };
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Gather")
   FCollisionProfileName _traceProfileForGather { FName(TEXT("Ping")) };

   UPROPERTY(EditDefaultsOnly, Category="TAT|Gather")
   float _maxGatherTraceRange { 999999.9 };

   UPROPERTY(EditDefaultsOnly, Category="TAT|Gather")
   FGameplayTagContainer _fallbackPingsForGather;
   
   TArray<FGatheredPingTarget> _gatheredPingableTargets;
   FHitResult  _gatheredBlockHitResult; 
};
