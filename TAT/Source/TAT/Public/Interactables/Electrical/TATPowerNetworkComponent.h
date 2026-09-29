// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"

#include "TATPowerNetworkComponent.generated.h"

class ATATPowerSource;

UENUM(BlueprintType)
enum class ETATPowerNetworkRole : uint8
{
   None = 0,
   Junction  UMETA(Tooltip = "A branching node in the network. Can have any number of connections"),
   Connector UMETA(Tooltip = "An edge in the network that links other actors together. Generally a power line of some kind"),
   Provider  UMETA(Tooltip = "A leaf in the network that connects to exactly one edge or node and provides power to all connected actors"),
   Consumer  UMETA(Tooltip = "A leaf in the network that connects to exactly one junction or connector and consumes power if it's available"),
};

USTRUCT(BlueprintType)
struct TAT_API FTATPowerNetworkComponentState
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Network Component State")
   bool IsEnabled = true;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Network Component State")
   bool IsPowered = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Network Component State")
   bool IsPowerSurge = false;

   bool operator==(const FTATPowerNetworkComponentState& rhs) const
   {
      return IsEnabled == rhs.IsEnabled && IsPowered == rhs.IsPowered && IsPowerSurge == rhs.IsPowerSurge;
   }
};

UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class TAT_API UTATPowerNetworkComponent : public UActorComponent
{
   GENERATED_BODY()

   friend class UTATPowerNetworkSubsystem;

public:
   UTATPowerNetworkComponent();

   UFUNCTION(BlueprintCallable, Category = "TAT|Electricity")
   static UTATPowerNetworkComponent* GetPowerNetworkComponent(const AActor* actor);

#if WITH_EDITOR
   // From UObject
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif

   // From UActorComponent
   virtual void OnRegister() override;
   virtual void OnUnregister() override;
   virtual void InitializeComponent() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Power Network")
   ETATPowerNetworkRole PowerNetworkRole = ETATPowerNetworkRole::None;

   /// Should this power network component be enabled at BeginPlay?
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Power Network")
   bool StartEnabled = true;

   /// Should it be considered to be powered even without provider
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Power Network")
   bool AlwaysActAsPowered = false;

   /// Controls whether this component will receive power-related events like OnPoweredChanged.
   /// This must be enabled if the owning actor needs to reflect power state for gameplay or VFX reasons.
   /// Turning this off improves performance when propagating power network state changes.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Power Network")
   bool ReceivePowerNetworkEvents = true;

   UPROPERTY(EditInstanceOnly, Category = "Power Network", Meta = (AllowedClasses = "/Script/TAT.TATPowerNetworkInterface", EditCondition = "PowerNetworkRole == ETATPowerNetworkRole::Connector", EditConditionHides))
   TObjectPtr<AActor> ConnectorLinks[2] = { nullptr, nullptr };

   UPROPERTY(EditInstanceOnly, Category = "Power Network", Meta = (AllowedClasses = "/Script/TAT.TATPowerNetworkInterface", EditCondition = "PowerNetworkRole == ETATPowerNetworkRole::Provider || PowerNetworkRole == ETATPowerNetworkRole::Consumer", EditConditionHides))
   TObjectPtr<AActor> PowerLink;

#if WITH_EDITORONLY_DATA
   /// How far up above the spline to draw the debug visualization for a connector/power line
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Power Network", AdvancedDisplay, Meta = (EditCondition = "PowerNetworkRole == ETATPowerNetworkRole::Connector", EditConditionHides))
   float ConnectorEditorVisualizationOffset = 50.0f;
#endif

   /// Can power flow through this component? Power network components can be disabled if, for example, the owning actor was broken by a player.
   UFUNCTION(BlueprintPure, Category = "Power Network")
   bool IsEnabled() const;

   /// Is power currently flowing through this component?
   UFUNCTION(BlueprintPure, Category = "Power Network")
   bool IsPowered() const;

   /// Is a power surge currently happening?
   UFUNCTION(BlueprintPure, Category = "Power Network")
   bool IsPowerSurgeActive() const;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnabledChanged, bool, newEnabled);
   UPROPERTY(BlueprintAssignable)
   FOnEnabledChanged OnEnabledChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPoweredChanged, bool, newPowered);
   UPROPERTY(BlueprintAssignable)
   FOnPoweredChanged OnPoweredChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPowerSurgeChanged, bool, newPowerSurgeActive);
   UPROPERTY(BlueprintAssignable)
   FOnPowerSurgeChanged OnPowerSurgeChanged;

   /// Called for all power state changes (enabled, powered, etc).
   /// Useful to manage visual state for different combinations.
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPowerStateChanged, FTATPowerNetworkComponentState, prevState, FTATPowerNetworkComponentState, newState);
   UPROPERTY(BlueprintAssignable)
   FOnPowerStateChanged OnPowerStateChanged;

   /// Enables or disables this power network component.
   /// Disabling a power network component prevents power from flowing through it - this will cut power to any devices for which
   /// this is along the only powered path between that device and a powered power source.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Power Network")
   void AuthoritySetEnabled(bool newEnabled);

   /// Triggers a power surge lasting the specified number of seconds.
   /// Returns true if the power surge was actually started, or false if no power surge was started (eg. this power network component isn't enabled and/or powered)
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Power Network")
   bool AuthorityTriggerPowerSurge(float durationSeconds, bool propagateAcrossPowerNetwork = true, bool propagateEnabledOnly = true);

   /// Gets the number of connected actor slots on this component.
   /// This includes null slots - for example, a power line will always return 2, even if it's not configured with valid actors on both ends.
   UFUNCTION(BlueprintPure, Category = "Power Network")
   int32 NumLinkSlots() const;

   /// Gets the directly connected actor at the specified index (call NumConnectedActors() to get the number of connected actor slots)
   /// This can return true with a null actor if the component is not configured properly in the level.
   UFUNCTION(BlueprintPure, Category = "Power Network")
   bool GetActorConnectedToLinkSlot(int32 index, AActor*& outConnectedActor) const;

   /// Gets an array view representing all connected actors.
   /// It's not guaranteed that array elements are non-null - always check for null.
   /// This is intended to be used in the same stack frame for which it is called. Do not store this array view - make a copy if you need one.
   TArrayView<TObjectPtr<AActor>> GetLinkSlotActors() const;

   /// Checks if we have a direct, one-way connection (eg. an explicit actor reference) from this component to a specific actor.
   /// This does _not_ check for reverse connections.
   bool HasDirectConnectionTo(const AActor* actor, int32* outSlotIndex = nullptr) const;

   /// Checks if we have a reverse connection from an actor to this component.
   bool HasReverseConnectionTo(const AActor* actor) const;

   /// Checks if this is connected to another actor, accounting for reverse connections.
   UFUNCTION(BlueprintPure, Category = "Power Network")
   FORCEINLINE bool IsDirectlyConnectedTo(const AActor* actor) const { return HasDirectConnectionTo(actor) || HasReverseConnectionTo(actor); }

   /// Gets the world location that represents the location an actor is connected to on this power network component.
   /// If this returns false, the actor is not connected to this power network component, and worldLocation may be set to (0, 0, 0).
   UFUNCTION(BlueprintPure, Category = "Power Network")
   bool GetWorldLocationForLinkedActor(const AActor* linkedActor, FVector& worldLocation, bool forDebugVis = false) const;

   /// Gets a world location for a connection point on a power network actor.
   /// Different power network actors can interpret the index differently - for example, a power line might have exactly two indices,
   /// where each represents one end of the power line.
   ///
   /// If forDebugVis is true, the returned locations will be more appropriate for debug visualization drawing.
   UFUNCTION(BlueprintPure, Category = "Power Network")
   bool GetWorldLocationForConnectorIndex(int32 index, FVector& worldLocation, bool forDebugVis = false) const;

protected:
   void _RegisterConnections();

#if WITH_EDITORONLY_DATA
   TArray<TWeakObjectPtr<AActor>, TInlineAllocator<3>> _editorRegisteredConnections;
#endif

   void _AuthorityUpdatePoweredState(bool newPowered);

   /// Requests that the power network subsystem updates the state of any component connected to this one.
   /// Call after any state change that originated from this component.
   void _AuthorityUpdatePowerNetworkState();

   void _OnPowerStateChanged(const FTATPowerNetworkComponentState& prevState, const FTATPowerNetworkComponentState& newState);

private:
   UFUNCTION()
   void _AuthorityOnPowerSurgeEnded();

   void _AuthorityActivatePowerSurge();
   void _AuthorityDeactivatePowerSurge();

   UFUNCTION()
   void _OnRep_PowerNetworkState(const FTATPowerNetworkComponentState& prevState);

   UPROPERTY(Replicated, ReplicatedUsing = _OnRep_PowerNetworkState)
   FTATPowerNetworkComponentState _powerNetworkState;

   /// Basically a reference counter for power surges so that multiple effects causing a surge cause exactly one surge,
   /// and the surge will end when the last effect causing it does.
   int32 _authorityPowerSurgeCount = 0;
};
