// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

// ose
#include "OSECoreCheats.h"

#include "TATPowerNetworkSubsystem.generated.h"

TAT_API DECLARE_LOG_CATEGORY_EXTERN(LogTATPowerNetwork, Log, All);

class ATATPowerLine;
class ATATPowerJunction;
class UTATPowerNetworkComponent;
enum class ETATPowerNetworkRole : uint8;

struct TAT_API FTATPowerNetworkTraversalState
{
   using FLinkFunctionRef = TFunctionRef<bool(AActor*, AActor*)>;
   using FComponentFunctionRef = TFunctionRef<bool(UTATPowerNetworkComponent*)>;

   /// Helper constants that you can return from traverse callbacks for better readability.
   static constexpr bool ContinueTraversal = true;
   static constexpr bool StopTraversal = false;

   TQueue<AActor*> Queue;
   TSet<AActor*> Visited;

   explicit FTATPowerNetworkTraversalState(const UTATPowerNetworkSubsystem& subsystem)
      : _subsystem(subsystem)
   {}
   void AddStartingActor(AActor* actor);

   /// Iterate over all links (pairs of connected actors) in the power network
   void TraverseLinks(FLinkFunctionRef callback, bool requireEnabled = true);

   /// Iterate over all connected power network components in the network
   void TraverseComponents(FComponentFunctionRef callback, bool requireEnabled = true);

private:
   const UTATPowerNetworkSubsystem& _subsystem;
};

/// Manages power networks
UCLASS()
class TAT_API UTATPowerNetworkSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;
   virtual void PostInitialize() override;
   virtual void OnWorldBeginPlay(UWorld& world) override;
   virtual void OnWorldComponentsUpdated(UWorld& world) override;

   void TraversePowerNetworkLinks(FTATPowerNetworkTraversalState::FLinkFunctionRef callback, bool requireEnabled = true, AActor* startingActor = nullptr) const;
   void TraversePowerNetworkComponents(FTATPowerNetworkTraversalState::FComponentFunctionRef callback, bool requireEnabled = true, AActor* startingActor = nullptr) const;
   bool ContainsActor(AActor* actor) const { return actor != nullptr && _actorToPowerComponentMap.Contains(actor); }
   void AddActor(AActor* actor);
   void RemoveActor(AActor* actor);

   void RegisterConnection(AActor* source, AActor* target);
   void UnregisterConnection(AActor* source, AActor* target);

   /// Checks if srcActor has a direct (explicit reference) connection to dstActor
   bool HasDirectConnectionTo(const AActor* srcActor, const AActor* dstActor) const;

   TConstArrayView<TWeakObjectPtr<AActor>> GetReverseConnections(const AActor* actor) const;

   UTATPowerNetworkComponent* FindFirstConnectedPowerSource(const AActor* actor, bool requireEnabled, bool requirePowered) const;

   UTATPowerNetworkComponent* GetPowerNetworkComponentForActor(const AActor* actor) const;

   /// Notifies of a change in a power network component that could affect connected components.
   /// For example, if you are building a power source actor, you would call this when turning power on or off to propagate the change through the network.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Power Network Subsystem")
   void AuthorityNotifyPowerComponentStateChanged(UTATPowerNetworkComponent* powerComp);

   const TSet<TObjectPtr<UTATPowerNetworkComponent>>& GetNetwork() const { return _network; }

private:
   void _OnPowerProviderStateChanged(UTATPowerNetworkComponent* powerComp);

   UFUNCTION()
   void _DumpAllPowerNetworkActorsToLog();

public:
   void TogglePowerNetworkDebugVis(APlayerController* pc);

private:
   UFUNCTION()
   void _TickPowerNetworkDebugVis(TWeakObjectPtr<APlayerController> pcWeak);

#if OSE_CHEATS_ENABLED
   TMap<TWeakObjectPtr<APlayerController>, FTimerHandle> _debugVisTickTimerHandles;
#endif


   UPROPERTY(Transient)
   TSet<TObjectPtr<UTATPowerNetworkComponent>> _network;

   TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UTATPowerNetworkComponent>> _actorToPowerComponentMap;
   TMap<TWeakObjectPtr<AActor>, TArray<TWeakObjectPtr<AActor>>> _reverseDependencies;
};
