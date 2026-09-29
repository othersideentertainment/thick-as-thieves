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

#include "Wwise/Metadata/WwiseMetadataLoadable.h"
#include "Wwise/Metadata/WwiseMetadataGameParameter.h"
#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"
#include "Wwise/Metadata/IWwiseMetadataLoader.h"
#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"

struct WwiseMetadataLoader : public IWwiseMetadataLoader
{
	bool bResult = true;
	const WwiseDBJsonObject& JsonObject;
	
	WwiseMetadataLoader(const WwiseDBJsonObject& InJsonObject) :
	    JsonObject(InJsonObject)
	{
	}

	void Fail(const WwiseDBString& FieldName) override;
	void LogParsed(const WwiseDBString& FieldName, const WwiseDBShortId Id = 0, const WwiseDBString& Name = WwiseDBString()) override;

	bool GetBool(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) override;
	float GetFloat(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) override;
	WwiseDBGuid GetGuid(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) override;
	int GetInt(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) override;
	WwiseDBString GetString(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) override;
	WwiseDBShortId GetWwiseShortId(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) override;

	template<typename T>
	T GetLoaderObject(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName);

	template<typename T>
	T* GetObjectPtr(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName);

	template<typename T>
	WwiseDBArray<T> GetArray(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName);

	template<typename T>
	void GetPropertyArray(T* Object, const WwiseDBMap<WwiseDBString, size_t>& FloatProperties);
};

template<typename T>
T WwiseMetadataLoader::GetLoaderObject(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName)
{
	if (!Object)
	{
		return {};
	}
	Object->AddRequestedValue(WwiseDBString("object"), FieldName);

	WwiseDBJsonObject InnerObject;
	if (!JsonObject.TryGetObjectField(FieldName, InnerObject))
	{
		Fail(FieldName);
		return T{};
	}
	
	WwiseMetadataLoader ObjectLoader(InnerObject);
	T Result(ObjectLoader);
	if (ObjectLoader.bResult)
	{
		Result.CheckRequestedValues(InnerObject);
	}
	else
	{
		bResult = false;
		LogParsed(FieldName);
	}

	return Result;
}


template <typename T>
T* WwiseMetadataLoader::GetObjectPtr(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName)
{
	if (!Object)
	{
		return {};
	}
	Object->AddRequestedValue(WwiseDBString("optional object"), FieldName);

	WwiseDBJsonObject InnerObject;
	if (!JsonObject.TryGetObjectField(FieldName, InnerObject))
	{
		return nullptr;
	}
	
	WwiseMetadataLoader ObjectLoader(InnerObject);
	T* Result = new T(ObjectLoader);
	if (ObjectLoader.bResult)
	{
		if (Result)
		{
			Result->CheckRequestedValues(InnerObject);
		}
	}
	else
	{
		bResult = false;
		LogParsed(FieldName);
		delete Result;
		return nullptr;
	}

	return Result;
}

template <typename T>
WwiseDBArray<T> WwiseMetadataLoader::GetArray(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName)
{
	if (!Object)
	{
		return {};
	}
	Object->AddRequestedValue(WwiseDBString("array"), FieldName);

	WwiseDBArray< WwiseDBJsonObject > Array;
	if (!JsonObject.TryGetArrayField(FieldName, Array))
	{
		// No data. Not a fail, valid!
		Object->IncLoadedSize(sizeof(WwiseDBArray<T>));
		return WwiseDBArray<T>{};
	}
	
	WwiseDBArray<T> Result;
	Result.Empty(Array.Size());

	for (auto& InnerObject : Array)
	{
		WwiseMetadataLoader ArrayLoader(InnerObject);
		T ResultObject(ArrayLoader);

		if (ArrayLoader.bResult)
		{
			ResultObject.CheckRequestedValues(InnerObject);
		}
		else
		{
			bResult = false;
			ArrayLoader.LogParsed(FieldName);
			Result.Empty();
			break;
		}

		Result.Add(std::move(ResultObject));
	}

	Object->IncLoadedSize(sizeof(WwiseDBArray<T>));
	return Result;
}

template <typename T>
void WwiseMetadataLoader::GetPropertyArray(T* Object, const WwiseDBMap<WwiseDBString, size_t>& FloatProperties)
{
	if (!Object)
	{
		return;
	}
	Object->AddRequestedValue("propertyarray"_wwise_db, "Properties"_wwise_db);

	Object->IncLoadedSize(FloatProperties.Size() * sizeof(float));

	WwiseDBArray<WwiseDBJsonObject> Array;
	if (!JsonObject.TryGetArrayField("Properties"_wwise_db, Array))
	{
		// No data. Not a fail, valid!
		return;
	}

	for (auto& InnerObject : Array)
	{
		WwiseDBString Name;
		if (!InnerObject.TryGetStringField("Name"_wwise_db, Name))
		{
			Fail("Property::Name"_wwise_db);
			continue;
		}
		WwiseDBString Type;
		if (!InnerObject.TryGetStringField("Type"_wwise_db, Type) || Type != "Real32"_wwise_db)
		{
			Fail("Property::Type"_wwise_db);
			continue;
		}
		double Value;
		if (!InnerObject.TryGetDoubleField("Value"_wwise_db, Value))
		{
			Fail("Property::Value"_wwise_db);
			continue;
		}
		if (const auto* Property = FloatProperties.Find(WwiseDBString(WwiseDBString(Name))))
		{
			*(float*)((intptr_t)Object + *Property) = (float)Value;
		}
		else
		{
			Fail(Name);
			continue;
		}
	}
}
