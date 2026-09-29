// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"

// tat
#include "Indicators/TATClientProxyInfo.h"

#include "TATThiefVisionSubsystem.generated.h"

class APlayerController;
struct FTATClientProxyIndicatorConfig;

USTRUCT(BlueprintType)
struct FTATThiefVisionIndicatorQuery
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query")
   bool FilterByIndicatorType = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query", Meta = (EditCondition = "FilterByIndicatorType"))
   FGameplayTagContainer IndicatorTypes;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query", Meta = (EditCondition = "FilterByIndicatorType"))
   bool IndicatorTypesMatchExact = true;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query")
   bool FilterByInstigator = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query", Meta = (EditCondition = "FilterByInstigator"))
   TObjectPtr<AActor> Instigator;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query")
   bool FilterByWorldLocation = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query", Meta = (EditCondition = "FilterByWorldLocation"))
   FVector WorldLocation = FVector::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query", Meta = (EditCondition = "FilterByWorldLocation"))
   float DistanceFromWorldLocation = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query")
   bool FilterByCustomData = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thief Vision Indicator Query", Meta = (EditCondition = "FilterByCustomData"))
   uint8 CustomData = 0;

   static FTATThiefVisionIndicatorQuery MatchAll()
   {
      return FTATThiefVisionIndicatorQuery{};
   }

   static FTATThiefVisionIndicatorQuery MatchIndicatorType(FGameplayTag indicatorType)
   {
      FTATThiefVisionIndicatorQuery query{};
      query.FilterByIndicatorType = true;
      query.IndicatorTypes.AddTag(indicatorType);
      return query;
   }

   static FTATThiefVisionIndicatorQuery MatchInstigator(AActor* instigator)
   {
      FTATThiefVisionIndicatorQuery query{};
      query.FilterByInstigator = true;
      query.Instigator = instigator;
      return query;
   }

   static FTATThiefVisionIndicatorQuery MatchWorldLocation(const FVector& worldLocation, float maxDistance)
   {
      FTATThiefVisionIndicatorQuery query{};
      query.FilterByWorldLocation = true;
      query.WorldLocation = worldLocation;
      query.DistanceFromWorldLocation = maxDistance;
      return query;
   }

   bool MatchesQuery(const FTATClientProxyInfo& clientProxyInfo) const;
};

/// Precomputed config data for thief vision indicators, so we don't have to keep looking up actor CDOs and check for data table value overrides.
USTRUCT(BlueprintType)
struct FTATThiefVisionIndicatorPrecomputedConfig
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   TSubclassOf<AActor> IndicatorClass;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   FGameplayTagContainer RequireGameplayTags;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   bool AutoRemoveOnInstigatorKnockout = false;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   float IndicatorLifeSpan = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   float IndicatorLifeSpanOutside = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   bool VisibleAtInfiniteRange = false;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   float MinimumDeduplicateDistance = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   float MaxVisibleRange = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   float HysteresisRange = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   float MaxReplicatedRange = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   float ReplicationBufferRange = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   bool AllowRotation = false;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Client Proxy Indicator")
   bool AllowScale = false;

   /// Inits this config struct from a client proxy indicator config by loading the indicator class synchronously.
   void ApplyConfigLoadSynchronous(const FTATClientProxyIndicatorConfig& cfg, const FGameplayTag& weatherType);

   /// Checks if a pawn has the tags required for this indicator to be visible
   bool IsVisibleToPawn(APawn* pawn) const;

   /// Checks if a given indicator is in replication range to a player
   bool InReplicationRange(const FVector& indicatorLocation, const FVector& playerLocation, float currentlyReplicated) const;

   /// Checks if a given indicator should be visible to a player
   bool InVisibleRange(const FVector& indicatorLocation, const FVector& playerLocation, bool currentlyVisible) const;

   FORCEINLINE float GetIndicatorLifeSpan(bool isOutside) const { return (isOutside && IndicatorLifeSpanOutside > 0) ? IndicatorLifeSpanOutside : IndicatorLifeSpan; }

   FORCEINLINE bool RequiresOutsideCheck() const { return IndicatorLifeSpanOutside > 0.0f; }
};

UCLASS()
class TAT_API UTATThiefVisionSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()

   // This is a fast array serializer for client proxy infos. At the moment it's hardcoded to direct all add/change/remove notifies to this subsystem.
   // Might be worth making that more abstract in the future - if so, we can get rid of this friend declaration.
   // See also: _ClientOnPreReplicatedRemove, _ClientOnPostReplicatedAdd, _ClientOnPostReplicatedChange
   friend struct FTATClientProxyInfoArray;

   // From USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   // From UWorldSubsystem
   virtual void OnWorldBeginPlay(UWorld& InWorld) override;
   // From UTickableWorldSubsystem
   virtual ETickableTickType GetTickableTickType() const override;

   // From UObject
   virtual void Tick(float deltaTime) override;
   virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTATThiefVisionSubsystem, STATGROUP_Tickables); }

