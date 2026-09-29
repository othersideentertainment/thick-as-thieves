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
	AkLateReverbComponent.cpp:
=============================================================================*/

#include "AkLateReverbComponent.h"
#include "AkCustomVersion.h"
#include "AkSettingsPerUser.h"
#include "AkComponentHelpers.h"
#include "AkAudioDevice.h"
#include "AkAuxBus.h"
#include "AkRoomComponent.h"
#include "AkSurfaceReflectorSetComponent.h"
#include "AkGeometryComponent.h"
#include "AkAcousticTextureSetComponent.h"
#include "Components/BrushComponent.h"
#include "Model.h"
#include "GameFramework/Volume.h"

#include "Engine/Canvas.h"
#include "CanvasItem.h"

#if WITH_EDITOR
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "AkAudioStyle.h"
#include "AkSpatialAudioHelper.h"
#endif

/*------------------------------------------------------------------------------------
	UAkLateReverbComponent
------------------------------------------------------------------------------------*/
#if WITH_EDITOR
float UAkLateReverbComponent::TextVisualizerHeightOffset = 80.0f;
#endif

UAkLateReverbComponent::UAkLateReverbComponent(const class FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	// SceneComponent property initialization
	bUseAttachParentBound = true;
	bWantsOnUpdateTransform = true;

	// LateReverbComponent property initialization
	SendLevel = 1.0f;
	FadeRate = 0.5f;
	Priority = 1.0f;
	Parent = TWeakObjectPtr<UPrimitiveComponent>();
	bEnable = true;

#if WITH_EDITOR
	// In editor we always want to tick in case the global RTPCs become active, or aux bus assignment is enabled.

	if (AkSpatialAudioHelper::GetObjectReplacedEvent())
	{
		AkSpatialAudioHelper::GetObjectReplacedEvent()->AddUObject(this, &UAkLateReverbComponent::HandleObjectsReplaced);
	}
#endif
}

void UAkLateReverbComponent::PostLoad()
{
	Super::PostLoad();
	const int32 AkVersion = GetLinkerCustomVersion(FAkCustomVersion::GUID);

	if (AkVersion < FAkCustomVersion::ReverbAuxBusAutoAssignment)
	{
		AutoAssignAuxBus = false;
		AuxBusManual = AuxBus;
	}

#if WITH_EDITOR
	RegisterReverbInfoEnabledCallback();
#endif
}

void UAkLateReverbComponent::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FAkCustomVersion::GUID);
	Super::Serialize(Ar);
}

bool UAkLateReverbComponent::HasEffectOnLocation(const FVector& Location) const
{
	// Need to add a small radius, because on the Mac, EncompassesPoint returns false if
	// Location is exactly equal to the Volume's location
	static float RADIUS = 0.01f;
	return LateReverbIsActive() && EncompassesPoint(Location, RADIUS);
}

void UAkLateReverbComponent::SetAutoAssignAuxBus(bool bInEnable)
{
	if (bInEnable == AutoAssignAuxBus)
	{
		return;
	}
	AutoAssignAuxBus = bInEnable;
	if (AutoAssignAuxBus)
	{
		AuxBusManual = AuxBus ;
		DecayEstimationNeedsUpdate = true;
	}
	else
	{
		AuxBus = AuxBusManual;
		ReverbParamsChanged = true;
	}
}

uint32 UAkLateReverbComponent::GetAuxBusId() const
{
	return FAkAudioDevice::GetShortID(AuxBus, AuxBusName);
}

