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

#include "Wwise/WwisePluginStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateStyleRegistry.h"
#include "Wwise/WwiseEditorUtilsModule.h"

TSharedPtr< FWwisePluginStyle > FWwisePluginStyle::StyleInstance;
const FName FWwisePluginStyle::AcousticTextureIconName{ TEXT("Wwise.AcousticTextureIcon") };
const FName FWwisePluginStyle::ActorMixerIconName{ TEXT("Wwise.ActorMixerIcon") };
const FName FWwisePluginStyle::AssetLibraryIconName{ TEXT("Wwise.AssetLibraryIcon") };
const FName FWwisePluginStyle::AssetLibraryFilterIconName{ TEXT("Wwise.AssetLibraryFilterIcon") };
const FName FWwisePluginStyle::AudioDeviceIconName{ TEXT("Wwise.AudioDeviceIcon") };
const FName FWwisePluginStyle::AuxBusIconName{ TEXT("Wwise.AuxBusIcon") };
const FName FWwisePluginStyle::BlendContainerIconName{ TEXT("Wwise.BlendContainerIcon") };
const FName FWwisePluginStyle::BusIconName{ TEXT("Wwise.BusIcon") };
const FName FWwisePluginStyle::EffectShareSetIconName{ TEXT("Wwise.EffectShareSetIcon") };
const FName FWwisePluginStyle::EventIconName{ TEXT("Wwise.EventIcon") };
const FName FWwisePluginStyle::ExternalSourceIconName{ TEXT("Wwise.ExternalSourceIcon") };
const FName FWwisePluginStyle::FolderIconName{ TEXT("Wwise.FolderIcon") };
const FName FWwisePluginStyle::GameParameterIconName{ TEXT("Wwise.GameParameterIcon") };
const FName FWwisePluginStyle::InitBankIconName{ TEXT("Wwise.InitBankIcon") };
const FName FWwisePluginStyle::MediaIconName{ TEXT("Wwise.MediaIcon") };
const FName FWwisePluginStyle::MotionBusIconName{ TEXT("Wwise.MotionBusIcon") };
const FName FWwisePluginStyle::PhysicalFolderIconName{ TEXT("Wwise.PhysicalFolderIcon") };
const FName FWwisePluginStyle::ProjectIconName{ TEXT("Wwise.ProjectIcon") };
const FName FWwisePluginStyle::RandomSequenceContainerIconName{ TEXT("Wwise.RandomSequenceContainerIcon") };
const FName FWwisePluginStyle::StateGroupIconName{ TEXT("Wwise.StateGroupIcon") };
const FName FWwisePluginStyle::StateIconName{ TEXT("Wwise.StateIcon") };
const FName FWwisePluginStyle::SoundBankIconName{ TEXT("Wwise.SoundBankIcon") };
const FName FWwisePluginStyle::SoundIconName{ TEXT("Wwise.SoundIcon") };
const FName FWwisePluginStyle::SwitchContainerIconName{ TEXT("Wwise.SwitchContainerIcon") };
const FName FWwisePluginStyle::SwitchGroupIconName{ TEXT("Wwise.SwitchGroupIcon") };
const FName FWwisePluginStyle::SwitchIconName{ TEXT("Wwise.SwitchIcon") };
const FName FWwisePluginStyle::TriggerIconName{ TEXT("Wwise.TriggerIcon") };
const FName FWwisePluginStyle::WorkUnitIconName{ TEXT("Wwise.WorkUnitIcon") };
const FName FWwisePluginStyle::WwiseBrowserTabIconName{ TEXT("Wwise.WwiseBrowserTabIcon") };
const FName FWwisePluginStyle::WwiseIconName{ TEXT("Wwise.WwiseIcon") };

FWwisePluginStyle::FWwisePluginStyle()
	: FSlateStyleSet("WwisePluginStyle")
{
}

void FWwisePluginStyle::SetV(
	const FName& BrushName,
	const FString& TextureName,
	const FVector2D& Size)
{
	static const auto ContentDir{ IPluginManager::Get().FindPlugin(TEXT("Wwise"))->GetContentDir() / TEXT("Editor/Slate/Wwise") };
	static const FString Extension{ TEXT(".svg") };

	const FString TexturePath{ ContentDir / TextureName + Extension };
	Set(BrushName, new FSlateVectorImageBrush{TexturePath, Size});
}

void FWwisePluginStyle::SetI(
	const FName& BrushName,
	const FString& TextureName,
	const FVector2D& Size)
{
	static const auto ContentDir{ IPluginManager::Get().FindPlugin(TEXT("Wwise"))->GetContentDir() / TEXT("Editor/Slate/Wwise") };
	static const FString Extension{ TEXT(".png") };

	const FString TexturePath{ ContentDir / TextureName + Extension };
	Set(BrushName, new FSlateImageBrush{TexturePath, Size});
}

