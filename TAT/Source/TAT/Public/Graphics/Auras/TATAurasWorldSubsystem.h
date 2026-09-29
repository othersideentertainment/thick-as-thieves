// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATAurasWorldSubsystem.generated.h"

class UTATAuraVisibilityPerceiverComponent;
class UTATAuraVisibilityTargetComponent;

DECLARE_STATS_GROUP(TEXT("TAT Auras"), STATGROUP_TAT_Auras, STATCAT_Advanced);
#define AURA_STAT_SCOPE(Name) DECLARE_SCOPE_CYCLE_COUNTER(TEXT(#Name), STAT_Aura_ ## Name , STATGROUP_TAT_Auras)

DECLARE_LOG_CATEGORY_EXTERN(LogTATAuraWorldSubsystem, Log, All);

///
///  Subsystem to manage aura components and their actors.
///
UCLASS()
class TAT_API UTATAurasWorldSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()
public:
   UTATAurasWorldSubsystem();

   static UTATAurasWorldSubsystem* Get(const UWorld* world);

   // FTickableGameObject implementation Begin
   virtual void Tick(float DeltaTime) override;
   virtual bool IsTickableInEditor() const override { return false; }
   virtual TStatId GetStatId() const override;
   // FTickableGameObject implementation End

   void RegisterAuraVisibilityTarget(TWeakObjectPtr<UTATAuraVisibilityTargetComponent> auraComponent);
   void UnregisterAuraVisibilityTarget(TWeakObjectPtr<UTATAuraVisibilityTargetComponent> auraComponent);

   void RegisterAuraVisibilityPerceiver(TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver);
   void UnregisterAuraVisibilityPerceiver(TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver);

   TArray<TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent>>& GetAuraVisibilityPerceivers() { return _auraVisibilityPerceivers; }
   const TArray<TWeakObjectPtr<UTATAuraVisibilityTargetComponent>>& GetAuraVisibilityTargets() { return _auraVisibilityTargets; }

   UFUNCTION(BlueprintCallable, Category = "Auras")
   void GetActiveAuraActors(TArray<AActor*>& activeAuraActors);

public:
   DECLARE_MULTICAST_DELEGATE_OneParam(FOnAuraVisibilityPerceiverRegistered, TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent>);

   FOnAuraVisibilityPerceiverRegistered OnAuraVisibilityPerceiverUnregistered;

private:

   void _cleanUpAuraComponents();
   void _populateActiveAuraActors();

   TArray<TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent>>_auraVisibilityPerceivers;
   
   TArray<TWeakObjectPtr<UTATAuraVisibilityTargetComponent>> _auraVisibilityTargets;

   UPROPERTY()
   TArray<TWeakObjectPtr<AActor>> _activeAuraActors;
};
