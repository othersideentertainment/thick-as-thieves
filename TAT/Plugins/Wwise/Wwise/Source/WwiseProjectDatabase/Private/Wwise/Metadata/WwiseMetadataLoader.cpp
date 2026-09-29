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

#include "Wwise/Metadata/WwiseMetadataLoader.h"

#include <inttypes.h>

void WwiseMetadataLoader::Fail(const WwiseDBString& FieldName)
{
	WWISE_DB_LOG(Error, "Could not retrieve field %s", *FieldName);
	bResult = false;
}

void WwiseMetadataLoader::LogParsed(const WwiseDBString& FieldName, const WwiseDBShortId Id, const WwiseDBString& Name)
{
	if (bResult)
	{
		if (Id && !Name.IsEmpty())
		{
			WWISE_DB_LOG(VeryVerbose, "Parsed %s [%" PRIu32 "] %s", *FieldName, Id, *Name);
		}
		else if (Id)
		{
			WWISE_DB_LOG(VeryVerbose, "Parsed %s [%" PRIu32 "]", *FieldName, Id);
		}
		else if (!Name.IsEmpty())
		{
			WWISE_DB_LOG(VeryVerbose, "Parsed %s: %s", *FieldName, *Name);
		}
		else
		{
			WWISE_DB_LOG(VeryVerbose, "Parsed %s", *FieldName);
		}
	}
	else 
	{
		if (Id && !Name.IsEmpty())
		{
			WWISE_DB_LOG(Log, "... while parsing %s [%" PRIu32 "] %s", *FieldName, Id, *Name);
		}
		else if (Id)
		{
			WWISE_DB_LOG(Log, "... while parsing %s [%" PRIu32 "]", *FieldName, Id);
		}
		else if (!Name.IsEmpty())
		{
			WWISE_DB_LOG(Log, "... while parsing %s: %s", *FieldName, *Name);
		}
		else
		{
			WWISE_DB_LOG(Log, "... while parsing %s", *FieldName);
		}
	}
}

bool WwiseMetadataLoader::GetBool(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required)
{
	if(Object == nullptr)
	{
		Fail(FieldName);
		return false;
	}
	Object->AddRequestedValue("bool"_wwise_db, FieldName);

	bool Value = false;

	if (!JsonObject.TryGetBoolField(FieldName, Value) && Required == WwiseRequiredMetadata::Mandatory)
	{
		Fail(FieldName);
	}

	Object->IncLoadedSize(sizeof(Value));
	return Value;
}

float WwiseMetadataLoader::GetFloat(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required)
{
	if(Object == nullptr)
	{
		Fail(FieldName);
		return 0.f;
	}
	Object->AddRequestedValue("float"_wwise_db, FieldName);

	double Value{};

	if (!JsonObject.TryGetDoubleField(FieldName, Value) && Required == WwiseRequiredMetadata::Mandatory)
	{
		Fail(FieldName);
	}

	Object->IncLoadedSize(sizeof(Value));
	return float(Value);
}

WwiseDBGuid WwiseMetadataLoader::GetGuid(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required)
{
	if(Object == nullptr)
	{
		Fail(FieldName);
		return WwiseDBGuid();
	}
	Object->AddRequestedValue("guid"_wwise_db, FieldName);

	WwiseDBGuid Value{};

	WwiseDBString ValueAsString;
	if (!JsonObject.TryGetStringField(FieldName, ValueAsString))
	{
		if (Required == WwiseRequiredMetadata::Mandatory)
		{
			Fail(FieldName);
		}
	}
	else if (ValueAsString.Size() != 38)
	{
		WWISE_DB_LOG(Error, "Invalid GUID %s: %s", *FieldName, *ValueAsString);
		Fail(FieldName);
	}
	else
	{
		if (!WwiseDBGuid::Parse(ValueAsString, Value))
		{
			WWISE_DB_LOG(Error, "Could not decode GUID %s: %s", *FieldName, *ValueAsString);
			Fail(FieldName);
		}
	}

	Object->IncLoadedSize(sizeof(Value));
	return Value;
}

int WwiseMetadataLoader::GetInt(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required)
{
	if(Object == nullptr)
	{
		Fail(FieldName);
		return 0;
	}
	Object->AddRequestedValue("int"_wwise_db, FieldName);

	int Value{};

	if (!JsonObject.TryGetIntField(FieldName, Value) && Required == WwiseRequiredMetadata::Mandatory)
	{
		Fail(FieldName);
	}

	Object->IncLoadedSize(sizeof(Value));
	return Value;
}

WwiseDBString WwiseMetadataLoader::GetString(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required)
{
	if(Object == nullptr)
	{
		Fail(FieldName);
		return WwiseDBString();
	}
	Object->AddRequestedValue("string"_wwise_db, FieldName);

	WwiseDBString Value{};

	if (!JsonObject.TryGetStringField(FieldName, Value) && Required == WwiseRequiredMetadata::Mandatory)
	{
		Fail(FieldName);
	}

	Object->IncLoadedSize(sizeof(Value) + Value.AllocatedSize());
	return WwiseDBString(Value);
}

WwiseDBShortId WwiseMetadataLoader::GetWwiseShortId(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required)
{
	if(Object == nullptr)
	{
		Fail(FieldName);
		return WwiseDBShortId();
	}
	Object->AddRequestedValue("WwiseShortId"_wwise_db, FieldName);

	WwiseDBShortId Value{};

	if (!JsonObject.TryGetShortIdField(FieldName, Value) && Required == WwiseRequiredMetadata::Mandatory)
	{
		Fail(FieldName);
	}

	Object->IncLoadedSize(sizeof(Value));
	return Value;
}



