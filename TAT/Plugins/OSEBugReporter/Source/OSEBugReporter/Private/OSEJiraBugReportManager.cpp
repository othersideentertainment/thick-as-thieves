// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSEJiraBugReportManager.h"

#include "OSEBugReporterSettings.h"
#include "OSEBugReporterLog.h"
#include "OSEBugData.h"


#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

namespace OSEBugJiraData
{
   static const TConsoleVariableData<FString>* CVarJiraUser = IConsoleManager::Get().RegisterConsoleVariable(TEXT("OSE.BugReport.User"), TEXT(""), TEXT("UserName to login with."))->AsVariableString();
   static const TConsoleVariableData<FString>* CVarJiraPwd = IConsoleManager::Get().RegisterConsoleVariable(TEXT("OSE.BugReport.Pwd"), TEXT(""), TEXT("Password to login with."))->AsVariableString();
}

void UOSEJiraBugReportManager::ReportBug(const FOSEBugData& Data)
{
   const FString user = GetUser();
   const FString pwd = GetPwd();
   const FString url = GetRestURI();
   const FString projectKey = GetJiraProject();

   if(user.IsEmpty())
   {
      UE_LOG(LogOSEBugReporter, Warning, TEXT("Bug Report Failed: No user name set"));
      return;
   }

   if(pwd.IsEmpty())
   {
      UE_LOG(LogOSEBugReporter, Warning, TEXT("Bug Report Failed: No password set"));
      return;
   }

   if(url.IsEmpty())
   {
      UE_LOG(LogOSEBugReporter, Warning, TEXT("Bug Report Failed: No rest uri set"));
      return;
   }

   if(projectKey.IsEmpty())
   {
      UE_LOG(LogOSEBugReporter, Warning, TEXT("Bug Report Failed: No project key set"));
      return;
   }

   TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

   //configure request headers
   {
      FString credentials = FString::Printf(TEXT("%s:%s"), *user, *pwd);
	   FString encodedCredentials = FBase64::Encode(credentials);
	   FString authorization = FString::Printf(TEXT("Basic %s"), *encodedCredentials);

      //UE_LOG(LogOSEBugReporter, Warning, TEXT("JIRA Submission: credentials [%s:%s]"), *user, *pwd);
      //UE_LOG(LogOSEBugReporter, Warning, TEXT("JIRA Submission: authorization [%s]"), *authorization);

      Request->SetVerb(TEXT("POST"));
      Request->SetURL(url);
      Request->SetHeader("Content-Type", TEXT("application/json"));
      Request->SetHeader("Authorization", authorization);
   }

   //Create the request body
   {
      TSharedRef<FJsonObject> jsonIssue = CreateJsonObject(Data, projectKey);

      FString jsonString;
	   TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&jsonString);
	   FJsonSerializer::Serialize(jsonIssue, Writer);

	   Request->SetContentAsString(jsonString);
   }

   //setup response handler
   {
      Request->OnProcessRequestComplete().BindUObject(this, &UOSEJiraBugReportManager::ProcessResponse);
   }

   //UE_LOG(LogOSEBugReporter, Warning, TEXT("JIRA Submission: URL [%s]"), *url);
   Request->ProcessRequest();
}
   
void UOSEJiraBugReportManager::ProcessResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool connectedSuccessfully)
{
   if (connectedSuccessfully)
   {
      TSharedPtr<FJsonObject> jsonObject;
      TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
      if (FJsonSerializer::Deserialize(jsonReader, jsonObject)) 
      {
         bool bSuccess = true;
         const TSharedPtr<FJsonObject>* errorsObject = nullptr;
         if(jsonObject->TryGetObjectField(TEXT("errors"), errorsObject))
         {
            FString errorMessage;
            for (const TPair<FString, TSharedPtr<FJsonValue>>& pair : errorsObject->Get()->Values) 
            {
               errorMessage += "\n\t"+ pair.Key + " : " + pair.Value->AsString();
            }
            if(!errorMessage.IsEmpty())
            {
               bSuccess = false;
               UE_LOG(LogOSEBugReporter, Warning, TEXT("Jira Issue Submission Failed: %s"), *errorMessage);
            }
         }

         const TArray<TSharedPtr<FJsonValue>>* errorMessages = nullptr;
         if(jsonObject->TryGetArrayField(TEXT("errorMessages"), errorMessages))
         {
            FString errorMessage;
            for (const TSharedPtr<FJsonValue>& value : *errorMessages)
            {
               bSuccess = false;
               UE_LOG(LogOSEBugReporter, Warning, TEXT("Jira Issue Submission Failed: %s"), *value->AsString());
            }
         }

         if(bSuccess)
         {
            FString key = jsonObject->GetStringField(TEXT("key"));
            FString issueUrl = FString::Printf(TEXT("%s%s"), *GetBrowseURI(), *key);

            UE_LOG(LogOSEBugReporter, Warning, TEXT("Jira Issue Submission Succeeded. New Issue: %s"), *issueUrl);
         }
      }
      else
      {
         UE_LOG(LogOSEBugReporter, Warning, TEXT("Jira Issue Submission Failed: Couldn't process response\n%s"), *Response->GetContentAsString());
      }
   }
   else
   {
      int32 responseCode = Response.Get()->GetResponseCode();
      FString errorMessage = FString::Printf(TEXT("Response code %d"), responseCode);
      UE_LOG(LogOSEBugReporter, Warning, TEXT("Jira Issue Submission Failed: %s"), *errorMessage );
   }
}

