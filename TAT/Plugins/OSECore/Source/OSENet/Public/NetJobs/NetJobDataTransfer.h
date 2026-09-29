// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "NetJob.h"

// engine
#include "CoreMinimal.h"

///////////////////////////////////////////////////////////////////
// NetJobUploadData
///////////////////////////////////////////////////////////////////

class OSENET_API NetJobUploadData : public NetJob
{
public:
   NetJobUploadData(const TArray<uint8>& bytes, const FString& url);
   NetJobUploadData(const TArray<uint8>&& bytes, const FString& url);
   virtual ENetJobRequestType GetRequestType() const override final { return ENetJobRequestType::Put; }
   virtual const FString& GetBaseURL() const override final { return _url; }
   virtual FString GetEndpoint() const override final { return FString(); }
   virtual void OnGenerateRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req) override final;
   virtual bool OnHandleResponse(FHttpResponsePtr response) override final;

   // access the data we're going to upload
   virtual const TArray<uint8>& GetBytes() const { return _bytes; }

private:
   TArray<uint8> _bytes;
   FString _url;
};

///////////////////////////////////////////////////////////////////
// NetJobDownloadData
///////////////////////////////////////////////////////////////////

class OSENET_API NetJobDownloadData : public NetJob
{
public:
   NetJobDownloadData(const FString& url);
   virtual ENetJobRequestType GetRequestType() const override final { return ENetJobRequestType::Get; }
   virtual const FString& GetBaseURL() const override final { return _url; }
   virtual FString GetEndpoint() const override final { return FString(); }
   virtual bool OnHandleResponse(FHttpResponsePtr response) override final;

   // access the data we're going to download
   virtual const TArray<uint8>& GetBytes() const { return _bytes; }

protected:
   // used by ourselves and subclasses to store off the downloaded data from the response
   virtual void SetBytes(const TArray<uint8>& bytes) { _bytes = bytes; }

private:
   TArray<uint8> _bytes;
   FString _url;
};
