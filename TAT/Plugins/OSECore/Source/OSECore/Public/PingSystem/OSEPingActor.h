// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "PingSystem/OSEPingSystemInfo.h"

// ue4
#include "GameFramework/Actor.h"

#include "OSEPingActor.generated.h"

class AOSEPlayerState;
class UOSEPingSystemComponent;

UCLASS(Blueprintable, BlueprintType)
class OSECORE_API AOSEPingActor : public AActor
{
   GENERATED_BODY()

public:
   AOSEPingActor();

   // from AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void Tick(float deltaTime) override;

   void AuthoritySetup(UOSEPingSystemComponent* pingSystemOwner, const FGameplayTag& pingTag, AActor* pingableTargetActor, const FHitResult& hitResult, const FTransform& spawnerViewXfm, const FTransform& spawnerWorldXfm);

   UFUNCTION(BlueprintPure, Category = "Ping System")
   const FGameplayTag& GetPingTag() const { return _pingTag; }

   UFUNCTION(BlueprintPure, Category = "Ping System")
   USceneComponent* GetPingUILocation() const { return PingUILocation; }

   UFUNCTION(BlueprintPure, Category = "Ping System")
   AOSEPlayerState* GetPingedBy() const { return _pingedBy; }

   UFUNCTION(BlueprintPure, Category = "Ping System")
   const FHitResult& GetHitResult() const { return _hitResult; }

   UFUNCTION(BlueprintPure, Category = "Ping System")
   const FTransform& GetSpawnerViewXfm() const { return _spawnerViewXfm; }

   UFUNCTION(BlueprintPure, Category = "Ping System")
   const FTransform& GetSpawnerWorldXfm() const { return _spawnerWorldXfm; }

   UFUNCTION(BlueprintNativeEvent, Category = "Ping System")
   bool IsLocallyReadyToBeFocused() const;

protected:
   UFUNCTION()
   void _OnRep_PingTag();
   void _BroadcastPingTagChanged();

   UFUNCTION(BlueprintNativeEvent, Category = "Ping System")
   void _OnPingTagChanged(const FGameplayTag& pingTag);

   UFUNCTION()
   void _OnRep_PingedBy();
   void _BroadcastPingedByChanged();

   UFUNCTION(BlueprintNativeEvent, Category = "Ping System")
   void _OnPingedByChanged(AOSEPlayerState* player);

   UFUNCTION()
   void _OnRep_PingableActor();
   void _BroadcastPingableActorChanged();

   UFUNCTION(BlueprintNativeEvent, Category = "Ping System")
   void _OnPingableActorChanged(AActor* actor);

   UFUNCTION(BlueprintNativeEvent, Category = "Ping System")
   void _OnAuthoritySetup();

protected:
   // Component(s)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System")
   USceneComponent* PingUILocation = nullptr;

   // NOTE: This is assumed to be the same one as the one on UOSEPingSystemComponent.  We could pass it over at spawn time
   // but it'd be one more thing to replicate, we can skip the extra net traffic by just defining it twice.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System")
   UOSEPingSystemInfoAsset* PingInfoAsset = nullptr;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System")
   float PingDespawnTime = 20.0f;

private:
   UPROPERTY(ReplicatedUsing=_OnRep_PingTag)
   FGameplayTag _pingTag;
   UPROPERTY(ReplicatedUsing = _OnRep_PingedBy)
   AOSEPlayerState* _pingedBy = nullptr;
   UPROPERTY(ReplicatedUsing = _OnRep_PingableActor)
   TWeakObjectPtr<AActor> _pingableActor = nullptr;
   UPROPERTY(Transient)
   UOSEPingSystemComponent* _authorityPingSystemOwner = nullptr;
   UPROPERTY(Transient)
   FHitResult _hitResult;
   UPROPERTY(Transient)
   FTransform _spawnerViewXfm = FTransform::Identity;
   UPROPERTY(Transient)
   FTransform _spawnerWorldXfm = FTransform::Identity;
};
