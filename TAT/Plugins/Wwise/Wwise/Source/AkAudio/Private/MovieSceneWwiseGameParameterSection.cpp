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

#include "MovieSceneWwiseGameParameterSection.h"
#include "AkAudioDevice.h"
#include "AkCustomVersion.h"
#include "MovieSceneWwiseGameParameterTemplate.h"

#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneChannelEditorData.h"
#include "MovieScene.h"

UMovieSceneWwiseGameParameterSection::UMovieSceneWwiseGameParameterSection(const FObjectInitializer& Init)
	: Super(Init)
{
	FMovieSceneChannelProxyData Channels;

#if WITH_EDITOR
	FMovieSceneChannelMetaData Metadata;
	Metadata.SetIdentifiers("GameParameter", NSLOCTEXT("MovieSceneWwiseGameParameterSectionEditorData", "GameParameter", "GameParameter"));
	Channels.Add(GameParameterChannel, Metadata, TMovieSceneExternalValue<float>());
#else
	Channels.Add(GameParameterChannel);
#endif

	// Populate the channel proxy - if any of our channels were ever reallocated, we'd need to repopulate the proxy,
	// but since ours are all value member types, we only need to populate in the constructor
	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(MoveTemp(Channels));
}

float UMovieSceneWwiseGameParameterSection::GetStartTime() const
{
	FFrameRate FrameRate = GetTypedOuter<UMovieScene>()->GetTickResolution();
	return (float)FrameRate.AsSeconds(GetRange().GetLowerBoundValue());
}

float UMovieSceneWwiseGameParameterSection::GetEndTime() const
{
	FFrameRate FrameRate = GetTypedOuter<UMovieScene>()->GetTickResolution();
	return (float)FrameRate.AsSeconds(GetRange().GetUpperBoundValue());
}

void UMovieSceneWwiseGameParameterSection::PostLoad()
{
	Super::PostLoad();
	const int32 AkVersion = GetLinkerCustomVersion(FAkCustomVersion::GUID);

	if (AkVersion < FAkCustomVersion::NewGameParameterTrackDataContainer)
	{

		if (FloatCurve.GetDefaultValue() != MAX_flt)
		{
			GameParameterChannel.SetDefault(FloatCurve.GetDefaultValue());
		}

		GameParameterChannel.PreInfinityExtrap = FloatCurve.PreInfinityExtrap;
		GameParameterChannel.PostInfinityExtrap = FloatCurve.PostInfinityExtrap;

		TArray<FFrameNumber> Times;
		TArray<FMovieSceneFloatValue> Values;
		Times.Reserve(FloatCurve.GetNumKeys());
		Values.Reserve(FloatCurve.GetNumKeys());

		const FFrameRate LegacyFrameRate = GetLegacyConversionFrameRate();
		const float      Interval = LegacyFrameRate.AsInterval();

		int32 Index = 0;
		for (auto It = FloatCurve.GetKeyIterator(); It; ++It)
		{
			const FRichCurveKey& Key = *It;

			FFrameNumber KeyTime = UpgradeLegacyMovieSceneTime(nullptr, LegacyFrameRate, It->Time);

			FMovieSceneFloatValue NewValue;
			NewValue.Value = Key.Value;
			NewValue.InterpMode = Key.InterpMode;
			NewValue.TangentMode = Key.TangentMode;
			NewValue.Tangent.ArriveTangent = Key.ArriveTangent * Interval;
			NewValue.Tangent.LeaveTangent = Key.LeaveTangent  * Interval;
			ConvertInsertAndSort<FMovieSceneFloatValue>(Index++, KeyTime, NewValue, Times, Values);
		}

		GameParameterChannel.Set(Times, Values);
		return;
	}

	FloatChannelSerializationHelper.ToFloatChannel(GameParameterChannel);
}

void UMovieSceneWwiseGameParameterSection::Serialize(FArchive& Ar)
{
	FloatChannelSerializationHelper = GameParameterChannel;
	Ar.UsingCustomVersion(FAkCustomVersion::GUID);
	Super::Serialize(Ar);
}