// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// UE
#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "Variation/TATMapVariationMgrComponent.h"

#include "TATAreaMarkupVolume.generated.h"

class ATATPowerSource;

/*
 * Private Space Volumes and TATAreaMarkupVolumes share _a lot_ of similar code.
 * I had wanted to pull that into a similar base class OR a component, but that proved to be a bigger job
 * and we're under time presure to deliver TOD, so this'll be backlogged as tech debt.
 */
UCLASS()
class TAT_API ATATAreaMarkupVolume : public AVolume
{
   GENERATED_BODY()
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAreaPowerSourceStatusChanged, bool, isToggledOn, TWeakObjectPtr<ATATPowerSource>, powerSource);

   ATATAreaMarkupVolume();

public:
   const FGameplayTagContainer& GetAreaMarkupTags() const { return _AreaMarkupTags; };
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   FOnAreaPowerSourceStatusChanged OnAreaPowerSourceStatusChanged;

protected:
   UFUNCTION()
   void _HandleMapStateChanged(ETATMapVariationLoadingState currentState);
   void _WaitForWorldBegunPlayOrTrigger();

   UFUNCTION()
   void _HandlePowerStateChanged(bool isToggledOn, TWeakObjectPtr<ATATPowerSource> powerSource);
   void _OnWorldBegunPlay();

   void _HandleInitialOverlaps();
   void _HandleOverlapWithActor(AActor* actor);
   virtual void NotifyActorBeginOverlap(AActor* otherActor) override;
   virtual void NotifyActorEndOverlap(AActor* otherActor) override;

   bool _IsReadyToProcessActorOverlaps { false };

   UPROPERTY(EditAnywhere)
   FGameplayTagContainer _AreaMarkupTags;

   UPROPERTY(EditAnywhere)
   TArray<TWeakObjectPtr<ATATPowerSource>> _PowerSources;
};
