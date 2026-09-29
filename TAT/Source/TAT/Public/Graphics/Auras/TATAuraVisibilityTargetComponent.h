// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Components/ActorComponent.h"

// tat
#include "Graphics/Auras/TATAuraVisibilityTypes.h"


#include "TATAuraVisibilityTargetComponent.generated.h"

class UTATAuraVisibilityPerceiverComponent;
enum class ETATAuraVisibilityType : uint8;


UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), Config = Game)
class TAT_API UTATAuraVisibilityTargetComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATAuraVisibilityTargetComponent();

   // From UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   void UpdateAuraPerceiverSenseEntry(TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver, FGameplayTag auraSenseTag, bool isBeingPerceived);

   void OnAuraPersistenceTimerElapsed(TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver, FGameplayTag auraSenseTag);

   ETATAuraVisibilityType GetAuraPerceivedVisibilityForLocalPlayer() const;

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAuraVisibilityChanged, ETATAuraVisibilityType, newAuraVisibility);
   UPROPERTY(BlueprintAssignable)
   FOnAuraVisibilityChanged OnAuraVisibilityRefreshed;

private:
   void _OnAuraPerceiverUnregisteredWithWorld(TWeakObjectPtr<UTATAuraVisibilityPerceiverComponent> auraVisibilityPerceiver);
   
   UFUNCTION()
   void _OnRep_SharedAuraVisibility(ETATAuraVisibilityType oldSharedVisibility);

   // Computes the perceived aura visibility for the local aura-perceiving player, notifying listeners of a visibility change
   void _RefreshAuraPerceivedVisibility();

   TArray<FTATAuraVisibilityState>& _GetAuraVisibilityStateArray() { return _auraPerceivedVisibilityStateArrayAuthority.Items; }

   // Stores visibility state for "shared" auras (populated on server only)
   UPROPERTY(Transient)
   FTATAuraVisibilityStateArray _auraPerceivedVisibilityStateArrayAuthority;

   // Stores visibility state for the local player perceiver's local-only auras
   UPROPERTY(Transient)
   FTATAuraVisibilityStateArray _auraPerceivedVisibilityStateArrayLocalOnly;

   // Target's current aura visibility as seen by local perceiving player
   UPROPERTY(Transient)
   ETATAuraVisibilityType _currentAuraVisibility;

   // Resolved "shared" aura visibility (aggregate of _auraPerceivedVisibilityStateArrayShared entries) replicated to clients
   UPROPERTY(Transient, ReplicatedUsing=_OnRep_SharedAuraVisibility)
   ETATAuraVisibilityType _sharedAuraVisibility;
};
