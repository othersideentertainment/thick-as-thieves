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

/*=============================================================================
	AkRoomComponent.cpp:
=============================================================================*/

#include "AkRoomComponent.h"
#include "AkComponentHelpers.h"
#include "AkAcousticPortal.h"
#include "AkAudioDevice.h"
#include "AkGeometryComponent.h"
#include "AkLateReverbComponent.h"
#include "AkSurfaceReflectorSetComponent.h"
#include "Components/BrushComponent.h"
#include "GameFramework/Volume.h"
#include "Model.h"
#include "EngineUtils.h"
#include "AkAudioEvent.h"
#include "AkSettingsPerUser.h"
#include "Wwise/API/WwiseSpatialAudioAPI.h"
#if WITH_EDITOR
#include "AkDrawRoomComponent.h"
#include "AkSpatialAudioHelper.h"
#endif

#define MOVEMENT_STOP_TIMEOUT 0.1f

/*------------------------------------------------------------------------------------
	UAkRoomComponent
------------------------------------------------------------------------------------*/

const FString UAkRoomComponent::OutdoorsRoomName = "Outdoors";

UAkRoomComponent::UAkRoomComponent(const class FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	Parent = NULL;

	WallOcclusion = 1.0f;

	bEnable = true;
	bUseAttachParentBound = true;
	AutoPost = false;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bTickInEditor = true;

#if WITH_EDITOR
	if (AkSpatialAudioHelper::GetObjectReplacedEvent())
	{
		AkSpatialAudioHelper::GetObjectReplacedEvent()->AddUObject(this, &UAkRoomComponent::HandleObjectsReplaced);
	}
	bWantsInitializeComponent = true;
	bWantsOnUpdateTransform = true;
#else
	bWantsOnUpdateTransform = false;
#endif
}

void UAkRoomComponent::SetEnable(bool bInEnable)
{
	if (bEnable == bInEnable)
	{
		return;
	}

	bEnable = bInEnable;

	if (bEnable)
	{
		AddSpatialAudioRoom();
	}
	else
	{
		RemoveSpatialAudioRoom();
	}
}

void UAkRoomComponent::SetDynamic(bool bInDynamic)
{
	bDynamic = bInDynamic;
#if WITH_EDITOR
	bWantsOnUpdateTransform = true;

	// If we're PIE, or somehow otherwise in a game world in editor, simulate the bDynamic behaviour.
	if (AkComponentHelpers::IsInGameWorld(this))
	{
		bWantsOnUpdateTransform = bDynamic;
	}
#else
	bWantsOnUpdateTransform = bDynamic;
#endif
}

void UAkRoomComponent::SetTransmissionLoss(float InTransmissionLoss)
{
	if (InTransmissionLoss != WallOcclusion)
	{
		WallOcclusion = InTransmissionLoss;
		if (bIsRegisteredWithWwise)
		{
			UpdateSpatialAudioRoom();
		}
	}
}

void UAkRoomComponent::SetAuxSendLevel(float InAuxSendLevel)
{
	if (InAuxSendLevel != AuxSendLevel)
	{
		AuxSendLevel = InAuxSendLevel;
		if (bIsRegisteredWithWwise)
		{
			UpdateSpatialAudioRoom();
		}
	}
}

FName UAkRoomComponent::GetName() const
{
	return Parent->GetFName();
}

bool UAkRoomComponent::HasEffectOnLocation(const FVector& Location) const
{
	// Need to add a small radius, because on the Mac, EncompassesPoint returns false if
	// Location is exactly equal to the Volume's location
	static float RADIUS = 0.01f;
	return RoomIsActive() && EncompassesPoint(Location, RADIUS);
}

bool UAkRoomComponent::RoomIsActive() const
{ 
	return Parent.IsValid() && bEnable && !IsRunningCommandlet();
}

