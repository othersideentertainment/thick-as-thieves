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

#include "AkInclude.h"
#include "WwiseUnrealDefines.h"
#include "AkRtpc.h"
#include "Curves/RichCurve.h"
#include "MovieSceneSection.h"
#include "MovieSceneFloatChannelSerializationHelper.h"

#include "Channels/MovieSceneFloatChannel.h"
#include "MovieSceneWwiseGameParameterSection.generated.h"

/**
* A single floating point section
*/
UCLASS()
class AKAUDIO_API UMovieSceneWwiseGameParameterSection
	: public UMovieSceneSection
{
	GENERATED_BODY()
	UMovieSceneWwiseGameParameterSection(const FObjectInitializer& Init);
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;

public:
	FMovieSceneFloatChannel GetChannel() const {	return GameParameterChannel;	}
	float GetStartTime() const;
	float GetEndTime() const;

	/** @return the GameParameter being modified by this track */
	TObjectPtr<UAkRtpc> GetGameParameter() const { return GameParameter; }

	void SetGameParameter(UAkRtpc* InGameParameter) { GameParameter = InGameParameter; }

protected:
	UPROPERTY(EditAnywhere, Category = "WwiseGameParameter", meta = (NoResetToDefault))
	TObjectPtr<UAkRtpc> GameParameter = nullptr;

	/** Curve data */
	UPROPERTY()
	FRichCurve FloatCurve;

	// Enabled serialization of GameParameterChannel when 4.24 support was added. 
	UPROPERTY()
	FMovieSceneFloatChannelSerializationHelper FloatChannelSerializationHelper;

	UPROPERTY()
	FMovieSceneFloatChannel GameParameterChannel;
};
