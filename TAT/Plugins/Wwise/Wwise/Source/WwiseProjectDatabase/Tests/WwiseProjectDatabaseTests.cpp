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



#ifdef TESTING_WWISE_PROJECT_DATABASE
#include <catch2/catch.hpp>
#include "Metadata/WwiseMetadataEvent.h"
#include "Metadata/WwiseMetadataLoader.h"
#include "Metadata/WwiseMetadataPlatformInfo.h"
#include "Metadata/WwiseMetadataProjectInfo.h"
#include "Metadata/WwiseMetadataRootFile.h"
#include "Metadata/WwiseMetadataSoundBanksInfo.h"
#else
#include "Wwise/Metadata/WwiseMetadataEvent.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"
#include "Wwise/Metadata/WwiseMetadataPlatformInfo.h"
#include "Wwise/Metadata/WwiseMetadataProjectInfo.h"
#include "Wwise/Metadata/WwiseMetadataRootFile.h"
#include "Wwise/Metadata/WwiseMetadataSoundBanksInfo.h"
#endif
#include "Wwise/AdapterTypes/Unreal/WwiseProjectDatabaseTestingUtils.h"
#include "WwiseDefines.h"

class WwiseMetadataTestLoadable : public WwiseMetadataLoadable
{
public:
	WwiseMetadataTestLoadable() : WwiseMetadataLoadable(){}

	WwiseMetadataTestLoadable(const WwiseMetadataLoader&  Loader){}
};

class WwiseMetadataTestStringLoadable : public WwiseMetadataLoadable
{
public:
	WwiseDBString String;
	WwiseMetadataTestStringLoadable() : WwiseMetadataLoadable(){}

	WwiseMetadataTestStringLoadable(WwiseMetadataLoader&  Loader) : String(Loader.GetString(this, "StringRecursive"_wwise_db))
	{
	}
};

bool NearlyEqual(float Expected, float Actual, float Epsilon = 0.0001f)
{
	return Actual <= Expected + Epsilon && Actual >= Actual - Epsilon;
}


bool MetadataBasicReferenceCheck(const WwiseMetadataBasicReference* Actual, const WwiseMetadataBasicReference& Expected)
{
	return Actual->Id == Expected.Id &&
			Actual->Name == Expected.Name &&
			Actual->ObjectPath == Expected.ObjectPath &&
			Actual->GUID == Expected.GUID &&
			Actual->Color == Expected.Color;
}

