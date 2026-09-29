// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"

#include "TATAvailableUnlocksComponent.generated.h"


class UTATUnlockableContentDataAsset;
class UTATSaveGame;

// An actor component that keeps of track whether the local player has any
// unlocks that they have met the requirements for purchasing.
//
// For use as a notifier in the thieves den.
//
// If there are more than one of these in a level, may want to move this logic into
// a subsystem or other shared thing so that the work is not repeated. (and this can
// still proxy)
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATAvailableUnlocksComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   // Sets default values for this component's properties
   UTATAvailableUnlocksComponent();

protected:
   // Called when the game starts
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAvailableUnlocksChanged, bool, hasUnlocks);

   // Event fired when the availability of purchasable unlocks changes
   // Not fired if initially false
   UPROPERTY(BlueprintAssignable, Category=Unlocks)
   FOnAvailableUnlocksChanged OnAvailableUnlocksChanged;

   UFUNCTION(BlueprintCallable, meta=(WorldContext="worldContext"))
   static void MarkAvailableUnlocksSeen(const UObject* worldContext, const UTATUnlockableContentDataAsset* unlockData);

private:
   UFUNCTION()
   void _ScheduleRefreshUnlocks();
   void _RefreshUnlocks();

   bool _CalculateHasAvailableUnlocks() const;

   UPROPERTY(EditAnywhere, Category=Unlocks)
   TObjectPtr<const UTATUnlockableContentDataAsset> _unlockableContent = nullptr;
   
   UPROPERTY(EditDefaultsOnly, Category=Unlocks)
   bool _checkMoneyCost = false;

   UPROPERTY(EditDefaultsOnly, Category=Unlocks)
   bool _skipIfFairyOutro = false;

   UPROPERTY(Transient)
   TObjectPtr<UTATSaveGame> _saveGame;

   bool _hasAvailableUnlocks = false;
   FTimerHandle _refreshTimerHandle;
};