public:
   /// Creates a new Thief Vision indicator at the specified location.
   ///
   /// This will result in a client proxy actor being spawned on each client for which the local player matches the visibility requirements
   /// for this indicator. Note that those actors are despawned if/when the player no longer matches the visibility requirements, and
   /// respawned if/when the client meets the requirements again.
   ///
   /// @param indicatorType       The type of this indicator. Refers to the Thief Vision data table row with the same tag.
   /// @param transform           Location, rotation, and scale of the indicator. Note the rotation and scale will not be applied unless the
   ///                            data table row opts in via the "Allow Rotation" and "Allow Scale" flags.
   /// @param deduplicateDistance If there is another indicator of the same type within this distance of the spawn location, the existing one
   ///                            will have its lifespan refreshed instead of a new indicator being spawned. Note that the data table row can
   ///                            set a minimum value, so you only need to specify a value here if you want a value greater than the one in the
   ///                            data table row.
   /// @param instigator          The actor responsible for this indicator being spawned. The Thief Vision system doesn't care what you set
   ///                            this to - it's just passed along to the spawned client proxy actor.
   /// @param customData          A byte-sized integer that's passed along to the client proxy actor. The Thief Vision system doesn't care what
   ///                            you set this to. It's intended to allow simple variations for indicators of the same type - for example,
   ///                            distinguishing left vs right footsteps. You could simply pass an integer here to represent a variant, or you
   ///                            could pass an enum value as a byte (converting back to that enum in the client proxy actor).
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Thief Vision")
   bool AuthoritySpawnThiefVisionIndicator(
      UPARAM(Meta = (Categories = "Indicator")) FGameplayTag indicatorType,
      const FTransform& transform,
      float deduplicateDistance = 0.0f,
      AActor* instigator = nullptr,
      uint8 customData = 0);

   /// Removes all thief vision indicators matching the query.
   /// Returns the number of removed indicators.
   int32 AuthorityRemoveThiefVisionIndicatorsMatchingQuery(const FTATThiefVisionIndicatorQuery& query);

   /// Finds all thief vision indicators that should be visible for a given player
   bool AuthorityUpdateThiefVisionIndicatorsForPlayer(APlayerController* pc, FTATClientProxyInfoArray& indicators);

   /// Update visibility of all client indicators based on distance
   void ClientSyncThiefVisionIndicatorVisibility(APlayerController* pc);

   /// Gets any components that should be hidden right now for the local player controller
   void ClientGetHiddenThiefVisionComponents(APlayerController* pc, const FVector& viewLocation, TSet<FPrimitiveComponentId>& outHiddenComponents);

   /// [Server and Clients] Called from the player controller when the thief vision ability has been activated or deactivated
   void NotifyThiefVisionStatusChanged(APlayerController* pc, bool newThiefVisionEnabled);

   /// Adds all thief vision indicator types to the gameplay tag container where the supplied callback returns true.
   void FindThiefVisionIndicatorTypes(FGameplayTagContainer& outIndicatorTypes, TFunctionRef<bool(FGameplayTag, const FTATThiefVisionIndicatorPrecomputedConfig&)> callback) const;

   /// Checks if a given character has thief vision enabled
   UFUNCTION(BlueprintCallable, Category = "Thief Vision")
   static bool IsThiefVisionEnabled(APawn* pawn);

   /// Checks if the local player has thief vision enabled (always returns false on dedicated servers)
   UFUNCTION(BlueprintPure, Category = "Thief Vision")
   FORCEINLINE bool IsThiefVisionEnabledForLocalPlayer() const { return _localPlayerThiefVisionEnabled; }

   /// Registers a component to be automatically hidden when thief vision is enabled.
   /// You should always unregister the component when done (suggestion: if you're registering the component in BeginPlay, call unregister for it in EndPlay).
   /// If delayTimeBeforeHide is greater than zero, when thief vision is enabled, the subsystem will wait this amount of time before hiding the component.
   /// This is intended to give you a chance to play any kind of transition VFX if needed. *NOTE*: If you do this, you are responsible for making sure the VFX
   /// only plays for the local player. The simplest way to handle this is to bind an event to TATThiefVisionSubsystem::OnLocalPlayerThiefVisionStatusChanged.
   UFUNCTION(BlueprintCallable, Category = "Thief Vision")
   void RegisterThiefVisionLinkedComponent(UPrimitiveComponent* component, float delayTimeBeforeHide = 0.0f);

   /// Unregisters a component that was registered with RegisterThiefVisionLinkedComponent.
   UFUNCTION(BlueprintCallable, Category = "Thief Vision")
   void UnregisterThiefVisionLinkedComponent(UPrimitiveComponent* component);

   /// Checks if a thief vision indicator type is valid. Mostly just needed for cheat-related error messages.
   FORCEINLINE bool IsValidThiefVisionIndicatorType(FGameplayTag indicatorType) const
   {
      return _indicatorConfig.Contains(indicatorType);
   }

   FORCEINLINE float GetThiefVisionIndicatorLifeSpan(FGameplayTag indicatorType) const
   {
      const FTATThiefVisionIndicatorPrecomputedConfig* cfg = _indicatorConfig.Find(indicatorType);
      return (cfg != nullptr) ? cfg->IndicatorLifeSpan : 0.0f;
   }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnThiefVisionStatusChanged, APlayerController*, controller, bool, thiefVisionEnabled);

   /// Event called when thief vision is enabled or disabled.
   /// This is fired on the server (for all players) and clients (for just the local player).
   UPROPERTY(BlueprintAssignable)
   FOnThiefVisionStatusChanged OnThiefVisionStatusChanged;

   /// Event called when the local player enables or disables thief vision.
   UPROPERTY(BlueprintAssignable)
   FOnThiefVisionStatusChanged OnLocalPlayerThiefVisionStatusChanged;