SCENARIO("Wwise::WwiseProjectDatabase::JSONReader")
{
	//Json files with at least one of each type (int, float, bool, String, Guid, Array & Object)
	SECTION("Json Operations")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseProjectDatabaseLoggingUtils::AddExpectedError("Could not retrieve field", 2);
		WwiseDBString JsonFile = R"({
			"String": "TestString",
			"Bool": "false",
			"Int": "42",
			"GUIDTest": "{ABCDDCBA-1234-4321-1357-BA9876543210}",
			"Float": "905.9827",
			"Array": [
			 {
				"StringRecursive": "Recursive1"
			 },
			 {
				"StringRecursive": "Recursive2"
			 }
		    ],
			"Object": {
				"StringRecursive": "Recursive3"
			},
			"BadType": "false"
			}
		)"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataTestLoadable Ref;
			//Getting a Bool
			auto Bool = MetadataLoader.GetBool(&Ref, "Bool"_wwise_db);
			CHECK(!Bool);
			//Getting an int
			auto Int = MetadataLoader.GetWwiseShortId(&Ref, "Int"_wwise_db);
			CHECK(Int == 42);
			//Getting a Float
			auto Float = MetadataLoader.GetFloat(&Ref, "Float"_wwise_db);
			CHECK(NearlyEqual(905.9827f, Float));
			//Getting a String
			auto String = MetadataLoader.GetString(&Ref, "String"_wwise_db);
			CHECK(String == "TestString"_wwise_db);
			//Getting a Guid
			auto Guid = MetadataLoader.GetGuid(&Ref, "GUIDTest"_wwise_db);
			CHECK(Guid == WwiseDBGuid("{ABCDDCBA-1234-4321-1357-BA9876543210}"_wwise_db));
			//Getting an Array.
			auto Array = MetadataLoader.GetArray<WwiseMetadataTestStringLoadable>(&Ref, "Array"_wwise_db);
			CHECK(Array.Size() == 2);
			for(unsigned int i = 0; i < Array.Size(); i++)
			{
				CHECK(Array[i].String == WwiseDBString(("Recursive" + std::to_string(i + 1)).c_str()));
			}
			//Getting a custom Object
			auto Object = MetadataLoader.GetLoaderObject<WwiseMetadataTestStringLoadable>(&Ref, "Object"_wwise_db);
			CHECK(Object.String == "Recursive3"_wwise_db);

			//Getting an existing field with the wrong type
			WwiseProjectDatabaseLoggingUtils::AddExpectedError("Invalid GUID");
			MetadataLoader.GetGuid(&Ref, "BadType"_wwise_db);

			//Getting a field that does not exist
			MetadataLoader.GetBool(&Ref, "NoField"_wwise_db);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("Json Optional Operations")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
			"IntMadatory": "42"
		})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataTestLoadable Ref;
			//Getting a Bool
			MetadataLoader.GetBool(&Ref, "Bool"_wwise_db, WwiseRequiredMetadata::Optional);
			//Getting an int
			MetadataLoader.GetWwiseShortId(&Ref, "Int"_wwise_db, WwiseRequiredMetadata::Optional);
			//Getting a Float
			MetadataLoader.GetFloat(&Ref, "Float"_wwise_db, WwiseRequiredMetadata::Optional);
			//Getting a String
			auto String = MetadataLoader.GetString(&Ref, "String"_wwise_db, WwiseRequiredMetadata::Optional);
			//Getting a Guid
			auto Guid = MetadataLoader.GetGuid(&Ref, "GUIDTest"_wwise_db, WwiseRequiredMetadata::Optional);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("Bad JSON File")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
			"String": "TestString",
			"Bool": "false",
			"Int": "42",
			"GUIDTest": "{ABCDDCBA-1234-4321-1357-BA9876543210}",
			"Float": "905.9827",
			"Array": [
			 {
				"StringRecursive": "Recursive1"
			 },
			 {
				"StringRecursive": "Recursive2"
			 }
		    ],
			"Object": {
				"StringRecursive": "Recursive3"
			},
		)"_wwise_db;
		WwiseProjectDatabaseLoggingUtils::AddExpectedError("Error while decoding json");
		WwiseDBJsonObject RootJsonObject;
		CHECK(!WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject));
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("ProjectInfo.json")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
			 "ProjectInfo": {
			  "Project": {
			   "Name": "WwiseDemoGame",
			   "GUID": "{F267EFB3-7242-487B-A715-85639DE7E4F1}",
			   "Generator": "2024.1.0.8404"
			  },
			  "CacheRoot": "../.cache",
			  "Platforms": [
			   {
			    "Name": "Android",
			    "GUID": "{A2D401DE-B8B6-4FEB-8142-137C34D507CB}",
			    "BasePlatform": "Android",
			    "BasePlatformGUID": "{A2D401DE-B8B6-4FEB-8142-137C34D507CA}",
			    "Path": "Android"
			   },
			   {
			    "Name": "iOS",
			    "GUID": "{7CC0D198-A107-4127-806F-AB305E44D9BA}",
			    "BasePlatform": "iOS",
			    "BasePlatformGUID": "{ECE03DB4-F948-462D-B2BB-A9173012B1F8}",
			    "Path": "iOS"
			   },
			   {
			    "Name": "Linux",
			    "GUID": "{BD0BDF13-3125-454F-8BFD-319537169F82}",
			    "BasePlatform": "Linux",
			    "BasePlatformGUID": "{BD0BDF13-3125-454F-8BFD-319537169F81}",
			    "Path": "Linux"
			   },
			   {
			    "Name": "Mac",
			    "GUID": "{02DC7702-6E7B-4AE4-BAE4-64D2B1057150}",
			    "BasePlatform": "Mac",
			    "BasePlatformGUID": "{9C6217D5-DD11-4795-87C1-6CE02853C540}",
			    "Path": "Mac"
			   },
			   {
			    "Name": "PS4",
			    "GUID": "{3AF9B9B6-6EF1-47E9-B5FE-E30C9E602C78}",
			    "BasePlatform": "PS4",
			    "BasePlatformGUID": "{3AF9B9B6-6EF1-47E9-B5FE-E30C9E602C77}",
			    "Path": "PS4"
			   },
			   {
			    "Name": "PS5",
			    "GUID": "{8AA4C8FA-1359-4987-839C-5D6E9DD9C54D}",
			    "BasePlatform": "PS5",
			    "BasePlatformGUID": "{662A5E67-9D35-48DA-B6A8-B77C7F1D84E0}",
			    "Path": "PS5"
			   },
			   {
			    "Name": "Switch",
			    "GUID": "{28A989E5-3495-4CCC-B983-8A1CF8678A81}",
			    "BasePlatform": "Switch",
			    "BasePlatformGUID": "{874F26D2-416D-4698-BFB6-3427CAFCFF9C}",
			    "Path": "Switch"
			   },
			   {
			    "Name": "Windows",
			    "GUID": "{6E0CB257-C6C8-4C5C-8366-2740DFC441EC}",
			    "BasePlatform": "Windows",
			    "BasePlatformGUID": "{6E0CB257-C6C8-4C5C-8366-2740DFC441EB}",
			    "Path": "Windows"
			   },
			   {
			    "Name": "XboxOneGDK",
			    "GUID": "{B131584B-9961-4BB5-9C58-A3E9ABFFBBF7}",
			    "BasePlatform": "XboxOne",
			    "BasePlatformGUID": "{B131584B-9961-4BB5-9C58-A3E9ABFFBBF6}",
			    "Path": "XboxOneGDK"
			   },
			   {
			    "Name": "XboxSeriesX",
			    "GUID": "{45A3EADA-793D-4E70-A0F1-D51473A8F352}",
			    "BasePlatform": "XboxSeriesX",
			    "BasePlatformGUID": "{26352FC0-7716-4F97-8DAF-3665EC2BB501}",
			    "Path": "XboxSeriesX"
			   }
			  ],
			  "Languages": [
			   {
			    "Name": "English",
			    "Id": "684519430",
			    "GUID": "{15508E7D-8FFD-47C0-959D-A576D1BDD1B4}",
			    "Default": "true",
			    "UseAsStandIn": "true"
			   },
			   {
			    "Name": "French",
			    "Id": "323458483",
			    "GUID": "{7F8A24DD-4DB2-4F3C-AAF3-8015405B8652}"
			   }
			  ],
			  "FileHash": "{7E7F4E7D-2884-6334-BE9C-024DBC4BC4FB}"
			 }
			})"_wwise_db;

		//Reading a ProjectInfo.json
		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataRootFile Ref(MetadataLoader);
			auto PlatformArray = Ref.ProjectInfo->Platforms;
			CHECK(PlatformArray.Size() == 10);

			auto LanguageArray = Ref.ProjectInfo->Languages;
			CHECK(LanguageArray.Size() == 2);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("PlatformInfo.json")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
				"PlatformInfo": {
				 "Platform": {
				  "Name": "Windows",
				  "BasePlatform": "Windows",
				  "Generator": "2024.1.1.8680"
				 },
				 "RootPaths": {
				  "ProjectRoot": "../..",
				  "SourceFilesRoot": "../../.cache",
				  "SoundBanksRoot": ".",
				  "ExternalSourcesInputFile": "../../ExternalSources/UnrealExternalSources.wsources",
				  "ExternalSourcesOutputRoot": "../../GeneratedExternalSources/Windows"
				 },
				 "DefaultAlign": "16",
				 "Settings": {
				  "AutoSoundBankDefinition": "true",
				  "CopyLooseStreamedMediaFiles": "true",
				  "SubFoldersForGeneratedFiles": "true",
				  "RemoveUnusedGeneratedFiles": "true",
				  "SourceControlGeneratedFiles": "false",
				  "GenerateHeaderFile": "false",
				  "GenerateContentTxtFile": "true",
				  "GenerateMetadataXML": "false",
				  "GenerateMetadataJSON": "true",
				  "GenerateAllBanksMetadata": "false",
				  "GeneratePerBankMetadata": "true",
				  "UseSoundBankNames": "true",
				  "AllowExceedingMaxSize": "false",
				  "MaxAttenuationInfo": "true",
				  "EstimatedDurationInfo": "true",
				  "PrintObjectGuid": "true",
				  "PrintObjectPath": "true",
				  "PrintObjectColor": "true"
				 },
				 "FileHash": "{D7DE7818-9875-FDC6-6F28-59CCC3F529AE}"
				}
})"_wwise_db;

		//Reading a PlatformInfo.json
		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataRootFile Ref(MetadataLoader);
			CHECK(Ref.PlatformInfo);
			if (Ref.PlatformInfo)
			{
				auto PlatformName = Ref.PlatformInfo->Platform.Name;
				CHECK(PlatformName ==  "Windows"_wwise_db);
			}
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("WwiseEvent")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"(
			{
			 "SoundBanksInfo": {
			  "Platform": "Windows",
			  "BasePlatform": "Windows",
			  "SchemaVersion": "16",
			  "SoundBankVersion": "150",
			  "SoundBanks": [
			   {
			    "Id": "1730369216",
			    "Type": "Event",
			    "GUID": "{7982C17E-7EBF-4DB1-9CFB-B221A1526EFE}",
			    "Color": "14",
			    "Language": "SFX",
			    "Hash": "{6F5EECEF-F17F-B4E3-0E99-F197C1031074}",
			    "ObjectPath": "Fire_Weapon",
			    "ShortName": "Fire_Weapon",
			    "Path": "Event/17/Fire_Weapon.bnk",
			    "Media": [
			     {
			      "Id": "903830747",
			      "Language": "SFX",
			      "Streaming": "false",
			      "Location": "Loose",
			      "ShortName": "Shotgun_Fire_01.wav",
			      "Path": "Media/90/903830747.wem",
			      "CachePath": "A0/{A099155E-66E0-7F2C-FAD9-6924634AECCF}.wem"
			  
			     }
			    ],
			    "Events": [
			     {
			      "Id": "1730369216",
			      "Name": "Fire_Weapon",
			      "ObjectPath": "\\Events\\Default Work Unit\\Fire_Weapon",
			      "GUID": "{7982C17E-7EBF-4DB1-9CFB-B221A1526EFE}",
			      "Color": "13",
			      "DurationType": "OneShot",
			      "DurationMin": "1.3388333",
			      "DurationMax": "1.3388333",
			      "MediaRefs": [
			       {
			        "Id": "903830747"
			       }
			      ]
			     }
			    ]
			   }
			  ],
			  "FileHash": "{344A49A4-9EAD-4C8D-98C7-3E088C0E462A}"
			 }
			})"_wwise_db;

		//Reading an Event JSON
		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataRootFile Ref(MetadataLoader);
			CHECK(Ref.SoundBanksInfo->Platform ==  "Windows"_wwise_db);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("InitBank")
	{
		// This test string needs to be broken up because of VS compiler error C2026
		WwiseDBString JsonFile = R"(
{
 "SoundBanksInfo": {
  "Platform": "Windows",
  "BasePlatform": "Windows",
  "SchemaVersion": "16",
  "SoundBankVersion": "150",
  "SoundBanks": [
   {
    "Id": "1355168291",
    "Type": "User",
    "GUID": "{701ECBBD-9C7B-4030-8CDB-749EE5D1C7B9}",
    "Language": "SFX",
    "Hash": "{615997BC-9DD8-244F-E4DA-E36233127E61}",
    "ObjectPath": "Init",
    "ShortName": "Init",
    "Path": "Init.bnk",
    "Plugins": {
     "Custom": [
      {
       "Id": "1008723532",
       "Name": "Rhythmic_Talk Define custom",
       "ObjectPath": "\\Master-Mixer Hierarchy\\Default Work Unit\\Master Audio Bus\\Tremolo\\[Effect Slot [0]: (Tremolo)]\\Rhythmic_Talk Define custom",
       "GUID": "{9AA194DD-6AC1-41FC-B2F2-49B8FFF0424B}",
       "LibName": "Wwise Tremolo",
       "LibId": "8585219"
      }
     ],
     "ShareSets": [
      {
       "Id": "1958819083",
       "Name": "SpatialAudio_Room_Medium",
       "ObjectPath": "\\Effects\\Default Work Unit\\SpatialAudio_Room_Medium",
       "GUID": "{05B23C85-8D32-420B-B481-1199CA275683}",
       "LibName": "Wwise RoomVerb",
       "LibId": "7733251"
      }
     ],
     "AudioDevices": [
      {
       "Id": "530303819",
       "Name": "Communication",
       "ObjectPath": "\\Audio Devices\\Default Work Unit\\Communication",
       "GUID": "{21ED38DD-4A4C-47AA-9A7C-B33DD9DB88B4}",
       "LibName": "Communication",
       "LibId": "11534343"
      },
      {
       "Id": "1334442663",
       "Name": "Controller_Speaker",
       "ObjectPath": "\\Audio Devices\\Default Work Unit\\Controller_Speaker",
       "GUID": "{2F8AC0FA-6FE4-4419-A35D-2DF92946DDD9}",
       "LibName": "Controller Speaker",
       "LibId": "11730951"
      },
      {
       "Id": "2317455096",
       "Name": "No_Output",
       "ObjectPath": "\\Audio Devices\\Default Work Unit\\No_Output",
       "GUID": "{B70DEED6-A809-454C-877E-4203E9BBF06A}",
       "LibName": "No Output",
       "LibId": "11862023"
      },
      {
       "Id": "3859886410",
       "Name": "System",
       "ObjectPath": "\\Audio Devices\\Default Work Unit\\System",
       "GUID": "{1B52A9AD-EDA1-4EAE-9D73-FEC868B8024F}",
       "LibName": "System",
       "LibId": "11403271"
      }
     ]
    },
    "Busses": [
     {
      "Id": "805203703",
      "Name": "Master Secondary Bus",
      "ObjectPath": "\\Master-Mixer Hierarchy\\Default Work Unit\\Master Secondary Bus",
      "GUID": "{D2E0BC7E-7B3F-4FFE-A670-FAE02761B631}"
     },
     {
      "Id": "3803692087",
      "Name": "Master Audio Bus",
      "ObjectPath": "\\Master-Mixer Hierarchy\\Default Work Unit\\Master Audio Bus",
      "GUID": "{1514A4D8-1DA6-412A-A17E-75CA0C2149F3}",
      "PluginRefs": {
       "ShareSets": [
        {
         "Id": "2589936646"
        }
       ]
      }
     }
    ],
    "AuxBusses": [
     {
      "Id": "187046019",
      "Name": "LargeRoom",
      "ObjectPath": "\\Master-Mixer Hierarchy\\Default Work Unit\\Master Audio Bus\\LargeRoom",
      "GUID": "{9BF4EB38-0C67-4347-8DB8-926791DD6ED3}",
      "PluginRefs": {
       "ShareSets": [
        {
         "Id": "1958819083"
        }
       ]
      }
     },
     {
      "Id": "243379636",
      "Name": "Reflect",
      "ObjectPath": "\\Master-Mixer Hierarchy\\Default Work Unit\\Master Audio Bus\\Reflect",
      "GUID": "{708C66AF-E74A-4237-9D06-364F73EE9484}",
      "MaxAttenuation": "5000"
     },
     {
      "Id": "348963605",
      "Name": "Reverb",
      "ObjectPath": "\\Master-Mixer Hierarchy\\Default Work Unit\\Master Audio Bus\\Reverb",
      "GUID": "{6F049A6E-9C36-4EC1-8DE9-C51380CD6E46}",
      "PluginRefs": {
       "Custom": [
        {
         "Id": "676782335"
        }
       ]
      }
     },
     {
      "Id": "1591283508",
      "Name": "DefaultReverb",
      "ObjectPath": "\\Master-Mixer Hierarchy\\Default Work Unit\\Master Audio Bus\\DefaultReverb",
      "GUID": "{DD509A41-F55B-4D8E-9D8B-F06ABD7AB4BD}",
      "PluginRefs": {
       "Custom": [
        {
         "Id": "1008318817"
        }
       ]
      }
     },
     {
      "Id": "2933838247",
      "Name": "SmallRoom",
      "ObjectPath": "\\Master-Mixer Hierarchy\\Default Work Unit\\Master Audio Bus\\SmallRoom",
      "GUID": "{2E59DD93-305A-4768-B48F-D3193F7C6F1C}",
      "PluginRefs": {
       "ShareSets": [
        {
         "Id": "2120793899"
        }
       ]
      }
     },
	 {
      "Id": "4056223263",
      "Name": "Tremolo",
      "ObjectPath": "\\Master-Mixer Hierarchy\\Default Work Unit\\Master Audio Bus\\Tremolo",
      "GUID": "{1BAB1064-9738-431C-9A25-293E0BC0888D}",
      "PluginRefs": {
       "Custom": [
        {
         "Id": "1008723532"
        }
       ]
      }
     }
    ],
    "GameParameters": [
     {
      "Id": "194404061",
      "Name": "PreDelay",
      "ObjectPath": "\\Game Parameters\\Default Work Unit\\PreDelay",
      "GUID": "{BDCEC39D-FCA8-40DF-B620-A3BBDA354F92}"
     },
     {
      "Id": "764238081",
      "Name": "DrumKitModulation",
      "ObjectPath": "\\Game Parameters\\Default Work Unit\\DrumKitModulation",
      "GUID": "{D8D07A44-750C-43F3-BEE4-7C5B0EB9B539}"
     },
     {
      "Id": "772397858",
      "Name": "TestCurrentVolume",
      "ObjectPath": "\\Game Parameters\\AkIntegrationTests\\TestCurrentVolume",
      "GUID": "{E6D6D775-B3F3-4D3E-AE1A-4746BBFBB40D}"
     }
    ],
    "StateGroups": [
     {
      "Id": "3978303460",
      "Name": "TestState",
      "ObjectPath": "\\States\\Default Work Unit\\TestState",
      "GUID": "{6476B68F-10E7-42BD-8090-40A54CEED6CA}",
      "States": [
       {
        "Id": "748895195",
        "Name": "None",
        "ObjectPath": "\\States\\Default Work Unit\\TestState\\None",
        "GUID": "{801C436E-66C8-4348-BB58-1398D6799B34}"
       },
       {
        "Id": "2021425847",
        "Name": "New_State",
        "ObjectPath": "\\States\\Default Work Unit\\TestState\\New_State",
        "GUID": "{90EAE6ED-DD96-49DA-8780-12E966F33BC0}"
       }
      ]
     }
    ],
    "SwitchGroups": [
     {
      "Id": "858531997",
      "Name": "Frequency",
      "ObjectPath": "\\Switches\\Default Work Unit\\Frequency",
      "GUID": "{EEBAA75B-9E94-4EAF-A9DD-4F38ED4D5C8D}",
      "Switches": [
       {
        "Id": "545371365",
        "Name": "Low",
        "ObjectPath": "\\Switches\\Default Work Unit\\Frequency\\Low",
        "GUID": "{7690228D-2131-4E18-BF05-6B16F5105B92}"
       },
       {
        "Id": "3550808449",
        "Name": "High",
        "ObjectPath": "\\Switches\\Default Work Unit\\Frequency\\High",
        "GUID": "{8804A8FD-746F-4F88-A8B8-C1E2E9F3AB8F}"
       }
      ]
     },
     {
      "Id": "1117085073",
      "Name": "Character_Type",
      "ObjectPath": "\\Switches\\Default Work Unit\\Character_Type",
      "GUID": "{6FB99E68-2844-4FF1-BDF7-DDACBC8C849F}",
      "Switches": [
       {
        "Id": "1930240631",
        "Name": "Robot",
        "ObjectPath": "\\Switches\\Default Work Unit\\Character_Type\\Robot",
        "GUID": "{9B3E427D-0633-4BC9-B1ED-E5B4447D0523}"
       },
       {
        "Id": "3887404748",
        "Name": "Human",
        "ObjectPath": "\\Switches\\Default Work Unit\\Character_Type\\Human",
        "GUID": "{59F60F2F-4E3D-443F-A89E-2ABE34E666E5}"
       }
      ]
     }
    ],
    "AcousticTextures": [
     {
      "Id": "1970351858",
      "Name": "Fabric",
      "ObjectPath": "\\Virtual Acoustics\\Factory Acoustic Textures\\Textures\\Fabric",
      "GUID": "{8381D532-DB6E-4EED-A7A3-BC7B0A0B8BA8}",
      "Properties": [
       {
        "Name": "AbsorptionLow",
        "Type": "Real32",
        "Value": "3"
       },
       {
        "Name": "AbsorptionMidLow",
        "Type": "Real32",
        "Value": "13"
       },
       {
        "Name": "AbsorptionMidHigh",
        "Type": "Real32",
        "Value": "48"
       },
       {
        "Name": "AbsorptionHigh",
        "Type": "Real32",
        "Value": "72"
       })"
#if WWISE_2023_1_OR_LATER
		R"(
,
       {
        "Name": "Scattering",
        "Type": "Real32",
        "Value": "0"
       }
	)"
#endif
	R"(
      ]
     }
    ]
   }
  ],
  "FileHash": "{8124FAE6-2829-1DEC-36EC-87E839A8B188}"
 }
})"_wwise_db;

		//Reading an Init Bank
		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataRootFile Ref(MetadataLoader);
			//Making sure everything was loaded
			CHECK(Ref.SoundBanksInfo->Platform == "Windows"_wwise_db);
			CHECK(Ref.SoundBanksInfo->SoundBanks.Size() == 1);
			CHECK(Ref.SoundBanksInfo->SoundBanks[0].Triggers.Size() == 0);
			CHECK(Ref.SoundBanksInfo->SoundBanks[0].AcousticTextures.Size() == 1);
			CHECK(Ref.SoundBanksInfo->SoundBanks[0].Busses.Size() == 2);
			CHECK(Ref.SoundBanksInfo->SoundBanks[0].AuxBusses.Size() == 6);
			CHECK(Ref.SoundBanksInfo->SoundBanks[0].GameParameters.Size() == 3);
			CHECK(Ref.SoundBanksInfo->SoundBanks[0].SwitchGroups.Size() == 2);
			CHECK(Ref.SoundBanksInfo->SoundBanks[0].StateGroups.Size() == 1);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataBasicReference")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
				WwiseDBString JsonFile = R"({
				        "Id": "123456789",
				        "Name": "ObjectName",
				        "ObjectPath": "\\A\\Path\\To\\Object",
				        "Color": "13",
				        "GUID": "{ABCDDCBA-1234-4321-1357-BA9876543210}"
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataBasicReference Ref(MetadataLoader);

			//Making sure the read Basic Reference have the expected values
			CHECK(MetadataBasicReferenceCheck(&Ref, WwiseMetadataBasicReference(123456789, "ObjectName"_wwise_db, "\\A\\Path\\To\\Object"_wwise_db, WwiseDBGuid("{ABCDDCBA-1234-4321-1357-BA9876543210}"_wwise_db), 13)));
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataBasicReference Missing Mandatory Field")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseProjectDatabaseLoggingUtils::AddExpectedError("Could not retrieve field", 1);
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
				        "Name": "ObjectName",
				        "ObjectPath": "\\A\\Path\\To\\Object"
						})"_wwise_db;

		//Reading a Metadata Reference with a missing field (throws an error)
		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataBasicReference Ref(MetadataLoader);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataBasicReference Missing Optional Field")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
				        "Name": "ObjectName",
				        "ObjectPath": "\\A\\Path\\To\\Object",
				        "GUID": "{ABCDDCBA-1234-4321-1357-BA9876543210}"
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataBasicReference Ref(MetadataLoader);

			// Making sure the read Basic Reference has the expected values, with the default 0 for missing "Color"
			CHECK(MetadataBasicReferenceCheck(&Ref, WwiseMetadataBasicReference(123456789, "ObjectName"_wwise_db, "\\A\\Path\\To\\Object"_wwise_db, WwiseDBGuid("{ABCDDCBA-1234-4321-1357-BA9876543210}"_wwise_db), 0)));
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataAcousticTexture")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
				        "Name": "ObjectName",
				        "ObjectPath": "\\A\\Path\\To\\Object",
				        "GUID": "{ABCDDCBA-1234-4321-1357-BA9876543210}",
					    "Properties": [
					      {
					       "Name": "AbsorptionLow",
					       "Type": "Real32",
					       "Value": "0"
					      },
					      {
					       "Name": "AbsorptionMidLow",
					       "Type": "Real32",
					       "Value": "1"
					      },
					      {
					       "Name": "AbsorptionMidHigh",
					       "Type": "Real32",
					       "Value": "2"
					      },
					      {
					       "Name": "AbsorptionHigh",
					       "Type": "Real32",
					       "Value": "3"
					      },
					      {
					       "Name": "Scattering",
					       "Type": "Real32",
					       "Value": "4"
					      }
					     ]
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataAcousticTexture Ref(MetadataLoader);

			//Making sure values read are as expected
			CHECK(MetadataBasicReferenceCheck(&Ref, WwiseMetadataBasicReference(123456789, "ObjectName"_wwise_db, "\\A\\Path\\To\\Object"_wwise_db, WwiseDBGuid("{ABCDDCBA-1234-4321-1357-BA9876543210}"_wwise_db))));
			CHECK(NearlyEqual(0, Ref.AbsorptionLow));
			CHECK(NearlyEqual(1, Ref.AbsorptionMidLow));
			CHECK(NearlyEqual(2, Ref.AbsorptionMidHigh));
			CHECK(NearlyEqual(3, Ref.AbsorptionHigh));
