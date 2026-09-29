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

#include "AkAudioStyle.h"

#include "AkAcousticTexture.h"
#include "AkAudioDevice.h"
#include "AkAudioEvent.h"
#include "AkAuxBus.h"
#include "AkEffectShareSet.h"
#include "AkRtpc.h"
#include "AkStateValue.h"
#include "AkSwitchValue.h"
#include "AkTrigger.h"

#include "Engine/Texture2D.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Notifications/SNotificationList.h"
#if WITH_EDITOR
#include "Wwise/WwisePluginStyle.h"
#include "Wwise/Ref/WwiseRefType.h"
#endif

TSharedPtr< FSlateStyleSet > FAkAudioStyle::StyleInstance = nullptr;
UMaterial* FAkAudioStyle::TextMaterial = nullptr;

/* Taken from the Wwise Authoring dark theme. @TODO: Take this from the theme's main.json file. */
TArray<FLinearColor> WwiseUnrealColorPalette = {
	FColor(191, 191, 191, 255),
	FColor(82, 90, 255, 1),
	FColor(41, 169, 255, 1),
	FColor(0, 255, 255, 1),
	FColor(0, 255, 0, 1),
	FColor(183, 255, 0, 1),
	FColor(255, 238, 0, 1),
	FColor(255, 170, 0, 1),
	FColor(255, 128, 0, 1),
	FColor(255, 85, 0, 1),
	FColor(255, 0, 0, 1),
	FColor(255, 0, 255, 1),
	FColor(190, 61, 255, 1),
	FColor(184, 107, 255, 1),
	FColor(112, 119, 255, 1),
	FColor(119, 164, 253, 1),
	FColor(132, 240, 240, 1),
	FColor(136, 242, 136, 1),
	FColor(209, 240, 122, 1),
	FColor(240, 192, 96, 1),
	FColor(255, 179, 102, 1),
	FColor(240, 192, 96, 1),
	FColor(255, 179, 102, 1),
	FColor(255, 146, 92, 1),
	FColor(255, 112, 255, 1),
	FColor(214, 133, 255, 1),
	FColor(166, 128, 255, 1),
	FColor(191, 191, 191, 1)
};

namespace AkAudioStyle_Helpers
{
	template<typename T1, typename T2>
	auto LoadAkTexture(T1&& RelativePath, T2&& TextureName)
	{
		return LoadObject<UTexture2D>(nullptr, *(FString("/Wwise/") / FString(Forward<T1>(RelativePath)) / FString(Forward<T2>(TextureName)) + FString(".") + FString(Forward<T2>(TextureName))));
	}

	UMaterial* LoadAkMaterial(const FString& RelativePath, const FString& MaterialName)
	{
		return LoadObject<UMaterial>(nullptr, *(FString("/Wwise/") + RelativePath + MaterialName + FString(".") + MaterialName));
	}

	auto CreateAkImageBrush(UTexture2D* Texture, const FVector2D& TextureSize)
	{
		return new FSlateImageBrush(Texture, TextureSize);
	}

	template<typename StringType, typename...Args>
	auto CreateEngineBoxBrush(FSlateStyleSet& Style, StringType& RelativePath, Args&&... args)
	{
		return new FSlateBoxBrush(Style.RootToContentDir(Forward<StringType>(RelativePath), TEXT(".png")), Forward<Args>(args)...);
	}

	template<typename StringType, typename...Args>
	auto EngineBoxBrush(FSlateStyleSet& Style, StringType&& RelativePath, Args&&... args)
	{
		return FSlateBoxBrush(Style.RootToContentDir(Forward<StringType>(RelativePath), TEXT(".png")), Forward<Args>(args)...);
	}

	template<typename StringType, typename...Args>
	auto CreateEngineImageBrush(FSlateStyleSet& Style, StringType& RelativePath, Args&&... args)
	{
		return new FSlateImageBrush(Style.RootToContentDir(Forward<StringType>(RelativePath), TEXT(".png")), Forward<Args>(args)...);
	}
}

void FAkAudioStyle::Initialize()
{
	using namespace AkAudioStyle_Helpers;

	if (!StyleInstance.IsValid())
	{
		StyleInstance = MakeShareable(new FSlateStyleSet(FAkAudioStyle::GetStyleSetName()));
		auto ContentRoot = FPaths::EngineContentDir() / TEXT("Slate");
		StyleInstance->SetContentRoot(ContentRoot);
		StyleInstance->SetCoreContentRoot(ContentRoot);

		FSlateStyleSet& Style = *StyleInstance.Get();
		{
			Style.Set("AudiokineticTools.GroupBorder", CreateEngineBoxBrush(Style, "Common/GroupBorder", FMargin(4.0f / 16.0f)));
			Style.Set("AudiokineticTools.AssetDragDropTooltipBackground", CreateEngineBoxBrush(Style, "Old/Menu_Background", FMargin(8.0f / 64.0f)));

			FButtonStyle HoverHintOnly = FButtonStyle()
				.SetNormal(FSlateNoResource())
				.SetHovered(EngineBoxBrush(Style, "Common/Button_Hovered", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 0.15f)))
				.SetPressed(EngineBoxBrush(Style, "Common/Button_Pressed", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 0.25f)))
				.SetNormalPadding(FMargin(0, 0, 0, 1))
				.SetPressedPadding(FMargin(0, 1, 0, 0));
			Style.Set("AudiokineticTools.HoverHintOnly", HoverHintOnly);

			Style.Set("AudiokineticTools.SourceTitleFont", FCoreStyle::GetDefaultFontStyle("Regular", 12));
			Style.Set("AudiokineticTools.SourceTreeItemFont", FCoreStyle::GetDefaultFontStyle("Regular", 10));
			Style.Set("AudiokineticTools.SourceTreeRootItemFont", FCoreStyle::GetDefaultFontStyle("Regular", 12));

			const FVector2D Icon12x12(12.0f, 12.0f);
			Style.Set("AudiokineticTools.Button_EllipsisIcon", CreateEngineImageBrush(Style, "Icons/ellipsis_12x", Icon12x12));
		}

		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}
