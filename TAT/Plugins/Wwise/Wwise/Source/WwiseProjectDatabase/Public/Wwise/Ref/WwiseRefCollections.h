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

#include "Wwise/Ref/WwiseRefForwardDeclarations.h"
#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"
#include "Wwise/WwiseDatabaseIdentifiers.h"

using WwiseLanguageNamesMap = WwiseDBMap<WwiseDBString, WwiseRefLanguage>;
using WwiseLanguageIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefLanguage>;
using WwisePlatformNamesMap = WwiseDBMap<WwiseDBString, WwiseRefPlatform>;
using WwisePlatformGuidsMap = WwiseDBMap<WwiseDBGuid, WwiseRefPlatform>;
using WwiseMediaIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefMedia>;
using WwisePluginLibNamesMap = WwiseDBMap<WwiseDBString, WwiseRefPluginLib>;
using WwisePluginLibIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefPluginLib>;
using WwiseSoundBankIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefSoundBank>;
using WwiseDialogueEventIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefDialogueEvent>;
using WwiseDialogueArgumentIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefDialogueArgument>;
using WwiseBusIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefBus>;
using WwiseAuxBusIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefAuxBus>;
using WwiseCustomPluginIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefCustomPlugin>;
using WwisePluginShareSetIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefPluginShareSet>;
using WwiseAudioDeviceIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefAudioDevice>;
using WwiseEventIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefEvent>;
using WwiseExternalSourceIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefExternalSource>;
using WwiseAcousticTextureIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefAcousticTexture>;
using WwiseGameParameterIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefGameParameter>;
using WwiseStateGroupIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefStateGroup>;
using WwiseSwitchGroupIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefSwitchGroup>;
using WwiseTriggerIdsMap = WwiseDBMap<WwiseDBShortId, WwiseRefTrigger>;
using WwiseStateIdsMap = WwiseDBMap<GroupValueKey, WwiseRefState>;
using WwiseSwitchIdsMap = WwiseDBMap<GroupValueKey, WwiseRefSwitch>;
using WwiseSwitchContainerArray = WwiseDBArray<WwiseRefSwitchContainer>;

using WwiseMediaGlobalIdsMap = WwiseDBMap<MediaIdKey, WwiseRefMedia>;
using WwiseSoundBankGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefSoundBank>;
using WwiseDialogueEventGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefDialogueEvent>;
using WwiseDialogueArgumentGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefDialogueArgument>;
using WwiseBusGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefBus>;
using WwiseAuxBusGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefAuxBus>;
using WwiseCustomPluginGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefCustomPlugin>;
using WwisePluginShareSetGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefPluginShareSet>;
using WwisePluginLibGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefPluginLib>;
using WwiseAudioDeviceGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefAudioDevice>;
using WwiseEventGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefEvent>;
using WwiseExternalSourceGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefExternalSource>;
using WwiseAcousticTextureGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefAcousticTexture>;
using WwiseGameParameterGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefGameParameter>;
using WwiseStateGroupGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefStateGroup>;
using WwiseSwitchGroupGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefSwitchGroup>;
using WwiseTriggerGlobalIdsMap = WwiseDBMap<LocalizableIdKey, WwiseRefTrigger>;
using WwiseStateGlobalIdsMap = WwiseDBMap<LocalizableGroupValueKey, WwiseRefState>;
using WwiseSwitchGlobalIdsMap = WwiseDBMap<LocalizableGroupValueKey, WwiseRefSwitch>;
using WwiseSwitchContainersByEvent = WwiseDBMultiMap<LocalizableIdKey, WwiseRefSwitchContainer>;

using WwiseGuidMap = WwiseDBMultiMap<LocalizableGuidKey, WwiseAnyRef>;
using WwiseNameMap = WwiseDBMultiMap<LocalizableNameKey, WwiseAnyRef>;