void UAkLateReverbComponent::InitializeParent()
{
	USceneComponent* SceneParent = GetAttachParent();
	if (SceneParent != nullptr)
	{
		ReverbDescriptor.SetPrimitive(Cast<UPrimitiveComponent>(SceneParent));
		Parent = Cast<UPrimitiveComponent>(SceneParent);
		if (Parent.IsValid())
		{
			ReverbDescriptor.SetReverbComponent(this);

			DecayEstimationNeedsUpdate = true;
			PredelayEstimationNeedsUpdate = true;

			UBodySetup* bodySetup = Parent->GetBodySetup();
			if (bodySetup == nullptr || !AkComponentHelpers::HasSimpleCollisionGeometry(bodySetup))
			{
				if (UBrushComponent* brush = Cast<UBrushComponent>(Parent.Get()))
					brush->BuildSimpleBrushCollision();
				else
					AkComponentHelpers::LogSimpleGeometryWarning(Parent.Get(), this);
			}
		}
		else
		{
			bEnable = false;
			AkComponentHelpers::LogAttachmentError(this, SceneParent, "UPrimitiveComponent");
			return;
		}
#if WITH_EDITOR
		if (!IsRunningCommandlet())
		{
			DestroyTextVisualizers();
			InitTextVisualizers();
			bTextValuesNeedUpdate = true;
			bTextVisibilityNeedUpdate = true;
		}
#endif
	}
	else // will happen when this component gets detached from its parent
	{
		Parent = TWeakObjectPtr<UPrimitiveComponent>();
		ReverbDescriptor.SetPrimitive(nullptr);
		bEnable = false;
	}
}

void UAkLateReverbComponent::BeginPlay()
{
	Super::BeginPlay();

	DecayEstimationNeedsUpdate = true;
	PredelayEstimationNeedsUpdate = true;

	UAkRoomComponent* pRoomCmpt = nullptr;
	if (Parent.IsValid())
	{
		pRoomCmpt = AkComponentHelpers::GetChildComponentOfType<UAkRoomComponent>(*Parent.Get());
	}

	if (!pRoomCmpt || !pRoomCmpt->RoomIsActive())
	{
		FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
		if (AkAudioDevice && LateReverbIsActive())
		{
			AkAudioDevice->IndexLateReverb(this);
		}
	}
}

void UAkLateReverbComponent::BeginDestroy()
{
	Super::BeginDestroy();
	if (TextureSetComponent.IsValid())
	{
		TextureSetComponent->SetReverbDescriptor(nullptr);
	}
	ReverbDescriptor.SetPrimitive(nullptr);
#if WITH_EDITOR
	if (AkSpatialAudioHelper::GetObjectReplacedEvent())
	{
		AkSpatialAudioHelper::GetObjectReplacedEvent()->RemoveAll(this);
	}
#endif
}

void UAkLateReverbComponent::OnRegister()
{
	Super::OnRegister();
	SetRelativeTransform(FTransform::Identity);
	InitializeParent();
	ParentChanged();

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
#if WITH_EDITOR
	bTickInEditor = true;
#endif
}

void UAkLateReverbComponent::ParentChanged()
{
	if (Parent.IsValid())
	{
#if WITH_EDITOR
		RegisterReverbAssignmentChangedCallback();
		RegisterGlobalDecayAbsorptionChangedCallback();
		RegisterReverbRTPCChangedCallback();
#endif
		// In the case where a blueprint class has a texture set component and a late reverb component as siblings, We can't know which will be registered first.
		// We need to check for the sibling in each OnRegister function and associate the texture set component to the late reverb when they are both registered.
		if (UAkSurfaceReflectorSetComponent* surfaceComponent = AkComponentHelpers::GetChildComponentOfType<UAkSurfaceReflectorSetComponent>(*Parent.Get()))
		{
			AssociateAkTextureSetComponent(surfaceComponent);
		}
		else if (UAkGeometryComponent* geometryComponent = AkComponentHelpers::GetChildComponentOfType<UAkGeometryComponent>(*Parent.Get()))
		{
			AssociateAkTextureSetComponent(geometryComponent);
		}
	}
}

void UAkLateReverbComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
	if (AkAudioDevice && IsIndexed)
	{
		AkAudioDevice->UnindexLateReverb(this);
	}
}

