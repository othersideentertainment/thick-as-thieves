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

#include "Wwise/API/WAAPI.h"

#if AK_SUPPORT_WAAPI
class WWISEAUTHORING_API FWAAPI_Bridge : public IWAAPI
{
public:
	UE_NONCOPYABLE(FWAAPI_Bridge);
	FWAAPI_Bridge() = default;
	~FWAAPI_Bridge() override {}

	class WWISEAUTHORING_API Client_Bridge : public Client
	{
	public:
		UE_NONCOPYABLE(Client_Bridge);
		Client_Bridge() = default;
		~Client_Bridge() override {}

		virtual bool Connect(const char* in_uri, unsigned int in_port, AK::WwiseAuthoringAPI::disconnectHandler_t disconnectHandler = nullptr, int in_timeoutMs = -1) override;
		virtual bool IsConnected() const override;
		virtual void Disconnect() override;

		virtual bool Subscribe(const char* in_uri, const char* in_options, AK::WwiseAuthoringAPI::Client::WampEventCallback in_callback, uint64_t& out_subscriptionId, std::string& out_result, int in_timeoutMs = -1) override;
		virtual bool Subscribe(const char* in_uri, const AK::WwiseAuthoringAPI::AkJson& in_options, AK::WwiseAuthoringAPI::Client::WampEventCallback in_callback, uint64_t& out_subscriptionId, AK::WwiseAuthoringAPI::AkJson& out_result, int in_timeoutMs = -1) override;
		
		virtual bool Unsubscribe(const uint64_t& in_subscriptionId, std::string& out_result, int in_timeoutMs = -1) override;
		virtual bool Unsubscribe(const uint64_t& in_subscriptionId, AK::WwiseAuthoringAPI::AkJson& out_result, int in_timeoutMs = -1) override;

		virtual bool Call(const char* in_uri, const char* in_args, const char* in_options, std::string& out_result, int in_timeoutMs = -1) override;
		virtual bool Call(const char* in_uri, const AK::WwiseAuthoringAPI::AkJson& in_args, const AK::WwiseAuthoringAPI::AkJson& in_options, AK::WwiseAuthoringAPI::AkJson& out_result, int in_timeoutMs = -1) override;

	private:
		AK::WwiseAuthoringAPI::Client Client;
	};
	virtual Client* NewClient() override;

	virtual std::string GetJsonString(const AK::WwiseAuthoringAPI::JsonProvider&) override;

#if !defined(AK_OPTIMIZED)
	/// Add the default, WwiseSDK-provided WAAPI error translator.
	virtual void SetupErrorTranslator(
		const FString& WaapiIP, ///< IP Address of the WAAPI server
		AkUInt32 WaapiPort, ///< Port of the WAAPI server
		AkUInt32 Timeout ///< Maximum time that can be spent resolving the error parameters. Set to INT_MAX to wait infinitely or 0 to disable XML translation entirely.
		) override;

	/// Terminate the default, WwiseSDK-provided WAAPI error translator.
	virtual void TerminateErrorTranslator() override;
private:
	static AkWAAPIErrorMessageTranslator WaapiErrorMessageTranslator;
#endif
};
#endif
