// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "PingSystem/OSEPingableInterface.h"
#include "PingSystem/OSEPingSystemInfo.h"

// ue4
#include "Components/ActorComponent.h"

#include "OSEPingSystemComponent.generated.h"

class AOSEPingActor;

USTRUCT()
struct FGatheredPingTarget
{
   GENERATED_BODY()

   TWeakObjectPtr<AActor> Actor;
   FHitResult HitResult;
};

UCLASS(Blueprintable, BlueprintType)
class OSECORE_API UOSEPingSystemComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSEPingSystemComponent();

   // from AActor
   virtual void BeginPlay() override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UFUNCTION(Reliable, Server, WithValidation, BlueprintCallable)
   void ServerSpawnPing(const FOSEPingSpawnInfo& spawnInfo, const FHitResult& hitResult, const FTransform& spawnerViewXfm, const FTransform& spawnerWorldXfm, const FGameplayTag& pingTag, AActor* pingableTargetActor);
   UFUNCTION(Reliable, Server, WithValidation, BlueprintCallable)
   void ServerCancelPing(AOSEPingActor* pingActor, bool isPlayerInitiated);
   UFUNCTION(Reliable, Server, WithValidation, BlueprintCallable)
   void ServerRespondToPing(AOSEPingActor* pingActor, const FGameplayTag& responseTag);

   // ping actors call over here to manage their bookkeeping
   void AuthorityAddSpawnedPingActor(AOSEPingActor* pingActor);
   void AuthorityRemoveSpawnedPingActor(AOSEPingActor* pingActor);

   void LocallyAddSpawnedPingActor(AOSEPingActor* pingActor);
   void LocallyRemoveSpawnedPingActor(AOSEPingActor* pingActor);


   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAuthoritySpawnedPing, AOSEPingActor*, pingActor);
   UPROPERTY(BlueprintAssignable)
   FOnAuthoritySpawnedPing OnAuthoritySpawnedPing;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAuthorityCanceledPing, const FGameplayTag&, pingTag, bool, isPlayerInitiated);
   UPROPERTY(BlueprintAssignable)
   FOnAuthorityCanceledPing OnAuthorityCanceledPing;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAuthorityRespondedToPing, AOSEPingActor*, pingActor, const FGameplayTag&, responseTag);
   UPROPERTY(BlueprintAssignable)
   FOnAuthorityRespondedToPing OnAuthorityRespondedToPing;

   UFUNCTION(BlueprintPure)
   AOSEPingActor* GetLocalFocusedPingActor() const { return _localFocusPingActor.Get(); }
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusedPingActorChanged, AOSEPingActor*, pingActor);
   UPROPERTY(BlueprintAssignable)
   FOnFocusedPingActorChanged OnLocalFocusedPingActorChanged;

protected:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System")
   UOSEPingSystemInfoAsset* PingInfoAsset = nullptr;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping System")
   float FocusRadiusPercent = 0.2f;

   APlayerController& _GetOwnerController() const;
   bool _IsLocallyControlled() const;

private:
   void _AuthorityAddPingToBookkeeping(AOSEPingActor* pingActor);
   void _AuthorityRemovePingFromBookkeeping(AOSEPingActor* pingActor);

   void _TickLocalFocusedPingActor();

private:
   TMap<FGameplayTag, TArray<TWeakObjectPtr<AOSEPingActor>>> _serverSpawnedPingActors;

   UPROPERTY(Transient)
   TArray<TWeakObjectPtr<AOSEPingActor>> _localPingActors;
   TWeakObjectPtr<AOSEPingActor> _localFocusPingActor = nullptr;
};
