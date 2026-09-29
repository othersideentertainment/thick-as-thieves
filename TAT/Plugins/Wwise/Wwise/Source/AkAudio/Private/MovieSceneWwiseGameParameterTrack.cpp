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

#include "MovieSceneWwiseGameParameterTrack.h"
#include "AkAudioDevice.h"

#include "IMovieScenePlayer.h"
#include "MovieSceneCommonHelpers.h"

#include "MovieSceneWwiseGameParameterSection.h"

#include "MovieSceneWwiseGameParameterTemplate.h"

FMovieSceneEvalTemplatePtr UMovieSceneWwiseGameParameterTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	return FMovieSceneWwiseGameParameterTemplate(*CastChecked<UMovieSceneWwiseGameParameterSection>(&InSection));
}

UMovieSceneSection* UMovieSceneWwiseGameParameterTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneWwiseGameParameterSection::StaticClass(), NAME_None, RF_Transactional);
}

#if WITH_EDITORONLY_DATA
FText UMovieSceneWwiseGameParameterTrack::GetDisplayName() const
{
	auto AllSections = GetAllSections();
	FString DisplayName("WwiseGameParameter");
	if (AllSections.Num() > 0)
	{
		UMovieSceneWwiseGameParameterSection* RTPCSection = CastChecked<UMovieSceneWwiseGameParameterSection>(AllSections[0]);
		DisplayName += TEXT(" - ") + (RTPCSection->GetGameParameter() ? RTPCSection->GetGameParameter()->GetName() : TEXT("Unassigned"));
	}

	if (AllSections.Num() > 1)
	{
		DisplayName += TEXT(" and more.");
	}

	return FText::FromString(DisplayName);
}
#endif

FName UMovieSceneWwiseGameParameterTrack::GetTrackName() const
{
	const auto Section = CastChecked<UMovieSceneWwiseGameParameterSection>(MovieSceneHelpers::FindNearestSectionAtTime(Sections, 0));
	return (Section != nullptr && Section->GetGameParameter()) ? FName(*Section->GetGameParameter()->GetName()) : FName(NAME_None);
}

bool UMovieSceneWwiseGameParameterTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UMovieSceneWwiseGameParameterSection::StaticClass();
}