#if WWISE_2023_1_OR_LATER
			CHECK(NearlyEqual(4, Ref.Scattering));
#endif
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataBus With No Plugin Refs or AuxBusRefs")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
				        "Name": "ObjectName",
				        "ObjectPath": "\\A\\Path\\To\\Object",
				        "GUID": "{ABCDDCBA-1234-4321-1357-BA9876543210}"
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataBus Ref(MetadataLoader);

			//Making sure values read are as expected
			CHECK(MetadataBasicReferenceCheck(&Ref, WwiseMetadataBasicReference(123456789, "ObjectName"_wwise_db, "\\A\\Path\\To\\Object"_wwise_db, WwiseDBGuid("{ABCDDCBA-1234-4321-1357-BA9876543210}"_wwise_db))));
			CHECK(!Ref.PluginRefs);
			CHECK(Ref.AuxBusRefs.Size() == 0);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataBus With Plugin Refs")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
				        "Name": "ObjectName",
				        "ObjectPath": "\\A\\Path\\To\\Object",
				        "GUID": "{ABCDDCBA-1234-4321-1357-BA9876543210}",
						"PluginRefs": {
						"ShareSets": [
						  {
						   "Id": "0987654321"
						  }
						 ]
						}

						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataBus Ref(MetadataLoader);

			//Making sure values read are as expected
			CHECK(MetadataBasicReferenceCheck(&Ref, WwiseMetadataBasicReference(123456789, "ObjectName"_wwise_db, "\\A\\Path\\To\\Object"_wwise_db, WwiseDBGuid("{ABCDDCBA-1234-4321-1357-BA9876543210}"_wwise_db))));
			CHECK(Ref.PluginRefs->ShareSets.Size() == 1);
			CHECK(Ref.AuxBusRefs.Size() == 0);
			CHECK(Ref.PluginRefs->ShareSets[0].Id == 987654321);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataBus With AuxBus Refs")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
				        "Name": "ObjectName",
				        "ObjectPath": "\\A\\Path\\To\\Object",
				        "GUID": "{ABCDDCBA-1234-4321-1357-BA9876543210}",
				        "Color": "5",
					    "AuxBusRefs": [
					     {
					      "Id": "135798642",
					      "Name": "AuxBus",
					      "ObjectPath": "\\Path\\To\\AuxBus",
					      "GUID": "{DCBAABCD-5678-8765-2468-0123456789AB}"
					     }
					     ]
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataBus Ref(MetadataLoader);

			//Making sure values read are as expected
			CHECK(MetadataBasicReferenceCheck(&Ref, WwiseMetadataBasicReference(123456789, "ObjectName"_wwise_db, "\\A\\Path\\To\\Object"_wwise_db, WwiseDBGuid("{ABCDDCBA-1234-4321-1357-BA9876543210}"_wwise_db), 5)));
			CHECK(!Ref.PluginRefs);
			CHECK(Ref.AuxBusRefs.Size() == 1);
			CHECK(MetadataBasicReferenceCheck(&Ref.AuxBusRefs[0], WwiseMetadataBasicReference(135798642, "AuxBus"_wwise_db, "\\Path\\To\\AuxBus"_wwise_db, WwiseDBGuid("{DCBAABCD-5678-8765-2468-0123456789AB}"_wwise_db))));
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataBus With AuxBus Refs and Plugin Refs")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
				        "Name": "ObjectName",
				        "ObjectPath": "\\A\\Path\\To\\Object",
				        "GUID": "{ABCDDCBA-1234-4321-1357-BA9876543210}",
				        "Color": "8",
					    "AuxBusRefs": [
					     {
					      "Id": "135798642",
					      "Name": "AuxBus",
					      "ObjectPath": "\\Path\\To\\AuxBus",
					      "GUID": "{DCBAABCD-5678-8765-2468-0123456789AB}"
					     }
					     ],
						"PluginRefs": {
						"ShareSets": [
						  {
						   "Id": "0987654321"
						  }
						 ]
						}
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataBus Ref(MetadataLoader);

			//Making sure values read are as expected
			CHECK(MetadataBasicReferenceCheck(&Ref, WwiseMetadataBasicReference(123456789,  "ObjectName"_wwise_db,  "\\A\\Path\\To\\Object"_wwise_db, WwiseDBGuid("{ABCDDCBA-1234-4321-1357-BA9876543210}"_wwise_db), 8)));
			CHECK(Ref.AuxBusRefs.Size() == 1);
			CHECK(MetadataBasicReferenceCheck(&Ref.AuxBusRefs[0], WwiseMetadataBasicReference(135798642,  "AuxBus"_wwise_db,  "\\Path\\To\\AuxBus"_wwise_db, WwiseDBGuid("{DCBAABCD-5678-8765-2468-0123456789AB}"_wwise_db))));
			CHECK(Ref.PluginRefs->ShareSets.Size() == 1);
			CHECK(Ref.PluginRefs->ShareSets[0].Id == 987654321);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataMedia")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
						"Language": "SFX",
						"Streaming": "false",
						"Location": "Loose",
				        "ShortName": "MediaName.wav",
				        "Path": "\\A\\Path\\To\\Object",
				        "CachePath": "SFX/Media.wem"
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataMedia Ref(MetadataLoader);

			//Making sure values read are as expected
			CHECK(Ref.Id == 123456789);
			CHECK(Ref.Language ==  "SFX"_wwise_db);
			CHECK(!Ref.bStreaming);
			CHECK(Ref.Location == WwiseMetadataMediaLocation::Loose);
			CHECK(Ref.ShortName ==  "MediaName.wav"_wwise_db);
			CHECK(Ref.Path ==  "\\A\\Path\\To\\Object"_wwise_db);
			CHECK(Ref.CachePath ==  "SFX/Media.wem"_wwise_db);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataMedia Loose without Path")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseProjectDatabaseLoggingUtils::AddExpectedError("Could not retrieve field", 1);
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
						"Language": "SFX",
						"Streaming": "false",
						"Location": "Loose",
				        "ShortName": "MediaName.wav",
				        "Path": "",
				        "CachePath": "SFX/Media.wem"
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;

		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			//Making sure values read are as expected
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataMedia Ref(MetadataLoader);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataMedia in Memory and Streaming without Path")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseProjectDatabaseLoggingUtils::AddExpectedError("Could not retrieve field", 1);
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
						"Language": "SFX",
						"Streaming": "true",
						"Location": "Memory",
				        "ShortName": "MediaName.wav",
				        "Path": "",
				        "CachePath": "SFX/Media.wem"
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			//Making sure values read are as expected
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataMedia Ref(MetadataLoader);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataMedia in Memory, Streaming and with a Path")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseProjectDatabaseLoggingUtils::AddExpectedError("Could not retrieve field", 1);
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
						"Language": "SFX",
						"Streaming": "false",
						"Location": "Memory",
				        "ShortName": "MediaName.wav",
				        "Path": "\\A\\Path\\To\\Object",
				        "CachePath": "SFX/Media.wem"
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			//Making sure values read are as expected
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataMedia Ref(MetadataLoader);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("MetadataGroupValueReference")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"({
				        "Id": "123456789",
				        "Name": "ObjectName",
				        "ObjectPath": "\\A\\Path\\To\\Object",
				        "GUID": "{ABCDDCBA-1234-4321-1357-BA9876543210}",
						"GroupId": "135798642",
						"Color": "5"
						})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			//Making sure values read are as expected
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataGroupValueReference Ref(MetadataLoader);

			CHECK(MetadataBasicReferenceCheck(&Ref, WwiseMetadataBasicReference(123456789, "ObjectName"_wwise_db, "\\A\\Path\\To\\Object"_wwise_db, WwiseDBGuid("{ABCDDCBA-1234-4321-1357-BA9876543210}"_wwise_db), 5)));
			CHECK(Ref.GroupId == 135798642);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}

	SECTION("WwiseEvent with Switches")
	{
		WwiseProjectDatabaseLoggingUtils::ResetErrors();
		WwiseDBString JsonFile = R"(
			{
			    "Id": "1730369216",
			    "Type": "Event",
			    "GUID": "{7982C17E-7EBF-4DB1-9CFB-B221A1526EFE}",
			    "Language": "SFX",
			    "Hash": "{6F5EECEF-F17F-B4E3-0E99-F197C1031074}",
			    "ObjectPath": "Fire_Weapon",
			    "ShortName": "Fire_Weapon",
			    "Path": "Event/17/Fire_Weapon.bnk",
			    "Media": [
			     {
			      "Id": "903830747",
			      "Language": "SFX",
			      "Streaming": "false",
			      "Location": "Loose",
			      "ShortName": "Shotgun_Fire_01.wav",
			      "Path": "Media/90/903830747.wem",
			      "CachePath": "A0/{A099155E-66E0-7F2C-FAD9-6924634AECCF}.wem"
			     }
			    ],
			    "Events": [
			     {
			      "Id": "1730369216",
			      "Name": "Fire_Weapon",
			      "ObjectPath": "\\Events\\Default Work Unit\\Fire_Weapon",
			      "GUID": "{7982C17E-7EBF-4DB1-9CFB-B221A1526EFE}",
			      "Color": "13",
			      "DurationType": "OneShot",
			      "DurationMin": "1.3388333",
			      "DurationMax": "1.3388333",
			      "MediaRefs": [
			       {
			        "Id": "903830747"
			       }
			      ]
			     }
			    ],
			    "SwitchGroups": [
			     {
			      "Id": "1117085073",
			      "Name": "Character_Type",
			      "ObjectPath": "\\Switches\\Switch1",
			      "GUID": "{6FB99E68-2844-4FF1-BDF7-DDACBC8C849F}",
			      "Switches": [
			       {
			        "Id": "1930240631",
			        "Name": "Robot",
			        "ObjectPath": "\\Switches\\Switch1-1",
			        "GUID": "{9B3E427D-0633-4BC9-B1ED-E5B4447D0523}",
			        "Color": "24"
			       },
			       {
			        "Id": "3887404748",
			        "Name": "Human",
			        "ObjectPath": "\\Switches\\Switch1-2",
			        "GUID": "{59F60F2F-4E3D-443F-A89E-2ABE34E666E5}"
			       }
			      ]
			     }
			    ]
			})"_wwise_db;

		WwiseDBJsonObject RootJsonObject;
		bool bFileParsed = false;
		if (WwiseDBJsonObject::CreateJsonObject(JsonFile, RootJsonObject))
		{
			//Making sure values read are as expected
			bFileParsed = true;
			WwiseMetadataLoader MetadataLoader(RootJsonObject);
			WwiseMetadataSoundBank Ref(MetadataLoader);
			CHECK(Ref.SwitchGroups.Size() == 1);
			CHECK(Ref.SwitchGroups[0].Switches.Size() == 2);
		}
		CHECK(bFileParsed);
		WwiseProjectDatabaseLoggingUtils::ParseErrors();
	}
}