// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/TATToolTypes.h"

// ue5
#include "Components/ActorComponent.h"

#include "TATToolWorldActorFillBoundingBoxComponent.generated.h"


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class TAT_API UTATToolWorldActorFillBoundingBoxComponent : public UActorComponent
{
   GENERATED_BODY()
public:
   UTATToolWorldActorFillBoundingBoxComponent();

   // From UObject
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // From UActorComponent
   virtual void PreReplication(IRepChangedPropertyTracker& changedPropertyTracker) override;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityFindAdjustedPlacement(const FTATWorldActorBoxFillExtentConstraints& extentConstraints);

   UFUNCTION(BlueprintCallable)
   void AdjustPlacement();

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAdjustedPlacement, const FTATWorldActorBoxFillAdjustedTransform&, adjustedTransform);
   UPROPERTY(BlueprintAssignable)
   FOnAdjustedPlacement OnAdjustedPlacement;

private:

   UPROPERTY(Transient, Replicated)
   FTATWorldActorBoxFillAdjustedTransform _worldActorAdjustedTransform;

   // used to verify that we are calling AuthorityFindAdjustedPlacement in BeginPlay on authority as required
   bool _authorityHasUpdatedWorldActorAdjustedTransform = false;
   bool _authorityHasReplicatedAtLeastOnce = false;
};