#undef IMAGE_BRUSH

void FAkAudioStyle::Shutdown()
{
    if (StyleInstance.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
        ensure(StyleInstance.IsUnique());
        StyleInstance.Reset();
    }
}

FName FAkAudioStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("AudiokineticToolsStyle"));
	return StyleSetName;
}

const ISlateStyle& FAkAudioStyle::Get()
{
	Initialize();
	return *StyleInstance;
}


#if WITH_EDITOR
void FAkAudioStyle::DisplayEditorMessage(const FText& messageText, EWwiseItemType::Type wwiseItemType /* = EWwiseItemType::Type::None*/, float duration /* = 1.5f */)
{
	if (!FApp::CanEverRender())
	{
		UE_LOG(LogAkAudio, Display, TEXT("DisplayEditorMessage: %s"), *messageText.ToString());
		return;
	}
	FNotificationInfo Info(messageText);
	if (wwiseItemType == EWwiseItemType::None)
		Info.Image = FAkAudioStyle::GetWwiseIcon();
	else
		Info.Image = FAkAudioStyle::GetBrush(wwiseItemType);
	Info.FadeInDuration = 0.1f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = duration;
	Info.bUseThrobber = false;
	Info.bUseSuccessFailIcons = false;
	Info.bUseLargeFont = true;
	Info.bFireAndForget = false;
	Info.bAllowThrottleWhenFrameRateIsLow = false;
	auto NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationItem->SetCompletionState(SNotificationItem::CS_None);
	NotificationItem->ExpireAndFadeout();
}

const FSlateBrush* FAkAudioStyle::GetWwiseIcon()
{
	auto Style = FWwisePluginStyle::Get();
	if (!Style.IsValid())
	{
		return nullptr;
	}
	return Style->GetBrush(FWwisePluginStyle::WwiseIconName);
}

const FSlateBrush* FAkAudioStyle::GetBrush(EWwiseItemType::Type ItemType)
{
	auto Style = FWwisePluginStyle::Get();
	if (!Style.IsValid())
	{
		return nullptr;
	}
	
	switch (ItemType)
	{
	case EWwiseItemType::Event: return Style->GetBrush(FWwisePluginStyle::EventIconName);
	case EWwiseItemType::AcousticTexture: return Style->GetBrush(FWwisePluginStyle::AcousticTextureIconName);
	case EWwiseItemType::AuxBus: return Style->GetBrush(FWwisePluginStyle::AuxBusIconName);
	case EWwiseItemType::Bus: return Style->GetBrush(FWwisePluginStyle::BusIconName);
	case EWwiseItemType::Folder: return Style->GetBrush(FWwisePluginStyle::FolderIconName);
	case EWwiseItemType::Project: return Style->GetBrush(FWwisePluginStyle::ProjectIconName);
	case EWwiseItemType::PhysicalFolder: return Style->GetBrush(FWwisePluginStyle::PhysicalFolderIconName);
	case EWwiseItemType::StandaloneWorkUnit:
	case EWwiseItemType::NestedWorkUnit: return Style->GetBrush(FWwisePluginStyle::WorkUnitIconName);
	case EWwiseItemType::ActorMixer: return Style->GetBrush(FWwisePluginStyle::ActorMixerIconName);
	case EWwiseItemType::Sound: return Style->GetBrush(FWwisePluginStyle::SoundIconName);
	case EWwiseItemType::SwitchContainer: return Style->GetBrush(FWwisePluginStyle::SwitchContainerIconName);
	case EWwiseItemType::RandomSequenceContainer: return Style->GetBrush(FWwisePluginStyle::RandomSequenceContainerIconName);
	case EWwiseItemType::BlendContainer: return Style->GetBrush(FWwisePluginStyle::BlendContainerIconName);
	case EWwiseItemType::MotionBus: return Style->GetBrush(FWwisePluginStyle::MotionBusIconName);
	case EWwiseItemType::GameParameter: return Style->GetBrush(FWwisePluginStyle::GameParameterIconName);
	case EWwiseItemType::State: return Style->GetBrush(FWwisePluginStyle::StateIconName);
	case EWwiseItemType::StateGroup: return Style->GetBrush(FWwisePluginStyle::StateGroupIconName);
	case EWwiseItemType::Switch: return Style->GetBrush(FWwisePluginStyle::SwitchIconName);
	case EWwiseItemType::SwitchGroup: return Style->GetBrush(FWwisePluginStyle::SwitchGroupIconName);
	case EWwiseItemType::Trigger: return Style->GetBrush(FWwisePluginStyle::TriggerIconName);
	case EWwiseItemType::EffectShareSet: return Style->GetBrush(FWwisePluginStyle::EffectShareSetIconName);
	case EWwiseItemType::AudioDeviceShareSet: return Style->GetBrush(FWwisePluginStyle::AudioDeviceIconName);

	default:
		return nullptr;
	}
}