void UAkLateReverbComponent::OnUnregister()
{
#if WITH_EDITOR
	if (!HasAnyFlags(RF_Transient))
	{
		DestroyTextVisualizers();
	}

	UAkSettings* AkSettings = GetMutableDefault<UAkSettings>();
	if (AkSettings != nullptr)
	{
		if (ReverbAssignmentChangedHandle.IsValid())
		{
			AkSettings->OnReverbAssignmentChanged.Remove(ReverbAssignmentChangedHandle);
		}
		if (GlobalDecayAbsorptionChangedHandle.IsValid())
		{
			AkSettings->OnGlobalDecayAbsorptionChanged.Remove(GlobalDecayAbsorptionChangedHandle);
		}
		if (RTPCChangedHandle.IsValid())
		{
			AkSettings->OnReverbRTPCChanged.Remove(RTPCChangedHandle);
		}
	}
#endif
	Super::OnUnregister();
}

bool UAkLateReverbComponent::MoveComponentImpl(
	const FVector & Delta,
	const FQuat & NewRotation,
	bool bSweep,
	FHitResult * Hit,
	EMoveComponentFlags MoveFlags,
	ETeleportType Teleport)
{
	if (AkComponentHelpers::DoesMovementRecenterChild(this, Parent.Get(), Delta))
		Super::MoveComponentImpl(Delta, NewRotation, bSweep, Hit, MoveFlags, Teleport);

	return false;
}

void UAkLateReverbComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
	DecayEstimationNeedsUpdate = ReverbDescriptor.ShouldEstimateDecay();
	PredelayEstimationNeedsUpdate = ReverbDescriptor.ShouldEstimatePredelay();

	if (IsIndexed)
	{
		FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
		if (AkAudioDevice)
			AkAudioDevice->ReindexLateReverb(this);
	}

#if WITH_EDITOR
	// text visualizers should not scale
	if (TextVisualizerLabels)
	{
		TextVisualizerLabels->SetWorldScale3D(FVector::OneVector);
	}
	if (TextVisualizerValues)
	{
		TextVisualizerValues->SetWorldScale3D(FVector::OneVector);
	}
#endif
}

void UAkLateReverbComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ReverbDescriptor.RequiresUpdates())
	{
		// Update decay and time to first reflection estimations when the size is changed, every PARAM_ESTIMATION_UPDATE_PERIOD seconds.
		if (SecondsSinceDecayUpdate < PARAM_ESTIMATION_UPDATE_PERIOD)
		{
			SecondsSinceDecayUpdate += DeltaTime;
		}
		if ((TextureSetHasChanged || DecayEstimationNeedsUpdate) && SecondsSinceDecayUpdate >= PARAM_ESTIMATION_UPDATE_PERIOD)
		{
			RecalculateDecay();
			DecayEstimationNeedsUpdate = false;
			TextureSetHasChanged = false;
#if WITH_EDITOR
			bTextValuesNeedUpdate = true;
#endif
		}
		if (SecondsSincePredelayUpdate < PARAM_ESTIMATION_UPDATE_PERIOD)
		{
			SecondsSincePredelayUpdate += DeltaTime;
		}
		if (PredelayEstimationNeedsUpdate && SecondsSincePredelayUpdate >= PARAM_ESTIMATION_UPDATE_PERIOD)
		{
			RecalculatePredelay();
			PredelayEstimationNeedsUpdate = false;
#if WITH_EDITOR
			bTextValuesNeedUpdate = true;
#endif
		}
		if (ReverbAssignmentNeedsUpdate)
		{
			UpdateDecayEstimation(ReverbDescriptor.T60Decay, ReverbDescriptor.PrimitiveVolume, ReverbDescriptor.PrimitiveSurfaceArea);
			ReverbAssignmentNeedsUpdate = false;
#if WITH_EDITOR
			bTextValuesNeedUpdate = true;
#endif
		}
		if (ReverbParamsChanged)
		{
			OnReverbParamsChanged();
			ReverbParamsChanged = false;
#if WITH_EDITOR
			bTextValuesNeedUpdate = true;
#endif
		}
	}