void UAkRoomComponent::OnRegister()
{
	Super::OnRegister();

#if WITH_EDITOR
	bWantsOnUpdateTransform = true;

	// If we're PIE, or somehow otherwise in a game world in editor, simulate the bDynamic behaviour.
	if (AkComponentHelpers::IsInGameWorld(this))
	{
		bWantsOnUpdateTransform = bDynamic;
	}
#else
	bWantsOnUpdateTransform = bDynamic;
#endif

	SetRelativeTransform(FTransform::Identity);
	InitializeParent();
	// We want to add / update the room both in BeginPlay and OnRegister. BeginPlay for aux bus and reverb level assignment, OnRegister for portal room assignment and visualization
	if (!bIsRegisteredWithWwise)
	{
		AddSpatialAudioRoom();
	}
	else
	{
		UpdateSpatialAudioRoom();
	}
#if WITH_EDITOR
	if (GetDefault<UAkSettingsPerUser>()->VisualizeRoomsAndPortals)
	{
		InitializeDrawComponent();
	}
#endif
}

void UAkRoomComponent::OnUnregister()
{
	RemoveSpatialAudioRoom();
	Super::OnUnregister();
}

#if WITH_EDITOR
void UAkRoomComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
	RegisterVisEnabledCallback();
}

void UAkRoomComponent::InitializeComponent()
{
	Super::InitializeComponent();
	RegisterVisEnabledCallback();
}

void UAkRoomComponent::PostLoad()
{
	Super::PostLoad();
	RegisterVisEnabledCallback();
}


void UAkRoomComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	UAkSettingsPerUser* AkSettingsPerUser = GetMutableDefault<UAkSettingsPerUser>();
	AkSettingsPerUser->OnShowRoomsPortalsChanged.Remove(ShowRoomsChangedHandle);
	ShowRoomsChangedHandle.Reset();
	ConnectedPortals.Empty();
	DestroyDrawComponent();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}
#endif // WITH_EDITOR

void UAkRoomComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
#if WITH_EDITOR
	if (bRequiresDeferredBeginPlay)
	{
		BeginPlayInternal();
		bRequiresDeferredBeginPlay = false;
	}
#endif

	// In PIE, only update in tick if bDynamic is true (simulate the behaviour in the no-editor game build).
	bool bUpdate = true;
#if WITH_EDITOR
	if (AkComponentHelpers::IsInGameWorld(this))
	{
		bUpdate = bDynamic;
	}
#endif
	if (bUpdate)
	{
		if (Moving)
		{
			SecondsSinceMovement += DeltaTime;
			if (SecondsSinceMovement >= MOVEMENT_STOP_TIMEOUT)
			{
				FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
				if (AkAudioDevice != nullptr)
				{
					//Update room facing in sound engine
					UpdateSpatialAudioRoom(true);
				}
				Moving = false;
			}
		}
	}

	if (ShouldSetReverbZone())
	{
		if (!bIsAReverbZoneInWwise)
		{
			// make sure the parent is still valid before setting the reverb zone
			UpdateParentRoom();
			bReverbZoneNeedsUpdate = true;
		}

		if (bReverbZoneNeedsUpdate)
		{
			SetReverbZone();
			bReverbZoneNeedsUpdate = false;
		}
	}
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

   // OSE BEGIN: turn off tick if not dirty
   if (!bDynamic && !(ShouldSetReverbZone() && !bIsAReverbZoneInWwise))
   {
      SetComponentTickEnabled(false);
   }
   // OSE END
}

#if WITH_EDITOR
void UAkRoomComponent::BeginDestroy()
{
	if (AkSpatialAudioHelper::GetObjectReplacedEvent())
	{
		AkSpatialAudioHelper::GetObjectReplacedEvent()->RemoveAll(this);
	}
	Super::BeginDestroy();
}

void UAkRoomComponent::HandleObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	if (ReplacementMap.Contains(Parent.Get()))
	{
		InitializeParent();
		if (!bIsRegisteredWithWwise)
		{
			AddSpatialAudioRoom();
		}
		else
		{
			UpdateSpatialAudioRoom();
		}
	}
	if (ReplacementMap.Contains(GeometryComponent))
	{
		GeometryComponent = AkComponentHelpers::GetChildComponentOfType<UAkAcousticTextureSetComponent>(*Parent.Get());
		if (GeometryComponent == nullptr || GeometryComponent->HasAnyFlags(RF_Transient) || GeometryComponent->IsBeingDestroyed())
		{
			GeometryComponent = NewObject<UAkGeometryComponent>(Parent.Get(), TEXT("GeometryComponent"));
			UAkGeometryComponent* GeomComp = Cast<UAkGeometryComponent>(GeometryComponent);
			GeomComp->MeshType = AkMeshType::CollisionMesh;
			GeomComp->bWasAddedByRoom = true;
			GeometryComponent->AttachToComponent(Parent.Get(), FAttachmentTransformRules::KeepRelativeTransform);
			GeometryComponent->RegisterComponent();

			if (!RoomIsActive())
				GeomComp->RemoveGeometry();
		}
		SendGeometry();
		UpdateSpatialAudioRoom();
	}
}

