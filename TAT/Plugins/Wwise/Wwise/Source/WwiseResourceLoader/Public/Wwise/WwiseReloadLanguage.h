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

#include "WwiseReloadLanguage.generated.h"

/**
 * @brief Determines the reload strategy to use for language changes.
 */
UENUM()
enum class EWwiseReloadLanguage
{
	/// Doesn't reload anything and doesn't call any SoundEngine operations. The game must reload elements as required.
	Manual,

	/// Reloads immediately without stopping any processes. The game is responsible for stopping and restarting
	/// sounds that could be affected, or audible breaks might occur. This is useful when some
	/// sounds can continue to play, such as music and ambient sounds, while the dialogues are being
	/// internally reloaded.
	/// 
	/// Depending on the quantity of currently loaded localized banks, the operation can take a long time.
	/// 
	/// \warning Affected events need to be restarted when the operation is done.
	Immediate,

	/// Stops all sounds first, unloads all the localized banks, and reloads the new language. This causes
	/// an audible break while the operation is done.
	/// 
	/// Depending on the quantity of currently loaded localized banks, the operation can take a long time.
	/// 
	/// \warning Affected events need to be restarted when the operation is done.
	Safe
};