#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (World && World->WorldType == EWorldType::Editor)
	{
		if (GCurrentLevelEditingViewportClient != nullptr)
		{
			// Keep the text renderers pointing to the camera.
			if (TextVisualizerLabels)
			{
				FVector PointTo = GCurrentLevelEditingViewportClient->GetViewLocation() - TextVisualizerLabels->GetComponentLocation();
				TextVisualizerLabels->SetWorldRotation(PointTo.Rotation());
			}
			if (TextVisualizerValues)
			{
				FVector PointTo = GCurrentLevelEditingViewportClient->GetViewLocation() - TextVisualizerValues->GetComponentLocation();
				TextVisualizerValues->SetWorldRotation(PointTo.Rotation());
			}
		}

		// Only update the text visibility when selecting or unselecting actors in editor.
		if (GetOwner()->IsSelected() && !bWasSelected)
		{
			bWasSelected = true;
			bTextVisibilityNeedUpdate = true;
		}
		if (!GetOwner()->IsSelected() && bWasSelected)
		{
			bWasSelected = false;
			bTextVisibilityNeedUpdate = true;
		}
	}

	if (bTextValuesNeedUpdate && bEnable)
	{
		UpdateTextValues();
		bTextValuesNeedUpdate = false;
	}

	if (bTextVisibilityNeedUpdate)
	{
		UpdateTextVisibility();
		bTextVisibilityNeedUpdate = false;
	}
#endif
}

void UAkLateReverbComponent::RecalculateDecay()
{
	if (ReverbDescriptor.ShouldEstimateDecay())
	{
		ReverbDescriptor.CalculateT60(this);
		SecondsSinceDecayUpdate = 0.0f;
	}
}

void UAkLateReverbComponent::RecalculatePredelay()
{
	if (ReverbDescriptor.ShouldEstimatePredelay())
	{
		ReverbDescriptor.CalculateTimeToFirstReflection();
		SecondsSincePredelayUpdate = 0.0f;
	}
}

void UAkLateReverbComponent::TextureSetUpdated()
{
	TextureSetHasChanged = true;
}

#if WITH_EDITOR
void UAkLateReverbComponent::RegisterReverbInfoEnabledCallback()
{
	if (ShowReverbInfoChangedHandle.IsValid()) return;

	UAkSettingsPerUser* AkSettingsPerUser = GetMutableDefault<UAkSettingsPerUser>();
	if (AkSettingsPerUser == nullptr) return;

	ShowReverbInfoChangedHandle = AkSettingsPerUser->OnShowReverbInfoChanged.AddLambda([this, AkSettingsPerUser]()
	{
		UpdateTextVisibility();
	});
}

void UAkLateReverbComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	UAkSettingsPerUser* AkSettingsPerUser = GetMutableDefault<UAkSettingsPerUser>();
	if (AkSettingsPerUser == nullptr) return;

	AkSettingsPerUser->OnShowReverbInfoChanged.Remove(ShowReverbInfoChangedHandle);
	ShowReverbInfoChangedHandle.Reset();

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UAkLateReverbComponent::InitializeComponent()
{
	Super::InitializeComponent();
	RegisterReverbInfoEnabledCallback();
}

void UAkLateReverbComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
	RegisterReverbInfoEnabledCallback();
}

void UAkLateReverbComponent::HandleObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	if (ReplacementMap.Contains(Parent.Get()))
	{
		InitializeParent();
		DecayEstimationNeedsUpdate = true;
		PredelayEstimationNeedsUpdate = true;
	}
	if (ReplacementMap.Contains(TextureSetComponent.Get()))
	{
		if (Parent.IsValid())
		{
			if (UAkSurfaceReflectorSetComponent* SurfaceComponent = AkComponentHelpers::GetChildComponentOfType<UAkSurfaceReflectorSetComponent>(*Parent.Get()))
			{
				AssociateAkTextureSetComponent(SurfaceComponent);
			}
			else if (UAkGeometryComponent* GeomComponent = AkComponentHelpers::GetChildComponentOfType<UAkGeometryComponent>(*Parent.Get()))
			{
				AssociateAkTextureSetComponent(GeomComponent);
			}
		}
	}
}

