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

#include "Wwise/API_Null/WwiseSoundEngineAPI_Null.h"
#include "Wwise/Stats/SoundEngine.h"

#include "AkInclude.h"

FWwiseSoundEngineAPI_Null::FWwiseSoundEngineAPI_Null():
	IWwiseSoundEngineAPI(new FQuery, new FAudioInputPlugin, new FDynamicDialogue, new FDynamicSequence)
{}

bool FWwiseSoundEngineAPI_Null::IsInitialized()
{
	return false;
}

AKRESULT FWwiseSoundEngineAPI_Null::Init(
	AkInitSettings* in_pSettings,
	AkPlatformInitSettings* in_pPlatformSettings
)
{
	return AK_NotImplemented;
}

void FWwiseSoundEngineAPI_Null::GetDefaultInitSettings(
	AkInitSettings& out_settings
)
{
}

void FWwiseSoundEngineAPI_Null::GetDefaultPlatformInitSettings(
	AkPlatformInitSettings& out_platformSettings
)
{
}

void FWwiseSoundEngineAPI_Null::Term()
{
}

AKRESULT FWwiseSoundEngineAPI_Null::GetAudioSettings(
	AkAudioSettings& out_audioSettings
)
{
	return AK_NotImplemented;
}

AkChannelConfig FWwiseSoundEngineAPI_Null::GetSpeakerConfiguration(
	AkOutputDeviceID	in_idOutput
)
{
	return {};
}