void UAkRoomComponent::RegisterVisEnabledCallback()
{
	if (!ShowRoomsChangedHandle.IsValid())
	{
		UAkSettingsPerUser* AkSettingsPerUser = GetMutableDefault<UAkSettingsPerUser>();
		ShowRoomsChangedHandle = AkSettingsPerUser->OnShowRoomsPortalsChanged.AddLambda([this, AkSettingsPerUser]()
		{
			if (AkSettingsPerUser->VisualizeRoomsAndPortals)
			{
				InitializeDrawComponent();
			}
			else
			{
				DestroyDrawComponent();
			}
		});
	}
}

void UAkRoomComponent::InitializeDrawComponent()
{
	if (AActor* Owner = GetOwner())
	{
		if (DrawRoomComponent == nullptr)
		{
			DrawRoomComponent = NewObject<UDrawRoomComponent>(Owner, NAME_None, RF_Transactional | RF_TextExportTransient);
			DrawRoomComponent->SetupAttachment(this);
			DrawRoomComponent->SetIsVisualizationComponent(true);
			DrawRoomComponent->CreationMethod = CreationMethod;
			DrawRoomComponent->RegisterComponentWithWorld(GetWorld());
			DrawRoomComponent->MarkRenderStateDirty();
		}
	}
}

void UAkRoomComponent::DestroyDrawComponent()
{
	if (DrawRoomComponent != nullptr)
	{
		DrawRoomComponent->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, /*bCallModify=*/ false));
		DrawRoomComponent->DestroyComponent();
		DrawRoomComponent = nullptr;
	}
}
#endif 

void UAkRoomComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Moving = true;
	SecondsSinceMovement = 0.0f;
   // OSE BEGIN: Dirty for update on next tick
   SetComponentTickEnabled(true);
   // OSE END
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
}

bool UAkRoomComponent::MoveComponentImpl(
	const FVector & Delta,
	const FQuat & NewRotation,
	bool bSweep,
	FHitResult * Hit,
	EMoveComponentFlags MoveFlags,
	ETeleportType Teleport)
{
	if (AkComponentHelpers::DoesMovementRecenterChild(this, Parent.Get(), Delta))
	{
		Super::MoveComponentImpl(Delta, NewRotation, bSweep, Hit, MoveFlags, Teleport);
	}
	return false;
}

void UAkRoomComponent::InitializeParent()
{
	USceneComponent* SceneParent = GetAttachParent();
	if (SceneParent != nullptr)
	{
		Parent = Cast<UPrimitiveComponent>(SceneParent);
		if (!Parent.IsValid())
		{
			bEnable = false;
			AkComponentHelpers::LogAttachmentError(this, SceneParent, "UPrimitiveComponent");
			return;
		}

		UBodySetup* bodySetup = Parent->GetBodySetup();
		if (bodySetup == nullptr || !AkComponentHelpers::HasSimpleCollisionGeometry(bodySetup))
		{
			if (UBrushComponent* brush = Cast<UBrushComponent>(Parent))
			{
				brush->BuildSimpleBrushCollision();
			}
			else
			{
				AkComponentHelpers::LogSimpleGeometryWarning(Parent.Get(), this);
			}
		}
	}
}

FString UAkRoomComponent::GetRoomName() const
{
	FString nameStr = UObject::GetName();

	AActor* roomOwner = GetOwner();
	if (roomOwner != nullptr)
	{
#if WITH_EDITOR
		nameStr = roomOwner->GetActorLabel();
#else
		nameStr = roomOwner->GetName();
#endif
		if (Parent.IsValid())
		{
			TInlineComponentArray<UAkRoomComponent*> RoomComponents;
			roomOwner->GetComponents(RoomComponents);
			if (RoomComponents.Num() > 1)
				nameStr.Append(FString("_").Append(Parent->GetName()));
		}
	}

	return nameStr;
}