void FWwisePluginStyle::Initialize()
{
	static const FVector2D Icon(16.0f, 16.0f);
	static const FVector2D Thumbnail(256.0f, 256.0f);

	SetV(AcousticTextureIconName, "ObjectIcons_AcousticTexture_nor", Icon);
	SetV(ActorMixerIconName, "ObjectIcons_ActorMixer_nor", Icon);
	SetV(AssetLibraryIconName, "ObjectIcons_Asset_Library", Icon);
	SetV(AssetLibraryFilterIconName, "ObjectIcons_Asset_Library_Filter", Icon);
	SetV(AudioDeviceIconName, "ObjectIcons_AudioDevicePlugin_nor", Icon);
	SetV(AuxBusIconName, "ObjectIcons_AuxBus_nor", Icon);
	SetV(BlendContainerIconName, "ObjectIcons_BlendContainer_nor", Icon);
	SetV(BusIconName, "ObjectIcons_Bus_nor", Icon);
	SetV(EffectShareSetIconName, "ObjectIcons_EffectPlugin_nor", Icon);
	SetV(EventIconName, "ObjectIcons_Event_nor", Icon);
	SetV(ExternalSourceIconName, "ObjectIcons_ExternalSource_nor", Icon);
	SetV(FolderIconName, "ObjectIcons_Folder_nor", Icon);
	SetV(GameParameterIconName, "ObjectIcons_GameParameter_nor", Icon);
	SetV(InitBankIconName, "ObjectIcons_SoundbankInit_nor", Icon);
	SetV(MediaIconName, "ObjectIcons_MediaPackage_nor", Icon);
	SetV(MotionBusIconName, "ObjectIcons_AuxBusMix_nor", Icon);
	SetV(PhysicalFolderIconName, "ObjectIcons_PhysicalFolder_nor", Icon);
	SetV(ProjectIconName, "ObjectIcons_Project_nor", Icon);
	SetV(RandomSequenceContainerIconName, "ObjectIcons_RandomSequenceContainer_nor", Icon);
	SetV(StateGroupIconName, "ObjectIcons_StateGroup_nor", Icon);
	SetV(StateIconName, "ObjectIcons_State_nor", Icon);
	SetV(SoundBankIconName, "ObjectIcons_Soundbank_nor", Icon);
	SetV(SoundIconName, "ObjectIcons_SoundFX_nor", Icon);
	SetV(SwitchContainerIconName, "ObjectIcons_SwitchContainer_nor", Icon);
	SetV(SwitchGroupIconName, "ObjectIcons_StateGroup_nor", Icon);
	SetV(SwitchIconName, "ObjectIcons_Switch_nor", Icon);
	SetV(TriggerIconName, "ObjectIcons_Trigger_nor", Icon);
	SetV(WorkUnitIconName, "ObjectIcons_Workunit_nor", Icon);
	SetV(WwiseBrowserTabIconName, "Wwise_Dark_TransparentBG", Icon);
	SetV(WwiseIconName, "Wwise_Dark_TransparentBG", Icon);

	SetV("ClassIcon.AkAcousticPortal", "Integration_AcousticPortal_Icon", Icon);
	SetV("ClassIcon.AkAmbientSound", "Integration_Object_Icon", Icon);
	SetV("ClassIcon.AkReverbVolume", "Integration_ReverbVolume_Icon", Icon);
	SetV("ClassIcon.AkSpatialAudioVolume", "Integration_SpatialAudioVolume_Icon", Icon);
	SetV("ClassIcon.AkSpotReflector", "Integration_SpotReflector_Icon", Icon);
	
	SetV("ClassThumbnail.AkAcousticPortal", "Integration_AcousticPortal", Thumbnail);
	SetV("ClassThumbnail.AkAmbientSound", "Integration_Object", Thumbnail);
	SetV("ClassThumbnail.AkAcousticTexture", "ObjectIcons_AcousticTexture_nor", Thumbnail);
	SetV("ClassThumbnail.AkAudioDeviceShareSet", "ObjectIcons_AudioDevicePlugin_nor", Thumbnail);
	SetV("ClassThumbnail.AkAudioEvent", "ObjectIcons_Event_nor", Thumbnail);
	SetV("ClassThumbnail.AkAuxBus", "ObjectIcons_AuxBus_nor", Thumbnail);
	SetV("ClassThumbnail.AkEffectShareSet", "ObjectIcons_EffectPlugin_nor", Thumbnail);
	SetV("ClassThumbnail.AkInitBank", "ObjectIcons_SoundbankInit_nor", Thumbnail);
	SetV("ClassThumbnail.AkReverbVolume", "Integration_ReverbVolume", Thumbnail);
	SetV("ClassThumbnail.AkRtpc", "ObjectIcons_GameParameter_nor", Thumbnail);
	SetV("ClassThumbnail.AkSpatialAudioVolume", "Integration_SpatialAudioVolume", Thumbnail);
	SetV("ClassThumbnail.AkSpotReflector", "Integration_SpotReflector", Thumbnail);
	SetV("ClassThumbnail.AkStateValue", "ObjectIcons_State_nor", Thumbnail);
	SetV("ClassThumbnail.AkSwitchValue", "ObjectIcons_Switch_nor", Thumbnail);
	SetV("ClassThumbnail.AkTrigger", "ObjectIcons_Trigger_nor", Thumbnail);
	SetV("ClassThumbnail.WwiseAssetLibrary", "ObjectIcons_Asset_Library", Thumbnail);
	SetV("ClassThumbnail.WwiseSharedAssetLibraryFilter", "ObjectIcons_Asset_Library_Filter", Thumbnail);

	FSlateStyleRegistry::RegisterSlateStyle(*this);
	SetParentStyleName(FAppStyle::GetAppStyleSetName());
}

void FWwisePluginStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*this);
}

void FWwisePluginStyle::InitializeWwisePluginStyle()
{
	ensure(!StyleInstance.IsValid());
	StyleInstance = MakeShareable(new FWwisePluginStyle);
	StyleInstance->Initialize();
}

void FWwisePluginStyle::ShutdownWwisePluginStyle()
{
	ensure(StyleInstance.IsValid());
	ensure(StyleInstance.IsUnique());
	StyleInstance->Shutdown();
	StyleInstance.Reset();
}

TSharedPtr< FWwisePluginStyle > FWwisePluginStyle::Get()
{
	IWwiseEditorUtilsModule::GetModule();
	return StyleInstance;
}