private:
   FTATClientProxyInfo* _AuthorityFindThiefVisionIndicatorClosestToLocation(FGameplayTag indicatorType, const FVector& worldLocation, float maxDistance);

   // ============== Server + client bits ==============

   bool _hasAuthority = false;
   FGameplayTag _weatherType;

   UPROPERTY(Transient)
   TMap<FGameplayTag, FTATThiefVisionIndicatorPrecomputedConfig> _indicatorConfig;

   TSet<FGameplayTag> _disabledIndicators;

   // ============== Server-only bits ==============

   int32 _AuthorityGenerateIndicatorUniqueId();

   FTATClientProxyInfo _AuthorityConstructNewThiefVisionIndicator(int32 uniqueId, const FGameplayTag& indicatorType, const FTransform& transform, AActor* instigator, uint8 customData);

   int32 _authorityNextIndicatorUniqueId = 0;

   /// All currently active server-owned thief vision indicators.
   UPROPERTY(Transient)
   TMap<int32, FTATClientProxyInfo> _authorityIndicators;

   /// Internal helper for AuthorityFindThiefVisionIndicatorsForPlayer, just here to allow reusing allocations between calls.
   TSet<int32> _authorityProcessedIndicators;

   // ============== Client-only bits ==============

   struct FClientProxyActorState
   {
      FGameplayTag IndicatorType;
      TWeakObjectPtr<AActor> Actor;
      bool Visible = false;
   };

   // Fast array serializer callbacks
   void _ClientOnPreReplicatedRemove(APlayerController* pc, const TArray<FTATClientProxyInfo>& indicators, const TArrayView<int32>& removedIndices, int32 finalSize);
   void _ClientOnPostReplicatedAdd(APlayerController* pc, const TArray<FTATClientProxyInfo>& indicators, const TArrayView<int32>& addedIndices, int32 finalSize);
   void _ClientOnPostReplicatedChange(APlayerController* pc, const TArray<FTATClientProxyInfo>& indicators, const TArrayView<int32>& changedIndices, int32 finalSize);

   /// Updates an existing client-owned proxy actor
   void _ClientUpdateProxyActor(APlayerController* pc, const FTATClientProxyInfo& info, float currentServerWorldTime, FClientProxyActorState& existingProxyState);

   /// Spawns a client-owned proxy actor that represents a thief vision indicator.
   bool _ClientSpawnProxyActor(APlayerController* pc, const FTATClientProxyInfo& indicatorInfo, float currentServerWorldTime, FClientProxyActorState& outProxyState) const;

   /// Destroys a client-owned proxy actor that represents a thief vision indicator.
   void _ClientDestroyProxyActor(FClientProxyActorState& proxyState) const;

   /// Makes a client-owned proxy actor visible or invisible, letting the actor take care of the transition if it implements the client proxy interface.
   void _ClientSetProxyActorVisibility(FClientProxyActorState& proxyState, bool newVisible, bool force = false) const;

   /// Internal helper for ClientSyncThiefVisionIndicatorActors, just here to allow reusing allocations between calls.
   TSet<int32> _clientUpdatedUniqueIndicatorIds;

   /// Mapping of unique id to client-owned indicator proxy actors
   TMap<int32, FClientProxyActorState> _clientProxyActors;

   // ============== Thief vision-linked component management ==============

   /// Gets the local player controller on this client, if any.
   APlayerController* _GetLocalPlayerController() const;

   /// Cached local player thief vision state
   bool _localPlayerThiefVisionEnabled = false;

   struct FThiefVisionComponentState
   {
      // Number of seconds to wait after setting to hidden that it should actually be hidden (allows it time to play a fade effect)
      float DelayTimeBeforeHide = 0.0f;
      // If greater than zero and game time is >= this value, the component should not be visible
      float HiddenAfterGameTime = 0.0f;
   };

   /// Registered components are components of actors that implement TATThiefVisionLinkedActorInterface that want some of their components to be visible only
   /// when thief vision is enabled.
   TMap<TWeakObjectPtr<UPrimitiveComponent>, FThiefVisionComponentState> _registeredComponents;

};
