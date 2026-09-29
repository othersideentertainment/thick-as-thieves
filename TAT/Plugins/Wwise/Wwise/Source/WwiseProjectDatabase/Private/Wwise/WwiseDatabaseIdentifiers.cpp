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

#include "Wwise/WwiseDatabaseIdentifiers.h"

WwiseDBShortId GetTypeHash(const WwiseDatabaseMediaIdKey& MediaId)
{
	return WwiseDBHashCombine(
		GetTypeHash(MediaId.MediaId),
		GetTypeHash(MediaId.SoundBankId));
}

WwiseDBShortId GetTypeHash(const WwiseDatabaseLocalizableIdKey& LocalizableId)
{
	return WwiseDBHashCombine(WwiseDBHashCombine(
		GetTypeHash(LocalizableId.Id),
		GetTypeHash(LocalizableId.SoundBankId)),
		GetTypeHash(LocalizableId.LanguageId));
}

WwiseDBShortId GetTypeHash(const WwiseDatabaseGroupValueKey& GroupId)
{
	return WwiseDBHashCombine(
		GetTypeHash(GroupId.GroupId),
		GetTypeHash(GroupId.Id));
}

WwiseDBShortId GetTypeHash(const WwiseDatabaseLocalizableGroupValueKey& LocalizableGroupValue)
{
	return WwiseDBHashCombine(
		GetTypeHash(LocalizableGroupValue.GroupValue),
		GetTypeHash(LocalizableGroupValue.LanguageId));
}

WwiseDBShortId GetTypeHash(const WwiseDatabaseLocalizableGuidKey& LocalizableGuid)
{
	return WwiseDBHashCombine(
		GetTypeHash(LocalizableGuid.Guid),
		GetTypeHash(LocalizableGuid.LanguageId));
}

WwiseDBShortId GetTypeHash(const WwiseDatabaseLocalizableNameKey& LocalizableName)
{
	return WwiseDBHashCombine(
		GetTypeHash(LocalizableName.Name),
		GetTypeHash(LocalizableName.LanguageId));
}