void UAkRoomComponent::GetRoomParams(AkRoomParams& outParams)
{
	FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
	if (!AkAudioDevice)
		return;

	if (Parent.IsValid())
	{
		AkComponentHelpers::GetPrimitiveUpAndFront(*Parent.Get(), outParams.Up, outParams.Front);
	}

	outParams.TransmissionLoss = WallOcclusion;

	UAkLateReverbComponent* ReverbComp = GetReverbComponent();
	if (ReverbComp && ReverbComp->bEnable)
	{
		if (UNLIKELY(!ReverbComp->AuxBus && ReverbComp->AuxBusName.IsEmpty()))
		{
			outParams.ReverbAuxBus = AK_INVALID_AUX_ID;
		}
		else
		{
			outParams.ReverbAuxBus = ReverbComp->GetAuxBusId();
		}
		outParams.ReverbLevel = ReverbComp->SendLevel;
	}

	if (GeometryComponent != nullptr)
		outParams.GeometryInstanceID = GeometryComponent->GetGeometrySetID();

	outParams.RoomPriority = Priority;
	
	outParams.RoomGameObj_AuxSendLevelToSelf = AuxSendLevel;
	outParams.RoomGameObj_KeepRegistered = AkAudioEvent == NULL ? false : true;
	const UAkSettings* AkSettings = GetDefault<UAkSettings>();
	if (AkSettings != nullptr && AkSettings->ReverbRTPCsInUse())
		outParams.RoomGameObj_KeepRegistered = true;
}

UPrimitiveComponent* UAkRoomComponent::GetPrimitiveParent() const
{
	return Parent.Get();
}

void UAkRoomComponent::SetReverbZone(const UAkRoomComponent* InParentRoom, float InTransitionRegionWidth)
{
	AActor* ParentActor = InParentRoom ? InParentRoom->GetOwner() : nullptr;
	UpdateParentRoomActor(ParentActor);
	UpdateTransitionRegionWidth(InTransitionRegionWidth);
	bEnableReverbZone = true;

	if (!bIsAReverbZoneInWwise)
	{
		bReverbZoneNeedsUpdate = true;
	   // OSE BEGIN: Dirty for update on next tick
	   SetComponentTickEnabled(true);
	   // OSE END
	}

	if (!bEnable)
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("UAkRoomComponent::SetReverbZone: The Room component %s is not enabled. When the Room gets enabled, it will be set as a Reverb Zone."), *GetRoomName());
	}
}

void UAkRoomComponent::RemoveReverbZone()
{
	bEnableReverbZone = false;

	if (!bIsAReverbZoneInWwise)
	{
		return;
	}

	if (!AkComponentHelpers::IsInGameWorld(this))
	{
		return;
	}

	auto* SpatialAudio = IWwiseSpatialAudioAPI::Get();
	if (LIKELY(SpatialAudio))
	{
		SpatialAudio->RemoveReverbZone(GetRoomID());
		bIsAReverbZoneInWwise = false;
	}
}

void UAkRoomComponent::AddSpatialAudioRoom()
{
	if (RoomIsActive())
	{
		SendGeometry();

		FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
		if (AkAudioDevice)
		{
			AkRoomParams Params;
			GetRoomParams(Params);
			if (AkAudioDevice->AddRoom(this, Params) == AK_Success)
			{
				bIsRegisteredWithWwise = true;
				if (Params.RoomGameObj_KeepRegistered)
				{
					Super::SetAttenuationScalingFactor();
				}
			}

			AkAudioDevice->IndexRoom(this);
			// Update all portals. This room might be colliding with one of them.
			AkAudioDevice->PortalsNeedRoomUpdate(GetWorld());
		}

		if (GetOwner() != nullptr && bIsRegisteredWithWwise && AkComponentHelpers::IsInGameWorld(this))
		{
			UAkLateReverbComponent* pRvbComp = GetReverbComponent();
			if (pRvbComp != nullptr)
				pRvbComp->UpdateRTPCs(this);
		}
	}
}