const FSlateBrush* FAkAudioStyle::GetBrush(WwiseRefType WwiseRefType)
{
	EWwiseItemType::Type ItemType = EWwiseItemType::Type::None;
	switch (WwiseRefType)
	{
	case WwiseRefType::Event:
		ItemType = EWwiseItemType::Event;
		break;
	case WwiseRefType::SwitchContainer:
		ItemType = EWwiseItemType::SwitchContainer;
		break;
	case WwiseRefType::Bus:		
		ItemType = EWwiseItemType::Bus;
		break;
	case WwiseRefType::AuxBus:
		ItemType = EWwiseItemType::AuxBus;
		break;
	case WwiseRefType::GameParameter:
		ItemType = EWwiseItemType::GameParameter;
		break;
	case WwiseRefType::StateGroup:
		ItemType = EWwiseItemType::StateGroup;
		break;
	case WwiseRefType::State:
		ItemType = EWwiseItemType::State;
		break;
	case WwiseRefType::SwitchGroup:
		ItemType = EWwiseItemType::SwitchGroup;
		break;
	case WwiseRefType::Switch:
		ItemType = EWwiseItemType::Switch;
		break;
	case WwiseRefType::Trigger:
		ItemType = EWwiseItemType::Trigger;
		break;
	case WwiseRefType::AcousticTexture:
		ItemType = EWwiseItemType::AcousticTexture;
		break;
	case WwiseRefType::PluginShareSet:
		ItemType = EWwiseItemType::EffectShareSet;
		break;
	case WwiseRefType::AudioDevice:
		ItemType = EWwiseItemType::AudioDeviceShareSet;
		break;
	}
	return GetBrush(ItemType);
}

const FSlateBrush* FAkAudioStyle::GetBrush(UClass* Class)
{
	EWwiseItemType::Type ItemType = EWwiseItemType::Type::None;
	if (Class == UAkAudioEvent::StaticClass())
	{
		ItemType = EWwiseItemType::Event;
	}
	if (Class == UAkAcousticTexture::StaticClass())
	{
		ItemType = EWwiseItemType::AcousticTexture;
	}
	if (Class == UAkRtpc::StaticClass())
	{
		ItemType = EWwiseItemType::GameParameter;
	}
	if (Class == UAkStateValue::StaticClass())
	{
		ItemType = EWwiseItemType::State;
	}
	if (Class == UAkSwitchValue::StaticClass())
	{
		ItemType = EWwiseItemType::Switch;
	}
	if (Class == UAkTrigger::StaticClass())
	{
		ItemType = EWwiseItemType::Trigger;
	}
	if (Class == UAkEffectShareSet::StaticClass())
	{
		ItemType = EWwiseItemType::EffectShareSet;
	}
	if (Class == UAkAuxBus::StaticClass())
	{
		ItemType = EWwiseItemType::AuxBus;
	}
	if(Class == UAkAudioDeviceShareSet::StaticClass())
	{
		ItemType = EWwiseItemType::AudioDeviceShareSet;
	}
	return GetBrush(ItemType);
}
#endif

const FSlateBrush* FAkAudioStyle::GetBrush(FName PropertyName, const ANSICHAR* Specifier)
{
	return Get().GetBrush(PropertyName, Specifier);
}

const FSlateFontInfo FAkAudioStyle::GetFontStyle(FName PropertyName, const ANSICHAR* Specifier)
{
	return Get().GetFontStyle(PropertyName, Specifier);
}

UMaterial* FAkAudioStyle::GetAkForegroundTextMaterial()
{
	if (TextMaterial == nullptr)
	{
		TextMaterial = AkAudioStyle_Helpers::LoadAkMaterial(FString(""), FString("DefaultForegroundTextMaterial"));
		if (TextMaterial != nullptr)
			TextMaterial->AddToRoot();
	}

	return TextMaterial;
}

FLinearColor FAkAudioStyle::GetWwiseObjectColor(int colorIndex)
{
	if (WwiseUnrealColorPalette.Num() > 0)
	{
		if (colorIndex == -1)
			colorIndex = WwiseUnrealColorPalette.Num() - 1;
		if (colorIndex < WwiseUnrealColorPalette.Num())
		{
			return WwiseUnrealColorPalette[colorIndex];
		}
	}
	return FLinearColor();
}