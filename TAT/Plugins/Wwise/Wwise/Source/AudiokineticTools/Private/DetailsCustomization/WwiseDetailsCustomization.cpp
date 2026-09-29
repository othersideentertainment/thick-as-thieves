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

#include "WwiseDetailsCustomization.h"
#include "DetailLayoutBuilder.h"
#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "AudiokineticTools"

FWwiseDetailsCustomization::FWwiseDetailsCustomization()
{
}

TSharedRef<IDetailCustomization> FWwiseDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FWwiseDetailsCustomization());
}

void FWwiseDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> CustomObjects;
	DetailLayout.GetObjectsBeingCustomized( CustomObjects );

	for(const auto& CustomObject : CustomObjects)
	{
		if(!CustomObject.IsValid()) continue;
		UClass* Class = CustomObject->GetClass();

		for (TPropertyValueIterator<FProperty> It(Class, CustomObject.Get(), EPropertyValueIteratorFlags::FullRecursion, EFieldIteratorFlags::ExcludeDeprecated); It; ++It)
		{
			const FProperty* Property = It.Key();

			bool bShowProperty = true;
			{
				static const FName MinUnrealFName("MinUnrealVersion"), MinWwiseFName("MinWwiseVersion"),
					MaxUnrealFName("MaxUnrealVersion"), MaxWwiseFName("MaxWwiseVersion");
				static constexpr int CurrentUnrealVersion[3] { ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, ENGINE_PATCH_VERSION };
				static constexpr int CurrentWwiseVersion[3] { AK_WWISE_SOUNDENGINE_MAJOR_VERSION, AK_WWISE_SOUNDENGINE_MINOR_VERSION, AK_WWISE_SOUNDENGINE_SUBMINOR_VERSION };

				if (Property->HasMetaData(MinUnrealFName))
				{
					const auto MinUnreal = Property->GetMetaData(MinUnrealFName);
					int RequestedVersion[3];
					GetVersionFromString(RequestedVersion, MinUnreal, 0);
					bShowProperty = bShowProperty && LhsVersionSmallerEqual(RequestedVersion, CurrentUnrealVersion);

				}
				if (Property->HasMetaData(MinWwiseFName))
				{
					const auto MinWwise = Property->GetMetaData(MinWwiseFName);
					int RequestedVersion[3];
					GetVersionFromString(RequestedVersion, MinWwise, 0);
					bShowProperty = bShowProperty && LhsVersionSmallerEqual(RequestedVersion, CurrentWwiseVersion);
				}
				if (Property->HasMetaData(MaxUnrealFName))
				{
					const auto MaxUnreal = Property->GetMetaData(MaxUnrealFName);
					int RequestedVersion[3];
					GetVersionFromString(RequestedVersion, MaxUnreal, 65536);
					bShowProperty = bShowProperty && LhsVersionSmallerEqual(CurrentUnrealVersion, RequestedVersion);
				}
				if (Property->HasMetaData(MaxWwiseFName))
				{
					const auto MaxWwise = Property->GetMetaData(MaxWwiseFName);
					int RequestedVersion[3];
					GetVersionFromString(RequestedVersion, MaxWwise, 65536);
					bShowProperty = bShowProperty && LhsVersionSmallerEqual(CurrentWwiseVersion, RequestedVersion);
				}
			}

			if (!bShowProperty)
			{
				// Get property path in x.y.z format for DetailLayout's GetProperty
				TArray<const FProperty*> PropertyChain;
				It.GetPropertyChain(PropertyChain);

				TStringBuilder<256> PropertyFullPath;
				for (auto Num{ PropertyChain.Num() - 1 }; Num >= 0; --Num)
				{
					const auto*& RecursedProperty{ PropertyChain[Num] };
					const auto RecursedPropertyName{ RecursedProperty->GetName() };
					
					PropertyFullPath.Append(RecursedPropertyName);

					if (Num != 0)
					{
						PropertyFullPath.AppendChar('.');
					}
				}
				const FName PropertyFullPathFName(PropertyFullPath.ToString());
				DetailLayout.HideProperty(PropertyFullPathFName, Class);
			}
		}
	}
}

void FWwiseDetailsCustomization::GetVersionFromString(int OutVersion[3], const FString& VersionString, int NoneValue)
{
	OutVersion[0] = NoneValue; OutVersion[1] = NoneValue; OutVersion[2] = NoneValue;
	TCHAR *End;

	const uint64 Major = FCString::Strtoui64(*VersionString, &End, 10);
	if(Major < MAX_uint16)
	{
		OutVersion[0] = static_cast<int>(Major);
	}
	if (*(End++) != '.')
	{
		return;
	}

	const uint64 Minor = FCString::Strtoui64(End, &End, 10);
	if(Minor < MAX_uint16)
	{
		OutVersion[1] = static_cast<int>(Minor);
	}
	if (*(End++) != '.')
	{
		return;
	}

	const uint64 Patch = FCString::Strtoui64(End, &End, 10);
	if(Patch < MAX_uint16)
	{
		OutVersion[2] = static_cast<int>(Patch);
	}
}

bool FWwiseDetailsCustomization::LhsVersionSmallerEqual(const int Lhs[3], const int Rhs[3])
{
	return Lhs[0] <= Rhs[0] || (Lhs[0] == Rhs[0]
		&& (Lhs[1] <= Rhs[1] || (Lhs[1] == Rhs[1]
			&& Lhs[2] <= Rhs[2])));
}


#undef LOCTEXT_NAMESPACE