void UAkRoomComponent::UpdateSpatialAudioRoom(bool bUpdateRoomIndex)
{
	if (RoomIsActive())
	{
		FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
		if (AkAudioDevice)
		{
			if (bIsRegisteredWithWwise)
			{
				AkRoomParams Params;
				GetRoomParams(Params);
				AkAudioDevice->UpdateRoom(this, Params);
			}

			if (bUpdateRoomIndex)
			{
				AkAudioDevice->ReindexRoom(this);
			}
			// Update all portals. This room might be colliding with one of them after this update.
			AkAudioDevice->PortalsNeedRoomUpdate(GetWorld());
		}

		if (GetOwner() != nullptr && AkComponentHelpers::IsInGameWorld(this))
		{
			UAkLateReverbComponent* pRvbComp = GetReverbComponent();
			if (pRvbComp != nullptr)
				pRvbComp->UpdateRTPCs(this);
		}
	}
}

void UAkRoomComponent::RemoveSpatialAudioRoom()
{
	if (Parent.IsValid() && !IsRunningCommandlet())
	{
		RemoveGeometry();

		FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
		if (AkAudioDevice)
		{
			if (GetOwner() != nullptr && AkComponentHelpers::IsInGameWorld(this))
			{
				// stop all sounds posted on the room
				Stop();
			}

			if (AkAudioDevice->RemoveRoom(this) == AK_Success)
			{
				bIsRegisteredWithWwise = false;
				bIsAReverbZoneInWwise = false;
			}

			AkAudioDevice->UnindexRoom(this);
			// Update connected portals. They have lost a connection with this room. No need to update all other portals.
			for (auto& Portal : ConnectedPortals)
			{
				if (Portal.Value.IsValid())
				{
					Portal.Value->PortalRoomsNeedUpdate();
					if (!bIsRegisteredWithWwise) // Only need to do this if remove room succeeded
					{
						// Connected portals have lost the connection with this room in Wwise. We need to call SetPortal() again.
						Portal.Value->PortalNeedsUpdate();
					}
				}
			}
		}
	}
}

int32 UAkRoomComponent::PostAssociatedAkEvent(int32 CallbackMask, const FOnAkPostEventCallback& PostEventCallback)
{
	if (LIKELY(IsValid(AkAudioEvent)))
	{
		return PostAkEvent(AkAudioEvent, CallbackMask, PostEventCallback);
	}

	UE_LOG(LogAkAudio, Error, TEXT("Failed to post invalid AkAudioEvent on Room component '%s'"), *GetRoomName());
	return AK_INVALID_PLAYING_ID;
}

AkPlayingID UAkRoomComponent::PostAkEventByNameWithDelegate(
	UAkAudioEvent* AkEvent,
	const FString& in_EventName,
	int32 CallbackMask, const FOnAkPostEventCallback& PostEventCallback)
{
	AkPlayingID PlayingID = AK_INVALID_PLAYING_ID;

	auto AudioDevice = FAkAudioDevice::Get();
	if (AudioDevice)
	{
		const AkUInt32 ShortID = AudioDevice->GetShortID(AkEvent, in_EventName);
		PlayingID = AkEvent->PostOnGameObject(this, PostEventCallback, CallbackMask);
	}

	return PlayingID;
}

void UAkRoomComponent::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	if (AkComponentHelpers::ShouldDeferBeginPlay(this))
		bRequiresDeferredBeginPlay = true;
	else
		BeginPlayInternal();
#else
	BeginPlayInternal();
#endif
}

void UAkRoomComponent::BeginPlayInternal()
{
	if (Parent.IsValid())
	{
		GeometryComponent = AkComponentHelpers::GetChildComponentOfType<UAkAcousticTextureSetComponent>(*Parent.Get());
		if (GeometryComponent == nullptr || GeometryComponent->HasAnyFlags(RF_Transient) || GeometryComponent->IsBeingDestroyed())
		{
			static const FName GeometryComponentName = TEXT("GeometryComponent");
			GeometryComponent = NewObject<UAkGeometryComponent>(Parent.Get(), GeometryComponentName);
			UAkGeometryComponent* geom = Cast<UAkGeometryComponent>(GeometryComponent);
			geom->MeshType = AkMeshType::CollisionMesh;
			geom->bWasAddedByRoom = true;
			GeometryComponent->AttachToComponent(Parent.Get(), FAttachmentTransformRules::KeepRelativeTransform);
			GeometryComponent->RegisterComponent();

			if (!RoomIsActive())
				geom->RemoveGeometry();
		}
	}

	// We want to add / update the room both in BeginPlay and OnRegister. BeginPlay for aux bus and reverb level assignment, OnRegister for portal room assignment and visualization
	if (!bIsRegisteredWithWwise)
	{
		AddSpatialAudioRoom();
	}
	else
	{
		SendGeometry();
		UpdateSpatialAudioRoom();
	}

	if (ShouldSetReverbZone() && !bIsAReverbZoneInWwise)
	{
		UpdateParentRoom();
		SetReverbZone();
	}

	if (AutoPost)
	{
		PostAssociatedAkEvent(0, FOnAkPostEventCallback());
	}
}

void UAkRoomComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (HasActiveEvents())
	{
		Stop();
	}

	ResetParentRoom();
	RemoveReverbZone();

	Super::EndPlay(EndPlayReason);
}

void UAkRoomComponent::SetGeometryComponent(UAkAcousticTextureSetComponent* textureSetComponent)
{
	if (GeometryComponent != nullptr)
	{
		RemoveGeometry();
	}
	GeometryComponent = textureSetComponent;
	if (RoomIsActive())
	{
		SendGeometry();
		UpdateSpatialAudioRoom();
	}
}

#if WITH_EDITOR
void UAkRoomComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName memberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (memberPropertyName == GET_MEMBER_NAME_CHECKED(UAkRoomComponent, WallOcclusion) ||
		memberPropertyName == GET_MEMBER_NAME_CHECKED(UAkRoomComponent, AuxSendLevel))
	{
		// Set the room in Spatial Audio again to update the room parameters, if it has already been set.
		if (bIsRegisteredWithWwise) UpdateSpatialAudioRoom();
	}
	if (memberPropertyName == GET_MEMBER_NAME_CHECKED(UAkRoomComponent, bEnableReverbZone))
	{
		OnSetEnableReverbZone();
	}
	if (memberPropertyName == GET_MEMBER_NAME_CHECKED(UAkRoomComponent, ParentRoomActor))
	{
		bReverbZoneNeedsUpdate = true;
	}
	if (memberPropertyName == GET_MEMBER_NAME_CHECKED(UAkRoomComponent, TransitionRegionWidth))
	{
		if (TransitionRegionWidth < 0.f)
			TransitionRegionWidth = 0.f;

		bReverbZoneNeedsUpdate = true;
	}

	if (IsAReverbZone())
	{
		UpdateParentRoom();
		check(ParentRoom != this);
	}

	// If this is a blueprint component Super::PostEditChangeProperty will turn this object into garbage
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UAkRoomComponent::OnParentNameChanged()
{
	TArray<AkPortalID> PortalsToRemove;

	for (auto& Portal : ConnectedPortals)
	{
		if (!Portal.Value.IsValid())
		{
			PortalsToRemove.Add(Portal.Key);
			continue;
		}
		Portal.Value->UpdateRoomNames();
	}

	for (auto& PortalID : PortalsToRemove)
	{
		ConnectedPortals.Remove(PortalID);
	}
}
#endif

bool UAkRoomComponent::EncompassesPoint(FVector Point, float SphereRadius/*=0.f*/, float* OutDistanceToPoint/*=nullptr*/) const
{
	if (Parent.IsValid())
	{
		return AkComponentHelpers::EncompassesPoint(*Parent.Get(), Point, SphereRadius, OutDistanceToPoint);
	}
	FString actorString = FString("NONE");
	if (GetOwner() != nullptr)
		actorString = GetOwner()->GetName();
	UE_LOG(LogAkAudio, Error, TEXT("UAkRoomComponent::EncompassesPoint : Error. In actor %s, AkRoomComponent %s has an invalid Parent."), *actorString, *UObject::GetName());
	return false;
}

void UAkRoomComponent::SendGeometry()
{
	if (GeometryComponent)
	{
		UAkGeometryComponent* GeometryComp = Cast<UAkGeometryComponent>(GeometryComponent);
		if (GeometryComp && GeometryComp->bWasAddedByRoom)
		{
			if (!GeometryComp->GetGeometryHasBeenSent())
				GeometryComp->SendGeometry();
			if (!GeometryComp->GetGeometryInstanceHasBeenSent())
				GeometryComp->UpdateGeometry();
		}
		UAkSurfaceReflectorSetComponent* SurfaceReflector = Cast<UAkSurfaceReflectorSetComponent>(GeometryComponent);
		if (SurfaceReflector && !SurfaceReflector->bEnableSurfaceReflectors)
		{
			if (!SurfaceReflector->GetGeometryHasBeenSent())
				SurfaceReflector->SendSurfaceReflectorSet();
			if (!SurfaceReflector->GetGeometryInstanceHasBeenSent())
				SurfaceReflector->UpdateSurfaceReflectorSet();
		}
	}
}