void UAkLateReverbComponent::UpdateTextVisibility()
{
	if (!IsTextVisualizersInitialized()) return;
	if (GetOwner() == nullptr) return;

	bool bIsVisible = false;

	if (GetDefault<UAkSettingsPerUser>()->bShowReverbInfo && bEnable && GetWorld() != nullptr)
	{
		EWorldType::Type WorldType = GetWorld()->WorldType;
		if (WorldType == EWorldType::Editor)
		{
			bIsVisible = GetOwner()->IsSelected();
		}
		else if (WorldType == EWorldType::EditorPreview)
		{
			bIsVisible = true;
		}
	}

	TextVisualizerLabels->SetVisibility(bIsVisible);
	TextVisualizerValues->SetVisibility(bIsVisible);
}

FText UAkLateReverbComponent::GetTextValues() const
{
	// Get a nicely formatted string showing the values of all of the reverb properties.
	FString BusName = FString("NONE");
	if (AuxBus != nullptr)
		AuxBus->GetName(BusName);
	FNumberFormattingOptions NumberFormat;
	NumberFormat.MaximumFractionalDigits = 2;
	FString volumeString = FText::AsNumber(EnvironmentVolume, &NumberFormat).ToString();
	FString areaString = FText::AsNumber(SurfaceArea, &NumberFormat).ToString();

	const FString missingRoomString = FString("Requires sibling AkRoom component");
	FString decayString = missingRoomString;
	FString reflectionTimeString = missingRoomString;
	FString dampingString = missingRoomString;

	if (AutoAssignAuxBus)
	{
		decayString = FText::AsNumber(EnvironmentDecayEstimate, &NumberFormat).ToString() + " seconds";
	}

	if (Parent.IsValid() && AkComponentHelpers::GetChildComponentOfType<UAkRoomComponent>(*Parent.Get()))
	{
		decayString = ReverbDescriptor.ShouldEstimateDecay() ? FText::AsNumber(EnvironmentDecayEstimate, &NumberFormat).ToString() + " seconds" : FString("Invalid Late Reverb or Room Primitive Component");

		reflectionTimeString = FString("Invalid Late Reverb or Room Primitive Component");
		if (ReverbDescriptor.ShouldEstimatePredelay())
		{
			reflectionTimeString = FText::AsNumber(TimeToFirstReflection, &NumberFormat).ToString() + " ms";
			if (TimeToFirstReflection > 100.0f)
			{
				reflectionTimeString = FText::AsNumber(TimeToFirstReflection / 1000.0f, &NumberFormat).ToString() + " s";
			}
		}

		dampingString = "No associated geometry component.";
		if (TextureSetComponent.IsValid())
		{
			dampingString = ReverbDescriptor.ShouldEstimateDamping() ? FText::AsNumber(HFDamping, &NumberFormat).ToString() : FString("Invalid Late Reverb or Room Primitive Component");
		}
	}

	const UAkSettings* AkSettings = GetDefault<UAkSettings>();
	if (AkSettings != nullptr)
	{
		if (!AkSettings->DecayRTPCInUse())
		{
			decayString += FString(" (RTPC not in use)");
		}

		if (!AkSettings->PredelayRTPCInUse())
		{
			reflectionTimeString += FString(" (RTPC not in use)");
		}

		if (!AkSettings->DampingRTPCInUse())
		{
			dampingString += FString(" (RTPC not in use)");
		}
	}

	return FText::FromString(FString::Format(TEXT("{1} cubic meters{0}{2} square meters{0}{3}{0}{4}{0}{5}{0}{6}"),
		{ LINE_TERMINATOR, volumeString, areaString, decayString, BusName, reflectionTimeString, dampingString }));
}

