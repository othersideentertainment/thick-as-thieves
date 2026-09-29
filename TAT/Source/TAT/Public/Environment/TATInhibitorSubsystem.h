// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Environment/TATInhibitableInterface.h"

// ue
#include "CoreMinimal.h"

#include "TATInhibitorSubsystem.generated.h"

class ATATInhibitorActor;

UCLASS()
class UTATInhibitorSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   UTATInhibitorSubsystem();

   /// Checks if an inhibitor class can be applied to the target actor
   static bool CanActorBeInhibitedBy(AActor* targetActor, TSubclassOf<ATATInhibitorActor> inhibitorClass);

   /// Checks if an inhibitor class can be applied to the target actor and returns the inhibitable type of that target (if valid)
   UFUNCTION(BlueprintPure, Category = "Inhibitor Subsystem")
   static bool IsActorInhibitable(AActor* targetActor, TSubclassOf<ATATInhibitorActor> inhibitorClass, FGameplayTag& inhibitableType);

   /// Checks if the specified actor currently has an inhibitor attached to it
   UFUNCTION(BlueprintPure, Category = "Inhibitor Subsystem")
   bool IsActorCurrentlyInhibited(const AActor* actor) const;

   /// Spawn an inhibitor actor on the target actor.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inhibitor Subsystem")
   static ATATInhibitorActor* AuthoritySpawnInhibitorActor(AActor* targetActor, APawn* instigatorPawn, TSubclassOf<ATATInhibitorActor> inhibitorClass,
      FLinearColor color = FLinearColor::White);

   /// Spawn an inhibitor actor on the target actor, using custom placement info (rather than the placement info provided by the target actor)
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inhibitor Subsystem")
   static ATATInhibitorActor* AuthoritySpawnInhibitorActorWithPlacementInfo(AActor* targetActor, APawn* instigatorPawn,
      TSubclassOf<ATATInhibitorActor> inhibitorClass, const FTATInhibitorPlacementInfo& placementInfo, FLinearColor color = FLinearColor::White);

   /// Lets this subsystem know that an inhibitor is being applied.
   /// Returns the number of inhibitors now applied to that actor.
   bool NotifyActorApplyInhibitor(AActor* actor, ATATInhibitorActor* inhibitorActor, int32& outNewInhibitorCount);

   /// Lets this subsystem know that an inhibitor is being removed.
   /// Returns the number of inhibitors now applied to that actor.
   bool NotifyActorRemoveInhibitor(AActor* actor, ATATInhibitorActor* inhibitorActor, int32& outNewInhibitorCount);

private:
   /// Mapping of inhibited actors to the number of spawned inhibitors.
   /// We handle this here so that inhibitable actor blueprints don't have to deal with the logic around multiple inhibitors.
   TMap<TWeakObjectPtr<AActor>, int32> _inhibitorRefCounts;
};


