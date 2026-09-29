// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "EngineDefines.h"
#include "OSEPerceptionTypes.h"
#include "OSESenseEvent.generated.h"

UCLASS(ClassGroup = AI, abstract, EditInlineNew, config = Game)
class OSEPERCEPTION_API UOSESenseEvent : public UObject
{
	GENERATED_BODY()

public:
	virtual FOSESenseID GetSenseID() const PURE_VIRTUAL(UOSESenseEvent::GetSenseID, return FOSESenseID::InvalidID(););

#if ENABLE_VISUAL_LOG
	virtual void DrawToVLog(UObject& LogOwner) const {}
#else
	void DrawToVLog(UObject& LogOwner) const {}
#endif // ENABLE_VISUAL_LOG
};
