// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"

#include "OSEAIPerceptionComponent.generated.h"

class AOSEAIController;

UCLASS(meta=(BlueprintSpawnableComponent))
class OSEAI_API UOSEAIPerceptionComponent : public UAIPerceptionComponent
{
   GENERATED_BODY()

public:

   virtual void BeginPlay() override;

   /// Get the last stimulus for the given actor and sense. Returns false if we don't know anything
   /// about this actor or if senseToUse is None.
   UFUNCTION(BlueprintCallable, Category = "AI|Perception")
   bool GetActorsLastStimulus(AActor* actor, TSubclassOf<UAISense> senseToUse, FAIStimulus& stimulus) const;

   /// Get the last sensed location for the given actor and sense. If senseToUse is none, all senses
   /// will be used. Returns false if we don't know anything about this actor.
   UFUNCTION(BlueprintCallable, Category = "AI|Perception")
   bool GetLastSensedActorLocation(AActor* actor, TSubclassOf<UAISense> senseToUse, FVector& lastSensedLocation) const;

   /// If senseToUse is none, all senses will be used.
   UFUNCTION(BlueprintCallable, Category = "AI|Perception")
   bool IsActorCurrentlyPerceived(AActor* actor, TSubclassOf<UAISense> senseToUse) const;

   UFUNCTION(BlueprintCallable, Category = "AI|Perception")
   void SetAllSensesEnabled(bool enabled);

   UPROPERTY(BlueprintAssignable)
   FActorPerceptionUpdatedDelegate OnTargetSightPerceptionUpdated;

   UPROPERTY(BlueprintAssignable)
   FActorPerceptionUpdatedDelegate OnVisualStimEvent;

   UPROPERTY(BlueprintAssignable)
   FActorPerceptionUpdatedDelegate OnHearingEvent;

   UPROPERTY(BlueprintAssignable)
   FActorPerceptionUpdatedDelegate OnDamageEvent;

   UPROPERTY(BlueprintAssignable)
   FActorPerceptionUpdatedDelegate OnTeamStimEvent;

   UPROPERTY(BlueprintAssignable)
   FActorPerceptionUpdatedDelegate OnTouchStimEvent;

protected:
   UFUNCTION()
   virtual void _OnTargetPerceptionUpdated(AActor* actor, FAIStimulus stimulus);
   UFUNCTION()
   virtual void _OnTargetPerceptionInfoUpdated(const FActorPerceptionUpdateInfo& updateInfo);
};
