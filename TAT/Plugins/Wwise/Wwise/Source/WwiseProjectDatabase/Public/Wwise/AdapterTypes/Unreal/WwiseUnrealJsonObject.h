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

#include "Wwise/AdapterTypes/WwiseJsonObject.h"
#include <Dom/JsonObject.h>
#include <Serialization/JsonSerializer.h>
#include "Wwise/AdapterTypes/WwiseProjectDatabaseLogging.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealString.h"
#include "Wwise/Stats/ProjectDatabase.h"
#include "Misc/FileHelper.h"

class WwiseUnrealJsonObject : public IWwiseJsonObject<FJsonObject>
{
public:
	WwiseUnrealJsonObject(){}
	
	WwiseUnrealJsonObject(const WwiseUnrealString& FilePath)
	{
		FString FileContents;
		if (FFileHelper::LoadFileToString(FileContents, *FilePath))
		{
			auto JsonReader = TJsonReaderFactory<>::Create(FileContents);
			TSharedPtr<FJsonObject> RootJsonObject = MakeShared<FJsonObject>(this->JsonObject);
			if (!FJsonSerializer::Deserialize(JsonReader, RootJsonObject))
			{
				WWISE_DB_LOG(Error, "Error while decoding json");
			}
			this->JsonObject = *RootJsonObject;
			WwiseUnrealJsonObject Obj;
			bool res = TryGetObjectField(TEXT("ProjectInfo"), Obj);
		}
		else
		{
			WWISE_DB_LOG(Error, "Error while loading file %s to string", *FilePath);
		}

	}

	WwiseUnrealJsonObject(const TSharedPtr<FJsonObject>& Object)
	{
		this->JsonObject = *Object;
	}

	static bool CreateJsonObject(WwiseUnrealString& File, WwiseUnrealJsonObject& JsonObject)
	{
		auto JsonReader = TJsonReaderFactory<>::Create(MoveTemp(File.String));
		TSharedPtr<FJsonObject> RootJsonObject;
		if (!FJsonSerializer::Deserialize(JsonReader, RootJsonObject))
		{
			UE_LOG(LogWwiseProjectDatabase, Error, TEXT("Error while decoding json"));
			return {};
		}
		JsonObject.JsonObject = *RootJsonObject;	
		return true;
	}
	
	bool TryGetBoolField(const WwiseUnrealString& Name, bool& OutValue) const
	{
		return this->JsonObject.TryGetBoolField(Name.String, OutValue);
	}

	bool TryGetIntField(const WwiseUnrealString& Name, int& OutValue) const
	{
		return this->JsonObject.TryGetNumberField(Name.String, OutValue);
	}

	bool TryGetShortIdField(const WwiseUnrealString& Name, WwiseDBShortId& OutValue) const
	{
		return this->JsonObject.TryGetNumberField(Name.String, OutValue);
	}

	bool TryGetFloatField(const WwiseUnrealString& Name, float& OutValue) const
	{
		double Out;
		if(this->JsonObject.TryGetNumberField(Name.String, Out))
		{
			OutValue = Out;
			return true;
		}
		return false;
	}

	bool TryGetDoubleField(const WwiseUnrealString& Name, double& OutValue) const
	{
		return this->JsonObject.TryGetNumberField(Name.String, OutValue);
	}
	
	bool TryGetObjectField(const WwiseUnrealString& Name, WwiseUnrealJsonObject& OutValue) const
	{
		const TSharedPtr<FJsonObject>* Object;
		if(this->JsonObject.TryGetObjectField(Name.String, Object))
		{
			OutValue = WwiseUnrealJsonObject(*Object);
			return true;
		}
		return false;
	}
	
	bool TryGetStringField(const WwiseUnrealString& Name, WwiseUnrealString& OutValue) const
	{
		return this->JsonObject.TryGetStringField(Name.String, OutValue.String);
	}

	bool TryGetArrayField(const WwiseUnrealString& Name, WwiseUnrealArray<WwiseUnrealJsonObject>& OutValue) const
	{
		const TArray< TSharedPtr<FJsonValue> >* OutArray;
		if(this->JsonObject.TryGetArrayField(Name.String, OutArray))
		{
			for(const TSharedPtr<FJsonValue>& Element : *OutArray)
			{
				OutValue.Add(WwiseUnrealJsonObject(Element->AsObject()));
			}
			return true;
		}
		return false;
	}

	int GetKeys(WwiseUnrealArray<WwiseUnrealString>& OutKeys) const
	{
		for(auto& Pair : JsonObject.Values)
		{
			OutKeys.Add(WwiseUnrealString(Pair.Key));		
		}
		return OutKeys.Size();	
	}
};