void UAkLateReverbComponent::InitTextVisualizers()
{
	if (!HasAnyFlags(RF_Transient))
	{
		FString TextVizName = GetOwner()->GetActorLabel() + GetName();
		UMaterialInterface* mat = Cast<UMaterialInterface>(FAkAudioStyle::GetAkForegroundTextMaterial());

		UWorld* World = GetWorld();

		TextVisualizerLabels = NewObject<UTextRenderComponent>(GetOuter(), *(TextVizName + TEXT("TextLabels")));
		if (mat != nullptr)
		{
			TextVisualizerLabels->SetTextMaterial(mat);
		}
		TextVisualizerLabels->SetHorizontalAlignment(EHorizTextAligment::EHTA_Right);
		TextVisualizerLabels->RegisterComponentWithWorld(World);
		TextVisualizerLabels->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
		TextVisualizerLabels->ResetRelativeTransform();
		TextVisualizerLabels->SetWorldScale3D(FVector::OneVector);
		TextVisualizerLabels->SetWorldLocation(GetTextVisualizersLocation());
		if (World != nullptr && World->WorldType == EWorldType::EditorPreview)
		{
			TextVisualizerLabels->SetWorldRotation(FVector(100, 0, 0).Rotation());
		}
		TextVisualizerLabels->bIsEditorOnly = true;
		// Creates a right-aligned block of text showing the property labels.
		TextVisualizerLabels->SetText(FText::FromString(FString::Format(TEXT("Volume {0}Area {0}Decay {0}AuxBus {0}Time to first reflection {0}HFDamping "), { LINE_TERMINATOR })));
		TextVisualizerLabels->bSelectable = false;
		TextVisualizerLabels->SetVisibility(false);

		TextVisualizerValues = NewObject<UTextRenderComponent>(GetOuter(), *(TextVizName + TEXT("TextValues")));
		if (mat != nullptr)
		{
			TextVisualizerValues->SetTextMaterial(mat);
		}
		TextVisualizerValues->SetHorizontalAlignment(EHorizTextAligment::EHTA_Left);
		TextVisualizerValues->RegisterComponentWithWorld(World);
		TextVisualizerValues->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
		TextVisualizerValues->ResetRelativeTransform();
		TextVisualizerValues->SetWorldScale3D(FVector::OneVector);
		TextVisualizerValues->SetWorldLocation(GetTextVisualizersLocation());
		if (World != nullptr && World->WorldType == EWorldType::EditorPreview)
		{
			TextVisualizerValues->SetWorldRotation(FVector(100, 0, 0).Rotation());
		}
		TextVisualizerValues->bIsEditorOnly = true;
		TextVisualizerValues->bSelectable = false;
		TextVisualizerValues->SetVisibility(false);
	}
}

bool UAkLateReverbComponent::IsTextVisualizersInitialized() const
{
	return TextVisualizerLabels && TextVisualizerValues;
}

void UAkLateReverbComponent::DestroyTextVisualizers()
{
	if (TextVisualizerLabels)
	{
		TextVisualizerLabels->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, /*bCallModify=*/ false));
		TextVisualizerLabels->DestroyComponent();
		TextVisualizerLabels = nullptr;
	}
	if (TextVisualizerValues)
	{
		TextVisualizerValues->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, /*bCallModify=*/ false));
		TextVisualizerValues->DestroyComponent();
		TextVisualizerValues = nullptr;
	}
}

void UAkLateReverbComponent::UpdateTextValues()
{
	if (!GetDefault<UAkSettingsPerUser>()->bShowReverbInfo) return;
	if (!IsTextVisualizersInitialized()) return;

	if (TextVisualizerValues)
	{
		TextVisualizerValues->SetText(GetTextValues());
	}
}
#endif // WITH_EDITOR

void UAkLateReverbComponent::UpdateRTPCs(const UAkRoomComponent* room) const
{
	// The global RTPCs are set on the room ids.
	ReverbDescriptor.UpdateAllRTPCs(room);
}

void UAkLateReverbComponent::AssociateAkTextureSetComponent(UAkAcousticTextureSetComponent* textureSetComponent)
{
	if (TextureSetComponent.IsValid())
		TextureSetComponent->SetReverbDescriptor(nullptr);
	TextureSetComponent = textureSetComponent;
	TextureSetComponent->SetReverbDescriptor(&ReverbDescriptor);
}

TWeakObjectPtr<UAkAcousticTextureSetComponent> UAkLateReverbComponent::GetAttachedTextureSetComponent()
{
	return TextureSetComponent;
}