void UAkRoomComponent::RemoveGeometry()
{
	if (IsValid(GeometryComponent))
	{
		UAkGeometryComponent* GeometryComp = Cast<UAkGeometryComponent>(GeometryComponent);
		if (GeometryComp && GeometryComp->bWasAddedByRoom)
		{
			GeometryComp->RemoveGeometry();
		}
		UAkSurfaceReflectorSetComponent* SurfaceReflector = Cast<UAkSurfaceReflectorSetComponent>(GeometryComponent);
		if (SurfaceReflector && !SurfaceReflector->bEnableSurfaceReflectors)
		{
			SurfaceReflector->RemoveSurfaceReflectorSet();
		}
	}
}

UAkLateReverbComponent* UAkRoomComponent::GetReverbComponent()
{
	UAkLateReverbComponent* pRvbComp = nullptr;
	if (Parent.IsValid())
	{
		pRvbComp = AkComponentHelpers::GetChildComponentOfType<UAkLateReverbComponent>(*Parent.Get());
	}
	return pRvbComp;
}

void UAkRoomComponent::AddPortalConnection(UAkPortalComponent* in_pPortal)
{
	ConnectedPortals.Add(in_pPortal->GetPortalID(), TWeakObjectPtr<UAkPortalComponent>(in_pPortal));
}

void UAkRoomComponent::RemovePortalConnection(AkPortalID in_portalID)
{
	ConnectedPortals.Remove(in_portalID);
}

void UAkRoomComponent::SetEnableReverbZone(bool bInEnableReverbZone)
{
	if (bEnableReverbZone != bInEnableReverbZone)
	{
		bEnableReverbZone = bInEnableReverbZone;
		OnSetEnableReverbZone();
	}
}

void UAkRoomComponent::UpdateParentRoomActor(AActor* InParentRoomActor)
{
	if (ParentRoomActor != InParentRoomActor)
	{
		ParentRoomActor = InParentRoomActor;
		UpdateParentRoom();
		bReverbZoneNeedsUpdate = true;
	}
}

void UAkRoomComponent::UpdateTransitionRegionWidth(float InTransitionRegionWidth)
{
	if (InTransitionRegionWidth < 0.f)
		InTransitionRegionWidth = 0.f;

	if (TransitionRegionWidth != InTransitionRegionWidth)
	{
		TransitionRegionWidth = InTransitionRegionWidth;
		bReverbZoneNeedsUpdate = true;
	}
}

bool UAkRoomComponent::CanBecomeReverbZone() const
{
	if (GeometryComponent == nullptr)
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkRoomComponent::SetReverbZone: Reverb Zone Room component %s doesn't have an associated geometry."), *GetRoomName());
		return false;
	}

	for (auto& iter : ConnectedPortals)
	{
		if (iter.Value.IsValid() && !iter.Value->PortalPlacementValid())
		{
			UE_LOG(LogAkAudio, Error, TEXT("UAkRoomComponent::SetReverbZone: Room component %s cannot become a Reverb Zone because Portal %s would connect rooms in the same hierarchy."), *GetRoomName(), *iter.Value->GetPortalName());
			return false;
		}
	}

	return true;
}

void UAkRoomComponent::SetReverbZone()
{
	if (!AkComponentHelpers::IsInGameWorld(this))
	{
		return;
	}

	if (TransitionRegionWidth < 0.f)
	{
		UE_LOG(LogAkAudio, Warning, TEXT("UAkGameplayStatics::SetReverbZone: Transition region width for Reverb Zone %s is a negative number. It has been clamped to 0."), *GetRoomName());
		TransitionRegionWidth = 0.f;
	}

	if (CanBecomeReverbZone())
	{
		auto* SpatialAudio = IWwiseSpatialAudioAPI::Get();
		if (LIKELY(SpatialAudio))
		{
			SpatialAudio->SetReverbZone(GetRoomID(), GetParentRoomID(), TransitionRegionWidth);
			bIsAReverbZoneInWwise = true;
		}
	}
	else
	{
		bEnableReverbZone = false;
	}
}

