// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"
#include "OSESightTargetInterface.generated.h"

class AActor;

UINTERFACE()
class OSEPERCEPTION_API UOSESightTargetInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class OSEPERCEPTION_API IOSESightTargetInterface
{
	GENERATED_IINTERFACE_BODY()

	/**	
	 * The method needs to check whether the implementer is visible from given observer's location. 
	 * @param ObserverLocation	The location of the observer
	 * @param OutSeenLocation	The first visible target location
	 * @param OutSightStrengh	The sight strength for how well the target is seen
	 * @param IgnoreActor		The actor to ignore when doing test
	 * @param bWasVisible		If available, it is the previous visibility state
	 * @param UserData			If available, it is a data passed between visibility tests for the users to store whatever they want
	 * @return	True if visible from the observer's location
	 */
	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor = nullptr, const bool* bWasVisible = nullptr, int32* UserData = nullptr) const
	{ 
		NumberOfLoSChecksPerformed = 0;
		OutSightStrength = 0;
		return false; 
	}

};