void UAkLateReverbComponent::UpdateDecayEstimation(float decay, float volume, float surfaceArea)
{
	if (AutoAssignAuxBus)
	{
		UAkSettings* AkSettings = GetMutableDefault<UAkSettings>();
		if (AkSettings != nullptr)
		{
			auto newAuxBus = AkSettings->GetAuxBusForDecayValue(decay);
			if (AuxBus != newAuxBus)
			{
				AuxBus = newAuxBus;
				ReverbParamsChanged = true;
			}
		}
	}

#if WITH_EDITOR
	EnvironmentVolume = volume;
	SurfaceArea = surfaceArea;
	EnvironmentDecayEstimate = decay;
#endif // WITH_EDITOR
}

#if WITH_EDITOR
void UAkLateReverbComponent::UpdateHFDampingEstimation(float hfDamping)
{
	HFDamping = hfDamping;
}

void UAkLateReverbComponent::UpdatePredelayEstimation(float predelay)
{
	TimeToFirstReflection = predelay;
}

void UAkLateReverbComponent::PreEditChange(FProperty* PropertyAboutToChange)
{
	if (PropertyAboutToChange != nullptr)
	{
		if (!AutoAssignAuxBus
			&& (PropertyAboutToChange->NamePrivate == GET_MEMBER_NAME_CHECKED(UAkLateReverbComponent, AutoAssignAuxBus)
				|| PropertyAboutToChange->NamePrivate == GET_MEMBER_NAME_CHECKED(UAkLateReverbComponent, AuxBus)))
		{
			AuxBusManual = AuxBus;
		}
		if (AutoAssignAuxBus
			&& PropertyAboutToChange->NamePrivate == GET_MEMBER_NAME_CHECKED(UAkLateReverbComponent, AutoAssignAuxBus))
		{
			AuxBus = AuxBusManual;
		}
	}

	Super::PreEditChange(PropertyAboutToChange);
}

void UAkLateReverbComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkLateReverbComponent, AutoAssignAuxBus))
	{
		DecayEstimationNeedsUpdate = true;

		if (!AutoAssignAuxBus)
		{
			AuxBus = AuxBusManual;
			ReverbParamsChanged = true;
		}
	}
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkLateReverbComponent, AuxBus))
	{
		if (!AutoAssignAuxBus)
		{
			AuxBusManual = AuxBus;
		}
		ReverbParamsChanged = true;
	}
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkLateReverbComponent, bEnable))
	{
		if (AkComponentHelpers::IsInGameWorld(this))
		{
			UAkRoomComponent* RoomCmpt = nullptr;
			if (Parent.IsValid())
			{
				RoomCmpt = AkComponentHelpers::GetChildComponentOfType<UAkRoomComponent>(*Parent.Get());
			}

			if (!RoomCmpt || !RoomCmpt->RoomIsActive())
			{
				// No room, or inactive room. Update the late reverb in the oct tree.
				FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
				if (AkAudioDevice)
				{
					if (!bEnable && IsIndexed)
					{
						AkAudioDevice->UnindexLateReverb(this);
					}
					else if (bEnable && !IsIndexed)
					{
						AkAudioDevice->IndexLateReverb(this);
					}
				}
			}
			else if (RoomCmpt && RoomCmpt->RoomIsActive())
			{
				// Late reverb is inside an active room. Update the room such that the reverb aux bus is correctly updated.
				RoomCmpt->UpdateSpatialAudioRoom();
			}
		}
	}
	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkLateReverbComponent, SendLevel))
	{
		ReverbParamsChanged = true;
	}
}

void UAkLateReverbComponent::OnAttachmentChanged()
{
	Super::OnAttachmentChanged();
	// In other cases of CreationMethod, OnRegister() gets called to initialize the parent
	if (CreationMethod == EComponentCreationMethod::Instance)
	{
		// OnAttachmentChanged can be called when this component is getting detached from its parent
		// in this case InitializeParent will disable the component and set the parent and the ReverbDescriptor's primitive to null
		InitializeParent();
		ParentChanged();
	}
}

