// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "OSEPerceptionTypes.h"
#include "OSESense.h"
#include "OSESenseConfig.h"
#include "OSESense_Sight.h"
#include "OSESenseConfig_Sight.generated.h"

class FGameplayDebuggerCategory;
class UOSEPerceptionComponent;

UCLASS(meta = (DisplayName = "OSE Sight config"))
class OSEPERCEPTION_API UOSESenseConfig_Sight : public UOSESenseConfig
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", NoClear, config)
	TSubclassOf<UOSESense_Sight> Implementation;

	/** Maximum sight distance to notice a target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
	float SightRadius;

	/** Maximum sight distance to see target that has been already seen. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
	float LoseSightRadius;

	/** How far to the side AI can see, in degrees. Use SetPeripheralVisionAngle to change the value at runtime. 
	 *	The value represents the angle measured in relation to the forward vector, not the whole range. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta=(UIMin = 0.0, ClampMin = 0.0, UIMax = 180.0, ClampMax = 180.0, DisplayName="PeripheralVisionHalfAngleDegrees"))
	float PeripheralVisionAngleDegrees;

	/** */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
	FOSESenseAffiliationFilter DetectionByAffiliation;

	/** If not an InvalidRange (which is the default), we will always be able to see the target that has already been seen if they are within this range of their last seen location. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
	float AutoSuccessRangeFromLastSeenLocation;

	/** Point of view move back distance for cone calculation. In conjunction with near clipping distance, this will act as a close by awareness and peripheral vision. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
	float PointOfViewBackwardOffset;

	/** Near clipping distance, to be used with point of view backward offset. Will act as a close by awareness and peripheral vision */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
	float NearClippingRadius;

	virtual TSubclassOf<UOSESense> GetSenseImplementation() const override;

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

#if WITH_GAMEPLAY_DEBUGGER
	virtual void DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* PerceptionComponent, FGameplayDebuggerCategory* DebuggerCategory) const;
#endif // WITH_GAMEPLAY_DEBUGGER
};