TSharedRef<FJsonObject> UOSEJiraBugReportManager::CreateJsonObject(const FOSEBugData& Data, const FString& projectKey) const
{
   //JSON issue format for JIRA REST v2
   //{
   //    "fields": {
   //       "project":
   //       {
   //          "key": "TEST"
   //       },
   //       "summary": "REST ye merry gentlemen.",
   //       "description": "Creating of an issue using project keys and issue type names using the REST API",
   //       "issuetype": {
   //          "name": "Bug"
   //       }
   //   }
   //}
   TSharedRef<FJsonObject> jsonIssue = MakeShared<FJsonObject>();
   {
      TSharedRef<FJsonObject> fieldsObject = MakeShared<FJsonObject>();
      jsonIssue->SetObjectField("fields", fieldsObject);
      {
         TSharedRef<FJsonObject> projectObject = MakeShared<FJsonObject>();
         fieldsObject->SetObjectField("project", projectObject);
         {
            projectObject->SetStringField("key", projectKey);
         }
         TSharedRef<FJsonObject> issueObject = MakeShared<FJsonObject>();
         fieldsObject->SetObjectField("issuetype", issueObject);
         {
            issueObject->SetStringField("name", "Bugsplat");
         }

         //create optional fields
         bool bCustomCL = false;
         {
            const UOSEBugReporterSettings*settings = GetDefault<UOSEBugReporterSettings>();
            if (!settings->ChangelistFieldId.IsEmpty())
            {
               bCustomCL = true;
               fieldsObject->SetStringField(settings->ChangelistFieldId, Data.BuildVersion);
            }
         }
         fieldsObject->SetStringField("summary", Data.Summary);
         //TODO: This could be made much more expressive if we switch to the rest 3 api
         {
            FString inlineDescription;
            inlineDescription = Data.Description;
            inlineDescription += "\nMap: " + Data.MapName;
            inlineDescription += FString::Printf(TEXT("\nTime in map: %g"), Data.WorldTime);
            inlineDescription += "\nPlayer Location: " + Data.WorldPosition.ToString();

            inlineDescription += "\n\nObject Directly In Front Of Camera:\n";
            inlineDescription += Data.DirectObjectData;

            inlineDescription += "\n\nObjects In Radius Of Object Directly In Front Of Camera:\n";
            inlineDescription += Data.RadiusObjectData;

            fieldsObject->SetStringField("description", inlineDescription);
         }

         //TODO: This could be made much more expressive
         {
            FString inlineEnvironment;
            if(!bCustomCL)
            {
               inlineEnvironment = "Build Version: " + Data.BuildVersion;
            }
            inlineEnvironment += "\nBuild Configuration: " + Data.BuildConfiguration;
            inlineEnvironment += "\nPlatform: " + Data.Platform;
            inlineEnvironment += "\nGPU: " + Data.GPU;
            fieldsObject->SetStringField("environment", inlineEnvironment);
         }
      }
   }
   

    FString OutputString;
    TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(jsonIssue, Writer);

    UE_LOG(LogOSEBugReporter, Verbose, TEXT("Issue JSON: %s"), *OutputString);

    return jsonIssue;
}

FString UOSEJiraBugReportManager::GetJiraProject() const
{
   const UOSEBugReporterSettings*settings = GetDefault<UOSEBugReporterSettings>();
   return settings->DefectProjectKey;
}

FString UOSEJiraBugReportManager::GetRestURI() const
{
   const UOSEBugReporterSettings*settings = GetDefault<UOSEBugReporterSettings>();
   const FString URL = FString::Printf(TEXT("%s/rest/api/2/issue/"), *settings->URL);
   return URL;
}

FString UOSEJiraBugReportManager::GetBrowseURI() const
{
   const UOSEBugReporterSettings*settings = GetDefault<UOSEBugReporterSettings>();
   const FString URL = FString::Printf(TEXT("%s/browse/"), *settings->URL);
   return URL;

}

FString UOSEJiraBugReportManager::GetUser() const
{
   return OSEBugJiraData::CVarJiraUser->GetValueOnGameThread();
}

FString UOSEJiraBugReportManager::GetPwd() const
{
   return OSEBugJiraData::CVarJiraPwd->GetValueOnGameThread();
}
