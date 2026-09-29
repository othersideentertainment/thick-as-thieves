/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#pragma once
#include "AkReverbDescriptor.h"
#include "GameFramework/Volume.h"
#include "AkGameObject.h"
#include "AkRoomComponent.generated.h"

class UAkLateReverbComponent;

UCLASS(ClassGroup = Audiokinetic, BlueprintType, hidecategories = (Transform, Rendering, Mobility, LOD, Component, Activation, Tags), meta = (BlueprintSpawnableComponent))
class AKAUDIO_API UAkRoomComponent : public UAkGameObject
{
	GENERATED_BODY()

public:
	typedef WwiseUnrealHelper::AkSpatialAudioIDKeyFuncs<TWeakObjectPtr<UAkPortalComponent>, false> PortalComponentSpatialAudioIDKeyFuncs;
	typedef TMap<AkPortalID, TWeakObjectPtr<UAkPortalComponent>, FDefaultSetAllocator, PortalComponentSpatialAudioIDKeyFuncs> PortalComponentMap;

	static const FString OutdoorsRoomName;

	UAkRoomComponent(const class FObjectInitializer& ObjectInitializer);

	/** 
	* Enable the Room Component to set this volume as a Spatial Audio Room. Additional properties are available in the Room and AkEvent categories.
	*/
	UPROPERTY(EditAnywhere, BlueprintSetter = SetEnable, Category="EnableComponent", meta = (DisplayName = "Enable Room"))
	bool bEnable = false;

	UFUNCTION(BlueprintSetter, Category = "Audiokinetic|AkRoomComponent")
	void SetEnable(bool bInEnable);

	/** 
	* If true, the Portal connections for this Room can change during runtime when this Room moves.
	* For worlds containing many Portals, this can be expensive. Note that this Room's Portal connections 
	* may still change, even when Room Is Dynamic = false, when dynamic Portals are moved (that is when Portals
	* move who have bDynamic = true).
	*/
	UPROPERTY(EditAnywhere, BlueprintSetter = SetDynamic, Category = "Room", meta = (DisplayName = "Room Is Dynamic"))
	bool bDynamic = false;

	UFUNCTION(BlueprintSetter, Category = "Audiokinetic|AkRoomComponent")
	void SetDynamic(bool bInDynamic);

	/**
	* The precedence in which the Rooms is applied. In the case of overlapping Rooms, only the one
	* with the highest priority is chosen. If two or more overlapping Rooms have the same
	* priority, the chosen Room is unpredictable.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	float Priority = .0f;

	/**
	* Used to set the transmission loss value in wwise, on emitters in the Room, when no audio paths to the 
	* listener are found via sound propagation in Wwise Spatial Audio. This value can be thought of as 
	* 'thickness', as it relates to how much sound energy is transmitted through the wall. Valid range 0.0f-1.0f.
	*/
	UPROPERTY(EditAnywhere, BlueprintSetter = SetTransmissionLoss, DisplayName = "Transmission Loss", Category = "Room", meta = (ClampMin=0.0f, ClampMax=1.0f, UIMin=0.0f, UIMax=1.0f))
	float WallOcclusion = .0f;

	/**
	* Sets the transmission loss value.
	* @param InTransmissionLoss - The new value for the transmission loss. Valid range 0.0f-1.0f.
	*/
	UFUNCTION(BlueprintSetter, Category = "Audiokinetic|AkRoomComponent")
	void SetTransmissionLoss(float InTransmissionLoss);

