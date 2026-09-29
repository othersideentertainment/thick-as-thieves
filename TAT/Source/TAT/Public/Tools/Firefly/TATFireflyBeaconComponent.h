// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "TATFireflyBeaconComponent.generated.h"

// A component that represent a target that fireflies may target
// Some visualization is turned on when that happens
UCLASS(Blueprintable, ClassGroup=(Firefly), meta=(BlueprintSpawnableComponent))
class TAT_API UTATFireflyBeaconComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   // Sets default values for this component's properties
   UTATFireflyBeaconComponent();
   
   FORCEINLINE bool IsShown() const { return _shown; }
   void SetShown(bool isShown);

   bool IsAllowed() const;

   FORCEINLINE float GetRadius() const { return _radius; }

protected:
   // Called when the game starts
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   UFUNCTION(BlueprintNativeEvent)
   void _OnShownChanged(bool shown);

protected:
   // Whether it is allowed to be a beacon in its current state
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Firefly")
   bool _allowed = true;

   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Firefly")
   FColor _color;

   // Additional range to account for the size of the target
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Firefly", meta=(Units="cm"))
   float _radius = 100;

   // Tags that are required on the owner, for this beacon to be allowed
   // TODO: extract if other misc requirements are added
   UPROPERTY(EditDefaultsOnly, Category="Requirements")
   FGameplayTagContainer _requiredTags;

   // Tags that prevent the beacon from being allowed if present
   // TODO: extract if other misc requirements are added
   UPROPERTY(EditDefaultsOnly, Category="Requirements")
   FGameplayTagContainer _blockedTags;


private:
   bool _shown = false;
};
