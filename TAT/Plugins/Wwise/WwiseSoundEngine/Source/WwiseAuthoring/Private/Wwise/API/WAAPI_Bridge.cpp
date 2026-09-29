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

#include "Wwise/API/WAAPI_Bridge.h"
#include "Wwise/API/WwiseMonitorAPI.h"
#include "Wwise/WwiseSoundEngineModule.h"

#if AK_SUPPORT_WAAPI

bool FWAAPI_Bridge::Client_Bridge::Connect(const char* in_uri, unsigned in_port,
	AK::WwiseAuthoringAPI::disconnectHandler_t disconnectHandler, int in_timeoutMs)
{
	return Client.Connect(in_uri, in_port, disconnectHandler, in_timeoutMs);
}

bool FWAAPI_Bridge::Client_Bridge::IsConnected() const
{
	return Client.IsConnected();
}

void FWAAPI_Bridge::Client_Bridge::Disconnect()
{
	Client.Disconnect();
}

bool FWAAPI_Bridge::Client_Bridge::Subscribe(const char* in_uri, const char* in_options,
	AK::WwiseAuthoringAPI::Client::WampEventCallback in_callback, uint64_t& out_subscriptionId, std::string& out_result,
	int in_timeoutMs)
{
	return Client.Subscribe(in_uri, in_options, in_callback, out_subscriptionId, out_result, in_timeoutMs);
}

bool FWAAPI_Bridge::Client_Bridge::Subscribe(const char* in_uri, const AK::WwiseAuthoringAPI::AkJson& in_options,
	AK::WwiseAuthoringAPI::Client::WampEventCallback in_callback, uint64_t& out_subscriptionId,
	AK::WwiseAuthoringAPI::AkJson& out_result, int in_timeoutMs)
{
	return Client.Subscribe(in_uri, in_options, in_callback, out_subscriptionId, out_result, in_timeoutMs);
}

bool FWAAPI_Bridge::Client_Bridge::Unsubscribe(const uint64_t& in_subscriptionId, std::string& out_result,
	int in_timeoutMs)
{
	return Client.Unsubscribe(in_subscriptionId, out_result, in_timeoutMs);
}

bool FWAAPI_Bridge::Client_Bridge::Unsubscribe(const uint64_t& in_subscriptionId,
	AK::WwiseAuthoringAPI::AkJson& out_result, int in_timeoutMs)
{
	return Client.Unsubscribe(in_subscriptionId, out_result, in_timeoutMs);
}

bool FWAAPI_Bridge::Client_Bridge::Call(const char* in_uri, const char* in_args, const char* in_options,
	std::string& out_result, int in_timeoutMs)
{
	return Client.Call(in_uri, in_args, in_options, out_result, in_timeoutMs);
}

bool FWAAPI_Bridge::Client_Bridge::Call(const char* in_uri, const AK::WwiseAuthoringAPI::AkJson& in_args,
	const AK::WwiseAuthoringAPI::AkJson& in_options, AK::WwiseAuthoringAPI::AkJson& out_result, int in_timeoutMs)
{
	return Client.Call(in_uri, in_args, in_options, out_result, in_timeoutMs);
}

IWAAPI::Client* FWAAPI_Bridge::NewClient()
{
	return new Client_Bridge;
}

std::string FWAAPI_Bridge::GetJsonString(const AK::WwiseAuthoringAPI::JsonProvider& InJsonProvider)
{
	return InJsonProvider.GetJsonString();
}

#if !defined(AK_OPTIMIZED)
void FWAAPI_Bridge::SetupErrorTranslator(const FString& WaapiIP, AkUInt32 WaapiPort, AkUInt32 Timeout)
{
	WaapiErrorMessageTranslator.SetConnectionIP(TCHAR_TO_ANSI(*WaapiIP), WaapiPort, Timeout);
	IWwiseSoundEngineModule::ForceLoadModule();
	if (IWwiseSoundEngineModule::Monitor)
	{
		IWwiseSoundEngineModule::Monitor->AddTranslator(&WaapiErrorMessageTranslator);
	}
}

void FWAAPI_Bridge::TerminateErrorTranslator()
{
	WaapiErrorMessageTranslator.Term();
}

AkWAAPIErrorMessageTranslator FWAAPI_Bridge::WaapiErrorMessageTranslator;

#endif

#endif
