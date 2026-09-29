// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"

#include "TATOverlapTargetTriggerComponent.generated.h"


enum class ETATOverlapTargetTriggerReason : uint8
{
   Entered,
   StateChange
};

// A component that keeps track of actors that overlap its containing actor that match
// gameplay tag queries.
//
// Not sure how general-purpose this is yet. Initial use-case is electric floors.
//
// Currently only uses Actor-level overlap for simplicity, but could be made to work with
// OSEShapeCollisionTracker if needed.
UCLASS( ClassGroup=(Custom) )
class TAT_API UTATOverlapTargetTriggerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
   DECLARE_MULTICAST_DELEGATE_OneParam(FOnTargetFound, ETATOverlapTargetTriggerReason);

	// Sets default values for this component's properties
   UTATOverlapTargetTriggerComponent();

   void StartTracking();
   void StopTracking();

   FOnTargetFound OnTargetFound;


   DECLARE_MULTICAST_DELEGATE_OneParam(FOnTargetLost, AActor*);
   FOnTargetLost OnTargetLost;

   bool HasValidTargets() const;
   TArrayView<const TWeakObjectPtr<AActor>> GetCurrentTargets() const;
   TArrayView<const TWeakObjectPtr<AActor>> GetCurrentCandidates() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:
   UFUNCTION()
   void _OnActorBeginOverlap(AActor* overlappedActor, AActor* otherActor);
   UFUNCTION()
   void _OnActorEndOverlap(AActor* overlappedActor, AActor* otherActor);
   void _TryAddCandidate(AActor* actor);

   bool _IsValidTarget(AActor* candidate) const;

protected:

   UPROPERTY(EditDefaultsOnly, Category = Filter)
   TSubclassOf<AActor> _candidateRequiredClass;

   UPROPERTY(EditDefaultsOnly, Category = Filter)
   FGameplayTagContainer _candidateRequiredTags;

   UPROPERTY(EditDefaultsOnly, Category = Filter)
   FGameplayTagContainer _candidateBlockedTags;

   UPROPERTY(EditDefaultsOnly, Category = Filter)
   FGameplayTagContainer _targetRequiredTags;

   UPROPERTY(EditDefaultsOnly, Category = Filter)
   FGameplayTagContainer _targetBlockedTags;

   UPROPERTY(EditDefaultsOnly, Category = Filter, meta = (InlineEditConditionToggle))
   bool _filterByStealthScore = false;
   
   // The maximum stealth score that can still be a valid target
   UPROPERTY(EditDefaultsOnly, Category = Filter, meta = (EditCondition="_filterByStealthScore"))
   FScalableFloat _maximumStealthScore;
	
private:
   TArray<TWeakObjectPtr<AActor>> _candidateActors;
   TArray<TWeakObjectPtr<AActor>> _targetActors;
};