AKRESULT FWwiseSoundEngineAPI_Null::GetOutputDeviceConfiguration(
	AkOutputDeviceID in_idOutput,
	AkChannelConfig& io_channelConfig,
	Ak3DAudioSinkCapabilities& io_capabilities
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::GetPanningRule(
	AkPanningRule& out_ePanningRule,
	AkOutputDeviceID	in_idOutput
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetPanningRule(
	AkPanningRule		in_ePanningRule,
	AkOutputDeviceID	in_idOutput
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::GetSpeakerAngles(
	AkReal32* io_pfSpeakerAngles,
	AkUInt32& io_uNumAngles,
	AkReal32& out_fHeightAngle,
	AkOutputDeviceID	in_idOutput
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetSpeakerAngles(
	const AkReal32* in_pfSpeakerAngles,
	AkUInt32			in_uNumAngles,
	AkReal32 			in_fHeightAngle,
	AkOutputDeviceID	in_idOutput
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetVolumeThreshold(
	AkReal32 in_fVolumeThresholdDB
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetMaxNumVoicesLimit(
	AkUInt16 in_maxNumberVoices
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetJobMgrMaxActiveWorkers( 
	AkJobType in_jobType,
	AkUInt32 in_uNewMaxActiveWorkers
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RenderAudio(
	bool in_bAllowSyncRender
)
{
	return AK_NotImplemented;
}

AK::IAkGlobalPluginContext* FWwiseSoundEngineAPI_Null::GetGlobalPluginContext()
{
	return nullptr;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterPlugin(
	AkPluginType in_eType,
	AkUInt32 in_ulCompanyID,
	AkUInt32 in_ulPluginID,
	AkCreatePluginCallback in_pCreateFunc,
	AkCreateParamCallback in_pCreateParamFunc,
	AkGetDeviceListCallback in_pGetDeviceList
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterPluginDLL(
	const AkOSChar* in_DllName,
	const AkOSChar* in_DllPath
)
{
	return AK_NotImplemented;
}

bool FWwiseSoundEngineAPI_Null::IsPluginRegistered(
	AkPluginType in_eType,
	AkUInt32 in_ulCompanyID,
	AkUInt32 in_ulPluginID
	)
{
	return false;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterCodec(
	AkUInt32 in_ulCompanyID,
	AkUInt32 in_ulCodecID,
	AkCreateFileSourceCallback in_pFileCreateFunc,
	AkCreateBankSourceCallback in_pBankCreateFunc
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterGlobalCallback(
	AkGlobalCallbackFunc in_pCallback,
	AkUInt32 in_eLocation,
	void* in_pCookie,
	AkPluginType in_eType,
	AkUInt32 in_ulCompanyID,
	AkUInt32 in_ulPluginID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::UnregisterGlobalCallback(
	AkGlobalCallbackFunc in_pCallback,
	AkUInt32 in_eLocation
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterResourceMonitorCallback(
	AkResourceMonitorCallbackFunc in_pCallback
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::UnregisterResourceMonitorCallback(
	AkResourceMonitorCallbackFunc in_pCallback
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterAudioDeviceStatusCallback(
	AK::AkDeviceStatusCallbackFunc in_pCallback
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::UnregisterAudioDeviceStatusCallback()
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AkUInt32 FWwiseSoundEngineAPI_Null::GetIDFromString(const wchar_t* in_pszString)
{
	return 0;
}
#endif //AK_SUPPORT_WCHAR

AkUInt32 FWwiseSoundEngineAPI_Null::GetIDFromString(const char* in_pszString)
{
	return 0;
}

AkPlayingID FWwiseSoundEngineAPI_Null::PostEvent(
	AkUniqueID in_eventID,
	AkGameObjectID in_gameObjectID,
	AkUInt32 in_uFlags,
	AkCallbackFunc in_pfnCallback,
	void* in_pCookie,
	AkUInt32 in_cExternals,
	AkExternalSourceInfo* in_pExternalSources,
	AkPlayingID	in_PlayingID
)
{
	return AK_INVALID_PLAYING_ID;
}

#ifdef AK_SUPPORT_WCHAR
AkPlayingID FWwiseSoundEngineAPI_Null::PostEvent(
	const wchar_t* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkUInt32 in_uFlags,
	AkCallbackFunc in_pfnCallback,
	void* in_pCookie,
	AkUInt32 in_cExternals,
	AkExternalSourceInfo* in_pExternalSources,
	AkPlayingID	in_PlayingID
)
{
	SCOPED_WWISESOUNDENGINE_EVENT(TEXT("FWwiseSoundEngineAPI_Null::PostEvent"));
	return AK_INVALID_PLAYING_ID;
}
#endif //AK_SUPPORT_WCHAR

AkPlayingID FWwiseSoundEngineAPI_Null::PostEvent(
	const char* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkUInt32 in_uFlags,
	AkCallbackFunc in_pfnCallback,
	void* in_pCookie,
	AkUInt32 in_cExternals,
	AkExternalSourceInfo* in_pExternalSources,
	AkPlayingID	in_PlayingID
)
{
	return AK_INVALID_PLAYING_ID;
}

AKRESULT FWwiseSoundEngineAPI_Null::ExecuteActionOnEvent(
	AkUniqueID in_eventID,
	AK::SoundEngine::AkActionOnEventType in_ActionType,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve,
	AkPlayingID in_PlayingID
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::ExecuteActionOnEvent(
	const wchar_t* in_pszEventName,
	AK::SoundEngine::AkActionOnEventType in_ActionType,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve,
	AkPlayingID in_PlayingID
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::ExecuteActionOnEvent(
	const char* in_pszEventName,
	AK::SoundEngine::AkActionOnEventType in_ActionType,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve,
	AkPlayingID in_PlayingID
)
{
	return AK_NotImplemented;
}

AkPlayingID FWwiseSoundEngineAPI_Null::PostMIDIOnEvent(
	AkUniqueID in_eventID,
	AkGameObjectID in_gameObjectID,
	AkMIDIPost* in_pPosts,
	AkUInt16 in_uNumPosts,
	bool in_bAbsoluteOffsets,
	AkUInt32 in_uFlags,
	AkCallbackFunc in_pfnCallback,
	void* in_pCookie,
	AkPlayingID in_playingID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::StopMIDIOnEvent(
	AkUniqueID in_eventID,
	AkGameObjectID in_gameObjectID,
	AkPlayingID in_playingID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PinEventInStreamCache(
	AkUniqueID in_eventID,
	AkPriority in_uActivePriority,
	AkPriority in_uInactivePriority
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PinEventInStreamCache(
	const wchar_t* in_pszEventName,
	AkPriority in_uActivePriority,
	AkPriority in_uInactivePriority
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PinEventInStreamCache(
	const char* in_pszEventName,
	AkPriority in_uActivePriority,
	AkPriority in_uInactivePriority
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::UnpinEventInStreamCache(
	AkUniqueID in_eventID
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::UnpinEventInStreamCache(
	const wchar_t* in_pszEventName
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::UnpinEventInStreamCache(
	const char* in_pszEventName
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::GetBufferStatusForPinnedEvent(
	AkUniqueID in_eventID,
	AkReal32& out_fPercentBuffered,
	bool& out_bCachePinnedMemoryFull
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::GetBufferStatusForPinnedEvent(
	const char* in_pszEventName,
	AkReal32& out_fPercentBuffered,
	bool& out_bCachePinnedMemoryFull
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::GetBufferStatusForPinnedEvent(
	const wchar_t* in_pszEventName,
	AkReal32& out_fPercentBuffered,
	bool& out_bCachePinnedMemoryFull
)
{
	return AK_NotImplemented;
}
#endif

AKRESULT FWwiseSoundEngineAPI_Null::SeekOnEvent(
	AkUniqueID in_eventID,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_iPosition,
	bool in_bSeekToNearestMarker,
	AkPlayingID in_PlayingID
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::SeekOnEvent(
	const wchar_t* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_iPosition,
	bool in_bSeekToNearestMarker,
	AkPlayingID in_PlayingID
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::SeekOnEvent(
	const char* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_iPosition,
	bool in_bSeekToNearestMarker,
	AkPlayingID in_PlayingID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SeekOnEvent(
	AkUniqueID in_eventID,
	AkGameObjectID in_gameObjectID,
	AkReal32 in_fPercent,
	bool in_bSeekToNearestMarker,
	AkPlayingID in_PlayingID
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::SeekOnEvent(
	const wchar_t* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkReal32 in_fPercent,
	bool in_bSeekToNearestMarker,
	AkPlayingID in_PlayingID
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::SeekOnEvent(
	const char* in_pszEventName,
	AkGameObjectID in_gameObjectID,
	AkReal32 in_fPercent,
	bool in_bSeekToNearestMarker,
	AkPlayingID in_PlayingID
)
{
	return AK_NotImplemented;
}

void FWwiseSoundEngineAPI_Null::CancelEventCallbackCookie(
	void* in_pCookie
)
{
}

void FWwiseSoundEngineAPI_Null::CancelEventCallbackGameObject(
	AkGameObjectID in_gameObjectID
)
{
}

void FWwiseSoundEngineAPI_Null::CancelEventCallback(
	AkPlayingID in_playingID
)
{
}

AKRESULT FWwiseSoundEngineAPI_Null::GetSourcePlayPosition(
	AkPlayingID		in_PlayingID,
	AkTimeMs* out_puPosition,
	bool			in_bExtrapolate
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::GetSourcePlayPositions(
	AkPlayingID		in_PlayingID,
	AkSourcePosition* out_puPositions,
	AkUInt32* io_pcPositions,
	bool			in_bExtrapolate
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::GetSourceStreamBuffering(
	AkPlayingID		in_PlayingID,
	AkTimeMs& out_buffering,
	bool& out_bIsBuffering
)
{
	return AK_NotImplemented;
}

void FWwiseSoundEngineAPI_Null::StopAll(
	AkGameObjectID in_gameObjectID
)
{
}

void FWwiseSoundEngineAPI_Null::StopPlayingID(
	AkPlayingID in_playingID,
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve
)
{
}

void FWwiseSoundEngineAPI_Null::ExecuteActionOnPlayingID(
	AK::SoundEngine::AkActionOnEventType in_ActionType,
	AkPlayingID in_playingID,
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve
)
{
}

void FWwiseSoundEngineAPI_Null::SetRandomSeed(
	AkUInt32 in_uSeed
)
{
}

void FWwiseSoundEngineAPI_Null::MuteBackgroundMusic(
	bool in_bMute
)
{
}

bool FWwiseSoundEngineAPI_Null::GetBackgroundMusicMute()
{
	return false;
}

AKRESULT FWwiseSoundEngineAPI_Null::SendPluginCustomGameData(
	AkUniqueID in_busID,
	AkGameObjectID in_busObjectID,
	AkPluginType in_eType,
	AkUInt32 in_uCompanyID,
	AkUInt32 in_uPluginID,
	const void* in_pData,
	AkUInt32 in_uSizeInBytes
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterGameObj(
	AkGameObjectID in_gameObjectID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterGameObj(
	AkGameObjectID in_gameObjectID,
	const char* in_pszObjName
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::UnregisterGameObj(
	AkGameObjectID in_gameObjectID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::UnregisterAllGameObj(
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetPosition(
	AkGameObjectID in_GameObjectID,
	const AkSoundPosition& in_Position,
	AkSetPositionFlags in_eFlags
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetMultiplePositions(
	AkGameObjectID in_GameObjectID,
	const AkSoundPosition* in_pPositions,
	AkUInt16 in_NumPositions,
	AK::SoundEngine::MultiPositionType in_eMultiPositionType,
	AkSetPositionFlags in_eFlags
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetMultiplePositions(
	AkGameObjectID in_GameObjectID,
	const AkChannelEmitter* in_pPositions,
	AkUInt16 in_NumPositions,
	AK::SoundEngine::MultiPositionType in_eMultiPositionType,
	AkSetPositionFlags in_eFlags
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetScalingFactor(
	AkGameObjectID in_GameObjectID,
	AkReal32 in_fAttenuationScalingFactor
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetDistanceProbe(
	AkGameObjectID in_listenerGameObjectID,
	AkGameObjectID in_distanceProbeGameObjectID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::ClearBanks()
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetBankLoadIOSettings(
	AkReal32            in_fThroughput,
	AkPriority          in_priority
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::LoadBank(
	const wchar_t* in_pszString,
	AkBankID& out_bankID,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::LoadBank(
	const char* in_pszString,
	AkBankID& out_bankID,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBank(
	AkBankID			in_bankID,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBankMemoryView(
	const void* in_pInMemoryBankPtr,
	AkUInt32 in_uInMemoryBankSize,
	AkBankID& out_bankID)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBankMemoryView(
	const void* in_pInMemoryBankPtr,
	AkUInt32			in_uInMemoryBankSize,
	AkBankID& out_bankID,
	AkBankType& out_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBankMemoryCopy(
	const void* in_pInMemoryBankPtr,
	AkUInt32 in_uInMemoryBankSize,
	AkBankID& out_bankID)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBankMemoryCopy(
	const void* in_pInMemoryBankPtr,
	AkUInt32			in_uInMemoryBankSize,
	AkBankID& out_bankID,
	AkBankType& out_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::DecodeBank(
	const void* in_pInMemoryBankPtr,
	AkUInt32			in_uInMemoryBankSize,
	AkMemPoolId			in_uPoolForDecodedBank,
	void*& out_pDecodedBankPtr,
	AkUInt32& out_uDecodedBankSize
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::LoadBank(
	const wchar_t* in_pszString,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankID& out_bankID,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::LoadBank(
	const char* in_pszString,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankID& out_bankID,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBank(
	AkBankID			in_bankID,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBankMemoryView(
	const void* in_pInMemoryBankPtr,
	AkUInt32			in_uInMemoryBankSize,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankID& out_bankID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBankMemoryView(
	const void* in_pInMemoryBankPtr,
	AkUInt32			in_uInMemoryBankSize,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankID& out_bankID,
	AkBankType& out_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBankMemoryCopy(
	const void* in_pInMemoryBankPtr,
	AkUInt32			in_uInMemoryBankSize,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankID& out_bankID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::LoadBankMemoryCopy(
	const void* in_pInMemoryBankPtr,
	AkUInt32			in_uInMemoryBankSize,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankID& out_bankID,
	AkBankType& out_bankType
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::UnloadBank(
	const wchar_t* in_pszString,
	const void* in_pInMemoryBankPtr,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::UnloadBank(
	const char* in_pszString,
	const void* in_pInMemoryBankPtr,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::UnloadBank(
	AkBankID            in_bankID,
	const void* in_pInMemoryBankPtr,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::UnloadBank(
	const wchar_t* in_pszString,
	const void* in_pInMemoryBankPtr,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::UnloadBank(
	const char* in_pszString,
	const void* in_pInMemoryBankPtr,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::UnloadBank(
	AkBankID            in_bankID,
	const void* in_pInMemoryBankPtr,
	AkBankCallbackFunc  in_pfnBankCallback,
	void* in_pCookie,
	AkBankType			in_bankType
)
{
	return AK_NotImplemented;
}

void FWwiseSoundEngineAPI_Null::CancelBankCallbackCookie(
	void* in_pCookie
)
{
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PrepareBank(
	AK::SoundEngine::PreparationType	in_PreparationType,
	const wchar_t* in_pszString,
	AK::SoundEngine::AkBankContent	in_uFlags,
	AkBankType						in_bankType
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PrepareBank(
	AK::SoundEngine::PreparationType	in_PreparationType,
	const char* in_pszString,
	AK::SoundEngine::AkBankContent	in_uFlags,
	AkBankType						in_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PrepareBank(
	AK::SoundEngine::PreparationType	in_PreparationType,
	AkBankID							in_bankID,
	AK::SoundEngine::AkBankContent		in_uFlags,
	AkBankType							in_bankType
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PrepareBank(
	AK::SoundEngine::PreparationType	in_PreparationType,
	const wchar_t* in_pszString,
	AkBankCallbackFunc	in_pfnBankCallback,
	void* in_pCookie,
	AK::SoundEngine::AkBankContent	in_uFlags,
	AkBankType						in_bankType
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PrepareBank(
	AK::SoundEngine::PreparationType	in_PreparationType,
	const char* in_pszString,
	AkBankCallbackFunc	in_pfnBankCallback,
	void* in_pCookie,
	AK::SoundEngine::AkBankContent	in_uFlags,
	AkBankType						in_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PrepareBank(
	AK::SoundEngine::PreparationType		in_PreparationType,
	AkBankID			in_bankID,
	AkBankCallbackFunc	in_pfnBankCallback,
	void* in_pCookie,
	AK::SoundEngine::AkBankContent	in_uFlags,
	AkBankType						in_bankType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::ClearPreparedEvents()
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PrepareEvent(
	AK::SoundEngine::PreparationType		in_PreparationType,
	const wchar_t** in_ppszString,
	AkUInt32			in_uNumEvent
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PrepareEvent(
	AK::SoundEngine::PreparationType		in_PreparationType,
	const char** in_ppszString,
	AkUInt32			in_uNumEvent
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PrepareEvent(
	AK::SoundEngine::PreparationType		in_PreparationType,
	AkUniqueID* in_pEventID,
	AkUInt32			in_uNumEvent
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PrepareEvent(
	AK::SoundEngine::PreparationType		in_PreparationType,
	const wchar_t** in_ppszString,
	AkUInt32			in_uNumEvent,
	AkBankCallbackFunc	in_pfnBankCallback,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PrepareEvent(
	AK::SoundEngine::PreparationType		in_PreparationType,
	const char** in_ppszString,
	AkUInt32			in_uNumEvent,
	AkBankCallbackFunc	in_pfnBankCallback,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PrepareEvent(
	AK::SoundEngine::PreparationType		in_PreparationType,
	AkUniqueID* in_pEventID,
	AkUInt32			in_uNumEvent,
	AkBankCallbackFunc	in_pfnBankCallback,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PrepareBus(
	AK::SoundEngine::PreparationType     in_PreparationType, ///< Preparation type ( Preparation_Load or Preparation_Unload )
	const wchar_t**     in_ppszString,      ///< Array of bus names
	AkUInt32            in_uBusses          ///< Number of bus names in the array
	)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PrepareBus(
		AK::SoundEngine::PreparationType     in_PreparationType, ///< Preparation type ( Preparation_Load or Preparation_Unload )
		const char**        in_ppszString,      ///< Array of bus names
		AkUInt32            in_uBusses          ///< Number of bus names in the array
		)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PrepareBus(
		AK::SoundEngine::PreparationType     in_PreparationType, ///< Preparation type ( Preparation_Load or Preparation_Unload )
		AkUniqueID*         in_pBusID,          ///< Array of bus IDs
		AkUInt32            in_uBusses          ///< Number of bus IDs in the array
		)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PrepareBus(
	AK::SoundEngine::PreparationType     in_PreparationType, ///< Preparation type ( Preparation_Load or Preparation_Unload )
	const wchar_t**     in_ppszString,      ///< Array of bus names
	AkUInt32            in_uBusses,         ///< Number of bus names in the array
	AkBankCallbackFunc  in_pfnBankCallback, ///< Callback function
	void*               in_pCookie          ///< Callback cookie (reserved to user, passed to the callback function)
	)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PrepareBus(
		AK::SoundEngine::PreparationType     in_PreparationType, ///< Preparation type ( Preparation_Load or Preparation_Unload )
		const char**        in_ppszString,      ///< Array of bus names 
		AkUInt32            in_uBusses,         ///< Number of bus names in the array
		AkBankCallbackFunc  in_pfnBankCallback, ///< Callback function
		void*               in_pCookie          ///< Callback cookie (reserved to user, passed to the callback function)
		)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PrepareBus(
		AK::SoundEngine::PreparationType     in_PreparationType, ///< Preparation type ( Preparation_Load or Preparation_Unload )
		AkUniqueID*         in_pBusID,          ///< Array of bus IDs
		AkUInt32            in_uBusses,         ///< Number of bus IDs in the array
		AkBankCallbackFunc  in_pfnBankCallback, ///< Callback function
		void*               in_pCookie          ///< Callback cookie (reserved to user, passed to the callback function)
		)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetMedia(
	AkSourceSettings* in_pSourceSettings,
	AkUInt32			in_uNumSourceSettings
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::TryUnsetMedia(
	AkSourceSettings* in_pSourceSettings,
	AkUInt32          in_uNumSourceSettings,
	AKRESULT* out_pUnsetResults
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PrepareGameSyncs(
	AK::SoundEngine::PreparationType	in_PreparationType,
	AkGroupType		in_eGameSyncType,
	const wchar_t* in_pszGroupName,
	const wchar_t** in_ppszGameSyncName,
	AkUInt32		in_uNumGameSyncs
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PrepareGameSyncs(
	AK::SoundEngine::PreparationType	in_PreparationType,
	AkGroupType		in_eGameSyncType,
	const char* in_pszGroupName,
	const char** in_ppszGameSyncName,
	AkUInt32		in_uNumGameSyncs
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PrepareGameSyncs(
	AK::SoundEngine::PreparationType	in_PreparationType,
	AkGroupType		in_eGameSyncType,
	AkUInt32		in_GroupID,
	AkUInt32* in_paGameSyncID,
	AkUInt32		in_uNumGameSyncs
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PrepareGameSyncs(
	AK::SoundEngine::PreparationType		in_PreparationType,
	AkGroupType			in_eGameSyncType,
	const wchar_t* in_pszGroupName,
	const wchar_t** in_ppszGameSyncName,
	AkUInt32			in_uNumGameSyncs,
	AkBankCallbackFunc	in_pfnBankCallback,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PrepareGameSyncs(
	AK::SoundEngine::PreparationType		in_PreparationType,
	AkGroupType			in_eGameSyncType,
	const char* in_pszGroupName,
	const char** in_ppszGameSyncName,
	AkUInt32			in_uNumGameSyncs,
	AkBankCallbackFunc	in_pfnBankCallback,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PrepareGameSyncs(
	AK::SoundEngine::PreparationType		in_PreparationType,
	AkGroupType			in_eGameSyncType,
	AkUInt32			in_GroupID,
	AkUInt32* in_paGameSyncID,
	AkUInt32			in_uNumGameSyncs,
	AkBankCallbackFunc	in_pfnBankCallback,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetListeners(
	AkGameObjectID in_emitterGameObj,
	const AkGameObjectID* in_pListenerGameObjs,
	AkUInt32 in_uNumListeners
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::AddListener(
	AkGameObjectID in_emitterGameObj,
	AkGameObjectID in_listenerGameObj
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RemoveListener(
	AkGameObjectID in_emitterGameObj,
	AkGameObjectID in_listenerGameObj
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetDefaultListeners(
	const AkGameObjectID* in_pListenerObjs,
	AkUInt32 in_uNumListeners
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::AddDefaultListener(
	AkGameObjectID in_listenerGameObj
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RemoveDefaultListener(
	AkGameObjectID in_listenerGameObj
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::ResetListenersToDefault(
	AkGameObjectID in_emitterGameObj
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetListenerSpatialization(
	AkGameObjectID in_uListenerID,
	bool in_bSpatialized,
	AkChannelConfig in_channelConfig,
	AK::SpeakerVolumes::VectorPtr in_pVolumeOffsets
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetRTPCValue(
	AkRtpcID in_rtpcID,
	AkRtpcValue in_value,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_uValueChangeDuration,
	AkCurveInterpolation in_eFadeCurve,
	bool in_bBypassInternalValueInterpolation
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::SetRTPCValue(
	const wchar_t* in_pszRtpcName,
	AkRtpcValue in_value,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_uValueChangeDuration,
	AkCurveInterpolation in_eFadeCurve,
	bool in_bBypassInternalValueInterpolation
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::SetRTPCValue(
	const char* in_pszRtpcName,
	AkRtpcValue in_value,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_uValueChangeDuration,
	AkCurveInterpolation in_eFadeCurve,
	bool in_bBypassInternalValueInterpolation
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetRTPCValueByPlayingID(
	AkRtpcID in_rtpcID,
	AkRtpcValue in_value,
	AkPlayingID in_playingID,
	AkTimeMs in_uValueChangeDuration,
	AkCurveInterpolation in_eFadeCurve,
	bool in_bBypassInternalValueInterpolation
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::SetRTPCValueByPlayingID(
	const wchar_t* in_pszRtpcName,
	AkRtpcValue in_value,
	AkPlayingID in_playingID,
	AkTimeMs in_uValueChangeDuration,
	AkCurveInterpolation in_eFadeCurve,
	bool in_bBypassInternalValueInterpolation
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::SetRTPCValueByPlayingID(
	const char* in_pszRtpcName,
	AkRtpcValue in_value,
	AkPlayingID in_playingID,
	AkTimeMs in_uValueChangeDuration,
	AkCurveInterpolation in_eFadeCurve,
	bool in_bBypassInternalValueInterpolation
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::ResetRTPCValue(
	AkRtpcID in_rtpcID,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_uValueChangeDuration,
	AkCurveInterpolation in_eFadeCurve,
	bool in_bBypassInternalValueInterpolation
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::ResetRTPCValue(
	const wchar_t* in_pszRtpcName,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_uValueChangeDuration,
	AkCurveInterpolation in_eFadeCurve,
	bool in_bBypassInternalValueInterpolation
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::ResetRTPCValue(
	const char* in_pszRtpcName,
	AkGameObjectID in_gameObjectID,
	AkTimeMs in_uValueChangeDuration,
	AkCurveInterpolation in_eFadeCurve,
	bool in_bBypassInternalValueInterpolation
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetSwitch(
	AkSwitchGroupID in_switchGroup,
	AkSwitchStateID in_switchState,
	AkGameObjectID in_gameObjectID
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::SetSwitch(
	const wchar_t* in_pszSwitchGroup,
	const wchar_t* in_pszSwitchState,
	AkGameObjectID in_gameObjectID
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::SetSwitch(
	const char* in_pszSwitchGroup,
	const char* in_pszSwitchState,
	AkGameObjectID in_gameObjectID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::PostTrigger(
	AkTriggerID 	in_triggerID,
	AkGameObjectID 	in_gameObjectID
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::PostTrigger(
	const wchar_t* in_pszTrigger,
	AkGameObjectID in_gameObjectID
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::PostTrigger(
	const char* in_pszTrigger,
	AkGameObjectID in_gameObjectID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetState(
	AkStateGroupID in_stateGroup,
	AkStateID in_state
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::SetState(
	const wchar_t* in_pszStateGroup,
	const wchar_t* in_pszState
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::SetState(
	const char* in_pszStateGroup,
	const char* in_pszState
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetGameObjectAuxSendValues(
	AkGameObjectID		in_gameObjectID,
	AkAuxSendValue* in_aAuxSendValues,
	AkUInt32			in_uNumSendValues
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterBusVolumeCallback(
	AkUniqueID in_busID,
	AkBusCallbackFunc in_pfnCallback,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterBusMeteringCallback(
	AkUniqueID in_busID,
	AkBusMeteringCallbackFunc in_pfnCallback,
	AkMeteringFlags in_eMeteringFlags,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterOutputDeviceMeteringCallback(
	AkOutputDeviceID in_idOutput,
	AkOutputDeviceMeteringCallbackFunc in_pfnCallback,
	AkMeteringFlags in_eMeteringFlags,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetGameObjectOutputBusVolume(
	AkGameObjectID		in_emitterObjID,
	AkGameObjectID		in_listenerObjID,
	AkReal32			in_fControlValue
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetActorMixerEffect(
	AkUniqueID in_audioNodeID,
	AkUInt32 in_uFXIndex,
	AkUniqueID in_shareSetID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetBusEffect(
	AkUniqueID in_audioNodeID,
	AkUInt32 in_uFXIndex,
	AkUniqueID in_shareSetID
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::SetBusEffect(
	const wchar_t* in_pszBusName,
	AkUInt32 in_uFXIndex,
	AkUniqueID in_shareSetID
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::SetBusEffect(
	const char* in_pszBusName,
	AkUInt32 in_uFXIndex,
	AkUniqueID in_shareSetID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetOutputDeviceEffect(
	AkOutputDeviceID in_outputDeviceID,
	AkUInt32 in_uFXIndex,
	AkUniqueID in_FXShareSetID
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetBusConfig(
	AkUniqueID in_audioNodeID,
	AkChannelConfig in_channelConfig
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::SetBusConfig(
	const wchar_t* in_pszBusName,
	AkChannelConfig in_channelConfig
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::SetBusConfig(
	const char* in_pszBusName,
	AkChannelConfig in_channelConfig
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetObjectObstructionAndOcclusion(
	AkGameObjectID in_EmitterID,
	AkGameObjectID in_ListenerID,
	AkReal32 in_fObstructionLevel,
	AkReal32 in_fOcclusionLevel
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetMultipleObstructionAndOcclusion(
	AkGameObjectID in_EmitterID,
	AkGameObjectID in_uListenerID,
	AkObstructionOcclusionValues* in_fObstructionOcclusionValues,
	AkUInt32 in_uNumOcclusionObstruction
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::GetContainerHistory(
	AK::IWriteBytes* in_pBytes
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetContainerHistory(
	AK::IReadBytes* in_pBytes
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::StartOutputCapture(
	const AkOSChar* in_CaptureFileName
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::StopOutputCapture()
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::AddOutputCaptureMarker(
	const char* in_MarkerText
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::AddOutputCaptureBinaryMarker(
	void* in_pMarkerData,
	AkUInt32 in_uMarkerDataSize
	)
{
	return AK_NotImplemented;
}

AkUInt32 FWwiseSoundEngineAPI_Null::GetSampleRate()
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RegisterCaptureCallback(
	AkCaptureCallbackFunc in_pfnCallback,
	AkOutputDeviceID in_idOutput,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::UnregisterCaptureCallback(
	AkCaptureCallbackFunc in_pfnCallback,
	AkOutputDeviceID in_idOutput,
	void* in_pCookie
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::StartProfilerCapture(
	const AkOSChar* in_CaptureFileName
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::StopProfilerCapture()
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetOfflineRenderingFrameTime(
	AkReal32 in_fFrameTimeInSeconds
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetOfflineRendering(
	bool in_bEnableOfflineRendering
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::AddOutput(
	const AkOutputSettings& in_Settings,
	AkOutputDeviceID* out_pDeviceID,
	const AkGameObjectID* in_pListenerIDs,
	AkUInt32 in_uNumListeners
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::RemoveOutput(
	AkOutputDeviceID in_idOutput
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::ReplaceOutput(
	const AkOutputSettings& in_Settings,
	AkOutputDeviceID in_outputDeviceId,
	AkOutputDeviceID* out_pOutputDeviceId
)
{
	return AK_NotImplemented;
}

AkOutputDeviceID FWwiseSoundEngineAPI_Null::GetOutputID(
	AkUniqueID in_idShareSet,
	AkUInt32 in_idDevice
)
{
	return AK_INVALID_DEVICE_ID;
}

AkOutputDeviceID FWwiseSoundEngineAPI_Null::GetOutputID(
	const char* in_szShareSet,
	AkUInt32 in_idDevice
)
{
	return AK_INVALID_DEVICE_ID;
}

#ifdef AK_SUPPORT_WCHAR
AkOutputDeviceID FWwiseSoundEngineAPI_Null::GetOutputID(
	const wchar_t* in_szShareSet,
	AkUInt32 in_idDevice
)
{
	return AK_INVALID_DEVICE_ID;
}
#endif

AKRESULT FWwiseSoundEngineAPI_Null::SetBusDevice(
	AkUniqueID in_idBus,
	AkUniqueID in_idNewDevice
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetBusDevice(
	const char* in_BusName,
	const char* in_DeviceName
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::SetBusDevice(
	const wchar_t* in_BusName,
	const wchar_t* in_DeviceName
)
{
	return AK_NotImplemented;
}
#endif

AKRESULT FWwiseSoundEngineAPI_Null::GetDeviceList(
	AkUInt32 in_ulCompanyID,
	AkUInt32 in_ulPluginID,
	AkUInt32& io_maxNumDevices,
	AkDeviceDescription* out_deviceDescriptions
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::GetDeviceList(
	AkUniqueID in_audioDeviceShareSetID,
	AkUInt32& io_maxNumDevices,
	AkDeviceDescription* out_deviceDescriptions
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::SetOutputVolume(
	AkOutputDeviceID in_idOutput,
	AkReal32 in_fVolume
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::GetDeviceSpatialAudioSupport(
	AkUInt32 in_idDevice)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::Suspend(
	bool in_bRenderAnyway,
	bool in_bFadeOut
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::WakeupFromSuspend(
	AkUInt32 in_uDelayMs
)
{
	return AK_NotImplemented;
}

AkUInt32 FWwiseSoundEngineAPI_Null::GetBufferTick()
{
	return 0;
}

AkUInt64 FWwiseSoundEngineAPI_Null::GetSampleTick()
{
	return 0;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetPosition(
	AkGameObjectID in_GameObjectID,
	AkSoundPosition& out_rPosition
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetListeners(
	AkGameObjectID in_GameObjectID,
	AkGameObjectID* out_ListenerObjectIDs,
	AkUInt32& oi_uNumListeners
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetListenerPosition(
	AkGameObjectID in_uListenerID,
	AkListenerPosition& out_rPosition
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetListenerSpatialization(
	AkGameObjectID in_uListenerID,
	bool& out_rbSpatialized,
	AK::SpeakerVolumes::VectorPtr& out_pVolumeOffsets,
	AkChannelConfig& out_channelConfig
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetRTPCValue(
	AkRtpcID in_rtpcID,
	AkGameObjectID in_gameObjectID,
	AkPlayingID	in_playingID,
	AkRtpcValue& out_rValue,
	AK::SoundEngine::Query::RTPCValue_type& io_rValueType
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetRTPCValue(
	const wchar_t* in_pszRtpcName,
	AkGameObjectID in_gameObjectID,
	AkPlayingID	in_playingID,
	AkRtpcValue& out_rValue,
	AK::SoundEngine::Query::RTPCValue_type& io_rValueType
)
{
	return AK_NotImplemented;
}

#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetRTPCValue(
	const char* in_pszRtpcName,
	AkGameObjectID in_gameObjectID,
	AkPlayingID	in_playingID,
	AkRtpcValue& out_rValue,
	AK::SoundEngine::Query::RTPCValue_type& io_rValueType
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetSwitch(
	AkSwitchGroupID in_switchGroup,
	AkGameObjectID  in_gameObjectID,
	AkSwitchStateID& out_rSwitchState
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetSwitch(
	const wchar_t* in_pstrSwitchGroupName,
	AkGameObjectID in_GameObj,
	AkSwitchStateID& out_rSwitchState
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetSwitch(
	const char* in_pstrSwitchGroupName,
	AkGameObjectID in_GameObj,
	AkSwitchStateID& out_rSwitchState
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetState(
	AkStateGroupID in_stateGroup,
	AkStateID& out_rState
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetState(
	const wchar_t* in_pstrStateGroupName,
	AkStateID& out_rState
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetState(
	const char* in_pstrStateGroupName,
	AkStateID& out_rState
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetGameObjectAuxSendValues(
	AkGameObjectID		in_gameObjectID,
	AkAuxSendValue* out_paAuxSendValues,
	AkUInt32& io_ruNumSendValues
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetGameObjectDryLevelValue(
	AkGameObjectID		in_EmitterID,
	AkGameObjectID		in_ListenerID,
	AkReal32& out_rfControlValue
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetObjectObstructionAndOcclusion(
	AkGameObjectID in_EmitterID,
	AkGameObjectID in_ListenerID,
	AkReal32& out_rfObstructionLevel,
	AkReal32& out_rfOcclusionLevel
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::QueryAudioObjectIDs(
	AkUniqueID in_eventID,
	AkUInt32& io_ruNumItems,
	AkObjectInfo* out_aObjectInfos
)
{
	return AK_NotImplemented;
}

#ifdef AK_SUPPORT_WCHAR
AKRESULT FWwiseSoundEngineAPI_Null::FQuery::QueryAudioObjectIDs(
	const wchar_t* in_pszEventName,
	AkUInt32& io_ruNumItems,
	AkObjectInfo* out_aObjectInfos
)
{
	return AK_NotImplemented;
}
#endif //AK_SUPPORT_WCHAR

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::QueryAudioObjectIDs(
	const char* in_pszEventName,
	AkUInt32& io_ruNumItems,
	AkObjectInfo* out_aObjectInfos
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetPositioningInfo(
	AkUniqueID in_ObjectID,
	AkPositioningInfo& out_rPositioningInfo
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetActiveGameObjects(
	FAkGameObjectsList& io_GameObjectList
)
{
	return AK_NotImplemented;
}

bool FWwiseSoundEngineAPI_Null::FQuery::GetIsGameObjectActive(
	AkGameObjectID in_GameObjId
)
{
	return false;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetMaxRadius(
	FAkRadiusList& io_RadiusList
)
{
	return AK_NotImplemented;
}

AkReal32 FWwiseSoundEngineAPI_Null::FQuery::GetMaxRadius(
	AkGameObjectID in_GameObjId
)
{
	return .0f;
}

AkUniqueID FWwiseSoundEngineAPI_Null::FQuery::GetEventIDFromPlayingID(
	AkPlayingID in_playingID
)
{
	return AK_INVALID_UNIQUE_ID;
}

AkGameObjectID FWwiseSoundEngineAPI_Null::FQuery::GetGameObjectFromPlayingID(
	AkPlayingID in_playingID
)
{
	return AK_INVALID_GAME_OBJECT;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetPlayingIDsFromGameObject(
	AkGameObjectID in_GameObjId,
	AkUInt32& io_ruNumIDs,
	AkPlayingID* out_aPlayingIDs
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetCustomPropertyValue(
	AkUniqueID in_ObjectID,
	AkUInt32 in_uPropID,
	AkInt32& out_iValue
)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FQuery::GetCustomPropertyValue(
	AkUniqueID in_ObjectID,
	AkUInt32 in_uPropID,
	AkReal32& out_fValue
)
{
	return AK_NotImplemented;
}

void FWwiseSoundEngineAPI_Null::FAudioInputPlugin::SetAudioInputCallbacks(
	AkAudioInputPluginExecuteCallbackFunc in_pfnExecCallback,
	AkAudioInputPluginGetFormatCallbackFunc in_pfnGetFormatCallback /*= nullptr */,
	AkAudioInputPluginGetGainCallbackFunc in_pfnGetGainCallback /*= nullptr*/
)
{
}

AkUniqueID FWwiseSoundEngineAPI_Null::FDynamicDialogue::ResolveDialogueEvent(
		AkUniqueID			in_eventID,
		AkArgumentValueID*	in_aArgumentValues,
		AkUInt32			in_uNumArguments,
		AkPlayingID			in_idSequence,
		AkCandidateCallbackFunc in_candidateCallbackFunc,
		void* in_pCookie
	)
{
	return AK_INVALID_PLAYING_ID;
}

#ifdef AK_SUPPORT_WCHAR
AkUniqueID FWwiseSoundEngineAPI_Null::FDynamicDialogue::ResolveDialogueEvent(
		const wchar_t*		in_pszEventName,
		const wchar_t**		in_aArgumentValueNames,
		AkUInt32			in_uNumArguments,
		AkPlayingID			in_idSequence,
		AkCandidateCallbackFunc in_candidateCallbackFunc,
		void* in_pCookie
	)
{
	return AK_INVALID_PLAYING_ID;
}
#endif //AK_SUPPORT_WCHAR

AkUniqueID FWwiseSoundEngineAPI_Null::FDynamicDialogue::ResolveDialogueEvent(
		const char*			in_pszEventName,
		const char**		in_aArgumentValueNames,
		AkUInt32			in_uNumArguments,
		AkPlayingID			in_idSequence,
		AkCandidateCallbackFunc in_candidateCallbackFunc,
		void* in_pCookie
	)
{
	return AK_INVALID_PLAYING_ID;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicDialogue::GetDialogueEventCustomPropertyValue(
	AkUniqueID in_eventID,
	AkUInt32 in_uPropID,
	AkInt32& out_iValue
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicDialogue::GetDialogueEventCustomPropertyValue(
	AkUniqueID in_eventID,
	AkUInt32 in_uPropID,
	AkReal32& out_fValue
	)
{
	return AK_NotImplemented;
}

void FWwiseSoundEngineAPI_Null::FDynamicSequence::FPlaylist::Enqueue(
		AkUniqueID in_audioNodeID,
		AkTimeMs in_msDelay,
		void * in_pCustomInfo,
		AkUInt32 in_cExternals,
		AkExternalSourceInfo *in_pExternalSources
		)
{
}

void FWwiseSoundEngineAPI_Null::FDynamicSequence::FPlaylist::RemoveAll(int Reserve)
{
}

AK::SoundEngine::DynamicSequence::PlaylistItem* FWwiseSoundEngineAPI_Null::FDynamicSequence::FPlaylist::Insert(unsigned int in_uIndex)
{
	return nullptr;
}

void FWwiseSoundEngineAPI_Null::FDynamicSequence::FPlaylist::Erase(unsigned int in_uIndex)
{
}

AK::SoundEngine::DynamicSequence::PlaylistItem& FWwiseSoundEngineAPI_Null::FDynamicSequence::FPlaylist::operator[](unsigned int uiIndex) const
{
	check(false);
	AK::SoundEngine::DynamicSequence::PlaylistItem* Empty = 0;
	return *Empty;
}

bool FWwiseSoundEngineAPI_Null::FDynamicSequence::FPlaylist::IsEmpty() const
{
	return true;
}

uint32 FWwiseSoundEngineAPI_Null::FDynamicSequence::FPlaylist::Length() const
{
	return 0;
}

AkPlayingID FWwiseSoundEngineAPI_Null::FDynamicSequence::Open(
	AkGameObjectID		in_gameObjectID,
	AkUInt32			in_uFlags,
	AkCallbackFunc		in_pfnCallback,
	void* 				in_pCookie,
	AK::SoundEngine::DynamicSequence::DynamicSequenceType in_eDynamicSequenceType
	)
{
	return AK_INVALID_PLAYING_ID;
}
											
AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::Close(
	AkPlayingID in_playingID
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::Play( 
	AkPlayingID in_playingID,
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::Pause( 
	AkPlayingID in_playingID,
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::Resume(
	AkPlayingID in_playingID,
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::Stop(
	AkPlayingID in_playingID,
	AkTimeMs in_uTransitionDuration,
	AkCurveInterpolation in_eFadeCurve
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::Break(
	AkPlayingID in_playingID
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::Seek(
	AkPlayingID in_playingID,
	AkTimeMs in_iPosition,
	bool in_bSeekToNearestMarker
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::Seek(
	AkPlayingID in_playingID,
	AkReal32 in_fPercent,
	bool in_bSeekToNearestMarker
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::GetPauseTimes(
	AkPlayingID in_playingID,
	AkUInt32 &out_uTime,
	AkUInt32 &out_uDuration
	)
{
	return AK_NotImplemented;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::GetPlayingItem(
	AkPlayingID in_playingID,
	AkUniqueID & out_audioNodeID,
	void *& out_pCustomInfo
	)
{
	return AK_NotImplemented;
}

FWwiseSoundEngineAPI_Null::FDynamicSequence::IPlaylist* FWwiseSoundEngineAPI_Null::FDynamicSequence::LockPlaylist(
	AkPlayingID in_playingID
	)
{
	return nullptr;
}

AKRESULT FWwiseSoundEngineAPI_Null::FDynamicSequence::UnlockPlaylist(
	IPlaylist* in_Playlist
	)
{
	return AK_NotImplemented;
}

#if WITH_EDITORONLY_DATA
FWwiseSoundEngineAPI_Null::FErrorTranslator::FErrorTranslator(FGetInfoErrorMessageTranslatorFunction InMessageTranslatorFunction) :
	GetInfoErrorMessageTranslatorFunction(InMessageTranslatorFunction)
{
}

bool FWwiseSoundEngineAPI_Null::FErrorTranslator::GetInfo(TagInformation* in_pTagList, AkUInt32 in_uCount,
                                              AkUInt32& out_uTranslated)
{
	return false;
}
#endif

AkErrorMessageTranslator* FWwiseSoundEngineAPI_Null::NewErrorMessageTranslator(FGetInfoErrorMessageTranslatorFunction InMessageTranslatorFunction)
{
	return nullptr;
}