bool UAkRoomComponent::IsAReverbZone() const
{
	return bEnable && (bIsAReverbZoneInWwise || bEnableReverbZone);
}

AkRoomID UAkRoomComponent::GetParentRoomID() const
{
	return ParentRoom.IsValid() ? ParentRoom->GetRoomID() : AK::SpatialAudio::kOutdoorRoomID;
}

bool UAkRoomComponent::ShouldSetReverbZone()
{
	return bEnable && bEnableReverbZone && GeometryComponent;
}

void UAkRoomComponent::OnSetEnableReverbZone()
{
	if (bEnableReverbZone)
	{
		// make sure the parent is still valid before setting the reverb zone
		UpdateParentRoom();
		bReverbZoneNeedsUpdate = true;
	}
	else
	{
		RemoveReverbZone();
	}
   // OSE BEGIN: Dirty for update on next tick
   SetComponentTickEnabled(true);
   // OSE END
}

void UAkRoomComponent::UpdateParentRoom()
{
	if (ParentRoomActor == nullptr)
	{
		ResetParentRoom();
		return;
	}

	UAkRoomComponent* ParentRoomComponent = Cast<UAkRoomComponent>(ParentRoomActor->GetComponentByClass(UAkRoomComponent::StaticClass()));
	if (ParentRoomComponent == nullptr || !ParentRoomComponent->bEnable)
	{
		UE_LOG(LogAkAudio, Warning, TEXT("UAkRoomComponent::UpdateParentRoom '%s': No valid Room component found for ParentRoomActor. The Parent Room will be reset to the 'Outdoors' room."), *GetRoomName());
		ResetParentRoom();
		return;
	}

	SetParentRoom(ParentRoomComponent);
}

void UAkRoomComponent::ResetParentRoom()
{
	if (ParentRoom.IsValid())
	{
		ParentRoom.Reset();
		ParentRoomName = OutdoorsRoomName;
	}
}

bool UAkRoomComponent::IsAParentOf(TWeakObjectPtr<const UAkRoomComponent> InRoom) const
{
	if (!InRoom.IsValid() || !InRoom->IsAReverbZone())
	{
		return false;
	}

	auto InRoomParent = InRoom->ParentRoom;

	if (!InRoomParent.IsValid())
	{
		return false;
	}

	if (GetRoomID() == InRoomParent->GetRoomID())
	{
		return true;
	}


	return IsAParentOf(InRoomParent);
}

AkRoomID UAkRoomComponent::GetRootID() const
{
	if (!IsAReverbZone())
	{
		return GetRoomID();
	}

	if (!ParentRoom.IsValid() ||
		ParentRoom == this )
	{
		return AK::SpatialAudio::kOutdoorRoomID;
	}

	return ParentRoom->GetRootID();
}

void UAkRoomComponent::SetParentRoom(TWeakObjectPtr<const UAkRoomComponent> InParentRoom)
{
	if (!InParentRoom.IsValid())
	{
		ResetParentRoom();
		return;
	}

	auto InParentRoomID = InParentRoom->GetRoomID();

	// make sure the parent room is not this room
	if (GetRoomID() == InParentRoomID)
	{
		UE_LOG(LogAkAudio, Warning, TEXT("UAkRoomComponent::SetParentRoom '%s': Assigned Parent Room '%s' is invalid. This Room and the assigned Parent Room are the same. The Parent Room will be reset to the 'Outdoors' room."), *GetRoomName(), *InParentRoom->GetRoomName());
		ResetParentRoom();
		return;
	}

	// make sure this room is not a parent of the current parent room (circular dependency)
	if (IsAParentOf(InParentRoom))
	{
		UE_LOG(LogAkAudio, Warning, TEXT("UAkRoomComponent::SetParentRoom '%s': Assigned Parent Room '%s' is invalid. This room is a parent of the assigned Parent Room. The Parent Room will be reset to the 'Outdoors' room."), *GetRoomName(), *InParentRoom->GetRoomName());
		ResetParentRoom();
		return;
	}

	if (InParentRoomID != GetParentRoomID())
	{
		ParentRoom = InParentRoom;
		ParentRoomName = InParentRoom->GetRoomName();
	}
}
