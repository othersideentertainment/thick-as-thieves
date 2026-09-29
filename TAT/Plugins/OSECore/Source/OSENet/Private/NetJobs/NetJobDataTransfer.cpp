// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "NetJobs/NetJobDataTransfer.h"

// ue4
#include "Interfaces/IHttpResponse.h"

///////////////////////////////////////////////////////////////////
// NetJobUploadData
///////////////////////////////////////////////////////////////////

NetJobUploadData::NetJobUploadData(const TArray<uint8>& bytes, const FString& url)
   : NetJob()
   , _bytes(bytes)
   , _url(url)
{

}

NetJobUploadData::NetJobUploadData(const TArray<uint8>&& bytes, const FString& url)
   : NetJob()
   , _bytes(bytes)
   , _url(url)
{

}

void NetJobUploadData::OnGenerateRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req)
{
   // add in our data to upload
   req->SetContent(GetBytes());
}

bool NetJobUploadData::OnHandleResponse(FHttpResponsePtr response)
{
   UE_LOG(LogNetJob, Verbose, TEXT("Bytes uploaded: '%d'..."), GetBytes().Num());
   return true;
}

///////////////////////////////////////////////////////////////////
// NetJobDownloadData
///////////////////////////////////////////////////////////////////

NetJobDownloadData::NetJobDownloadData(const FString& url)
   : NetJob()
   , _url(url)
{

}

bool NetJobDownloadData::OnHandleResponse(FHttpResponsePtr response)
{
   SetBytes(response->GetContent());
   UE_LOG(LogNetJob, Verbose, TEXT("Bytes downloaded: '%d'..."), GetBytes().Num());
   return true;
}