FVector UAkLateReverbComponent::GetTextVisualizersLocation()
{
	if (!Parent.IsValid())
		return FVector();

	FBoxSphereBounds bounds = Parent->CalcBounds(Parent->GetComponentTransform());
	return Parent->GetComponentLocation() + FVector(0.0f, 0.0f, bounds.BoxExtent.Z + TextVisualizerHeightOffset);
}

void UAkLateReverbComponent::RegisterReverbAssignmentChangedCallback()
{
	UAkSettings* AkSettings = GetMutableDefault<UAkSettings>();
	if (AkSettings != nullptr)
	{
		if (ReverbAssignmentChangedHandle.IsValid())
		{
			AkSettings->OnReverbAssignmentChanged.Remove(ReverbAssignmentChangedHandle);
		}
		ReverbAssignmentChangedHandle = AkSettings->OnReverbAssignmentChanged.AddLambda([this]()
		{
			ReverbAssignmentNeedsUpdate = true;
		});
	}
}

void UAkLateReverbComponent::RegisterGlobalDecayAbsorptionChangedCallback()
{
	UAkSettings* AkSettings = GetMutableDefault<UAkSettings>();
	if (AkSettings != nullptr)
	{
		if (GlobalDecayAbsorptionChangedHandle.IsValid())
		{
			AkSettings->OnGlobalDecayAbsorptionChanged.Remove(GlobalDecayAbsorptionChangedHandle);
		}
		GlobalDecayAbsorptionChangedHandle = AkSettings->OnGlobalDecayAbsorptionChanged.AddLambda([this]()
		{
			DecayEstimationNeedsUpdate = true;
		});
	}
}

void UAkLateReverbComponent::RegisterReverbRTPCChangedCallback()
{
	UAkSettings* AkSettings = GetMutableDefault<UAkSettings>();
	if (AkSettings != nullptr)
	{
		if (RTPCChangedHandle.IsValid())
		{
			AkSettings->OnReverbRTPCChanged.Remove(RTPCChangedHandle);
		}
		RTPCChangedHandle = AkSettings->OnReverbRTPCChanged.AddLambda([this]()
		{
			DecayEstimationNeedsUpdate = true;
			PredelayEstimationNeedsUpdate = true;
		});
	}
}
#endif

bool UAkLateReverbComponent::EncompassesPoint(FVector Point, float SphereRadius/*=0.f*/, float* OutDistanceToPoint) const
{
	if (Parent.IsValid())
	{
		return AkComponentHelpers::EncompassesPoint(*Parent.Get(), Point, SphereRadius, OutDistanceToPoint);
	}
	FString actorString = FString("NONE");
	if (GetOwner() != nullptr)
	{
#if WITH_EDITOR
		actorString = GetOwner()->GetActorLabel();
#else
		actorString = GetOwner()->GetName();
#endif
	}
	UE_LOG(LogAkAudio, Error, TEXT("UAkLateReverbComponent::EncompassesPoint : Error. In actor %s, AkLateReverbComponent %s has an invalid Parent."), *actorString, *GetName());
	return false;
}

void UAkLateReverbComponent::OnReverbParamsChanged()
{
	if (AkComponentHelpers::IsInGameWorld(this))
	{
		UAkRoomComponent* RoomCmpt = nullptr;
		if (Parent.IsValid())
		{
			RoomCmpt = AkComponentHelpers::GetChildComponentOfType<UAkRoomComponent>(*Parent.Get());
		}
		if (!RoomCmpt || !RoomCmpt->RoomIsActive())
		{
			// No room, or inactive room. Update the late reverb in the oct tree.
			FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
			if (AkAudioDevice && bEnable && IsIndexed)
			{
				AkAudioDevice->ReindexLateReverb(this);
			}
		}
		else if (RoomCmpt && RoomCmpt->RoomIsActive())
		{
			// Late reverb is inside an active room. Update the room such that the reverb aux bus is correctly updated.
			RoomCmpt->UpdateSpatialAudioRoom();
		}
	}
}