	/**
	* Send level for sounds that are posted on the room. Valid range: (0.f-1.f). A value of 0 disables the aux send.
	*/
	UPROPERTY(EditAnywhere, BlueprintSetter = SetAuxSendLevel, Category = "AkEvent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AuxSendLevel = .0f;

	/**
	* Sets the Send level. A value of 0 disables the aux send.
	* @param InAuxSendLevel - The new value for the Send level. Valid range 0.0f-1.0f.
	*/
	UFUNCTION(BlueprintSetter, Category = "Audiokinetic|AkRoomComponent")
	void SetAuxSendLevel(float InAuxSendLevel);

	/** Automatically post the associated AkAudioEvent on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AkEvent", SimpleDisplay)
	bool AutoPost = false;

	/**
	* Sets this Room as a Reverb Zone.
	* A Reverb Zone models a region that has a distinct reverb effect or ambience but does not require Portals to connect to neighboring Rooms.
	* Use Reverb Zones instead of standard Rooms whenever there are no obvious walls,
	* or generally when there is more negative space than positive space at the interface between the two regions.
	*/
	UPROPERTY(EditAnywhere, BlueprintSetter = SetEnableReverbZone, Category = "ReverbZone")
	bool bEnableReverbZone = false;

	/** Set bEnableReverbZone to a new value and set or remove the Reverb Zone in Wwise. */
	UFUNCTION(BlueprintSetter, Category = "Audiokinetic|AkRoomComponent")
	void SetEnableReverbZone(bool bInEnableReverbZone);

	/**
	* Establishes a parent-child relationship between two Rooms and allows for sound propagation between them
	* as if they were the same Room, without the need for a connecting Portal.
	* A parent Room can have multiple Reverb Zones, but a Reverb Zone can only have a single Parent.
	* The Reverb Zone and its parent are both Rooms, and as such, must be specified using Enable Room.
	* The automatically created 'outdoors' Room is commonly used as a parent Room for Reverb Zones, because they often model open spaces.
	* When set to None (the default value), the Reverb Zone is automatically attached to the 'outdoors' Room.
	*/
	UPROPERTY(EditAnywhere, BlueprintSetter = UpdateParentRoomActor, Category = "ReverbZone", meta = (EditCondition = "bEnableReverbZone"))
	TObjectPtr<AActor> ParentRoomActor = nullptr;

	/** Set ParentRoomActor with a new actor and update the Reverb Zone in Wwise asynchronously. */
	UFUNCTION(BlueprintSetter, Category = "Audiokinetic|AkRoomComponent")
	void UpdateParentRoomActor(AActor* InParentRoomActor);

	/**
	* The name of the Parent Room of this Reverb Zone.
	* If the Parent Room Actor is None, or if the Parent Room Actor is not valid, the 'outdoors' Room is chosen and printed here.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ReverbZone", meta = (EditCondition = "bEnableReverbZone"))
	FString ParentRoomName = OutdoorsRoomName;

	/**
	* Width of the transition region between the Reverb Zone and its parent.
	* The transition region acts the same way as Portal depth, but is centered around the Reverb Zone geometry. Transition regions are only added on surfaces where transmission loss is set to 0.
	* The value must be positive. Negative values are treated as 0.
	*/
	UPROPERTY(EditAnywhere, BlueprintSetter = UpdateTransitionRegionWidth, Category = "ReverbZone", meta = (EditCondition = "bEnableReverbZone"))
	float TransitionRegionWidth = 100.f;

	/** Set TransitionRegionWidth to a new value and updates the Reverb Zone in Wwise asynchronously. */
	UFUNCTION(BlueprintSetter, Category = "Audiokinetic|AkRoomComponent")
	void UpdateTransitionRegionWidth(float InTransitionRegionWidth);

	/** Posts this game object's AkAudioEvent to Wwise, using this as the game object source */
	virtual int32 PostAssociatedAkEvent(
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/AkAudio.EAkCallbackType")) int32 CallbackMask,
		const FOnAkPostEventCallback& PostEventCallback
	);

	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkRoomComponent")
	UPrimitiveComponent* GetPrimitiveParent() const;

	/**
	* Establishes a parent-child relationship between this Room and a parent Room and allows for sound propagation between them as if they were the same Room, without the need for a connecting Portal.
	* Setting a Room as a Reverb Zone is useful in situations where two or more acoustic environments are not easily modeled as closed Rooms connected by Portals.
	* Possible uses for Reverb Zones include: a covered area with no walls, a forested area within an outdoor space, or any situation where multiple reverb effects are desired within a common space.
	* Reverb Zones have many advantages compared to standard Game-Defined Auxiliary Sends (that is compared to the AkLateReverbComponent or the AkReverbVolume). They are part of the wet path, and form reverb chains with other Rooms;
	* they are spatialized according to their 3D extent; they are also subject to other acoustic phenomena simulated in Wwise Spatial Audio, such as diffraction and transmission.
	* A Reverb Zone needs to be a Room component with an associated geometry.
	*
	* @param InParentRoom - The parent Room component. A parent Room can have multiple Reverb Zones, but a Reverb Zone can only have a single Parent. If a Room is already assigned to a parent Room, it is first removed from the old parent (exactly as if RemoveReverbZone were called), and is then assigned to the new parent Room. A Reverb Zone can be the parent of another Reverb Zone. A Room cannot be its own parent. Defaults to the 'Outdoors' Room if left empty.
	* @param InTransitionRegionWidth - Width of the transition region between the Reverb Zone and its parent. The transition region acts the same way as Portal depth, but is centered around the Reverb Zone geometry. Transition regions are only added on surfaces where transmission loss is set to 0. The value must be positive. Negative values are treated as 0.
	*/
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkRoomComponent")
	void SetReverbZone(const UAkRoomComponent* InParentRoom, float InTransitionRegionWidth);

	/**
	* Removes this Reverb Zone from its parent.
	* Sound can no longer propagate between the two Rooms, unless they are explicitly connected with a Portal.
	*/
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkRoomComponent")
	void RemoveReverbZone();

	/** Add a Room in AK Spatial Audio. */
	void AddSpatialAudioRoom();

	/** 
		Modify a Room in AK Spatial Audio. 
		Pass bUpdateRoomIndex = true if the room transform has changed
	*/
	void UpdateSpatialAudioRoom(bool bUpdateRoomIndex = false);

	/** Remove a Room from AK Spatial Audio	*/
	void RemoveSpatialAudioRoom();

	bool HasEffectOnLocation(const FVector& Location) const;

	bool RoomIsActive() const;

	AkRoomID GetRoomID() const { return AkRoomID(this); }

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
#if WITH_EDITOR
	virtual void BeginDestroy() override;
	virtual void OnComponentCreated() override;
	virtual void InitializeComponent() override;
	virtual void PostLoad() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
#endif
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;
	virtual bool MoveComponentImpl(
		const FVector & Delta,
		const FQuat & NewRotation,
		bool bSweep,
		FHitResult * Hit,
		EMoveComponentFlags MoveFlags,
		ETeleportType Teleport) override;

	FName GetName() const;

	virtual AkPlayingID PostAkEventByNameWithDelegate(
		UAkAudioEvent* AkEvent,
		const FString& in_EventName,
		int32 CallbackMask, const FOnAkPostEventCallback& PostEventCallback);

	// Begin USceneComponent Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void OnParentNameChanged();
#endif

	/** Set the geometry component used to send the geometry of the Room to Wwise. For example, in a Blueprint that has a static mesh component with an AkGeometry child component, this function can be called in BeginPlay to associate that AkGeometry component with this Room component.
	 *  If this Room component has a sibling AkGeometry component (or AkSurfaceReflectorSet component), they are automatically associated and there is no need to call this function.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkRoomComponent")
	void SetGeometryComponent(UAkAcousticTextureSetComponent* textureSetComponent);

	void AddPortalConnection(UAkPortalComponent* in_pPortal);
	void RemovePortalConnection(AkPortalID in_portalID);
	PortalComponentMap& GetConnectedPortals() { return ConnectedPortals; }

	bool IsAReverbZone() const;
	AkRoomID GetParentRoomID() const;
	TWeakObjectPtr<const UAkRoomComponent> GetParentRoom() const { return ParentRoom; }

	/// <summary>
	/// Returns the root Room ID of this Room's hierarchy.
	/// Returns this Room's ID if this Room is not a Reverb Zone.
	/// </summary>
	/// <returns></returns>
	AkRoomID GetRootID() const;

	void SetReverbZone();

	FString GetRoomName() const;
	UAkLateReverbComponent* GetReverbComponent();

private:
	TWeakObjectPtr<class UPrimitiveComponent> Parent;

	UPROPERTY(Transient)
	TObjectPtr<class UAkAcousticTextureSetComponent> GeometryComponent = nullptr;

	PortalComponentMap ConnectedPortals;

	void InitializeParent();
	void GetRoomParams(AkRoomParams& outParams);
	bool EncompassesPoint(FVector Point, float SphereRadius = 0.f, float* OutDistanceToPoint = nullptr) const;
	void BeginPlayInternal();
	void SendGeometry();
	void RemoveGeometry();
	float SecondsSinceMovement = 0.0f;
	bool Moving = false;

	bool ShouldSetReverbZone();
	void OnSetEnableReverbZone();
	bool CanBecomeReverbZone() const;
	
	void UpdateParentRoom();
	void ResetParentRoom();
	bool IsAParentOf(TWeakObjectPtr<const UAkRoomComponent> InRoom) const;
	void SetParentRoom(TWeakObjectPtr<const UAkRoomComponent> InParentRoom);

	UPROPERTY()
	TWeakObjectPtr<const UAkRoomComponent> ParentRoom;
	bool bIsAReverbZoneInWwise = false;
	bool bReverbZoneNeedsUpdate = false;

#if WITH_EDITOR
	void HandleObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap);
	bool bRequiresDeferredBeginPlay = false;
	class UDrawRoomComponent* DrawRoomComponent = nullptr;
	void RegisterVisEnabledCallback();
	void InitializeDrawComponent();
	void DestroyDrawComponent();
	FDelegateHandle ShowRoomsChangedHandle;
#endif
};
