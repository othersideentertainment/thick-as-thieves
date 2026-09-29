// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ServerManager/OSEDedicatedServerManagerGameliftTypes.h"

// ose dedicated server
#include "ServerManager/OSEDedicatedServerManagerBase.h" // for logging

// ue4
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Serialization/JsonReader.h"

namespace MatchmakingParseUtl
{
   bool ParseStr(const TSharedPtr<FJsonObject>& jsonObj, const FString& fieldName, FString& outStr)
   {
      if (jsonObj->TryGetStringField(fieldName, outStr))
      {
         outStr = FGenericPlatformHttp::UrlDecode(outStr);
         return true;
      }
      return false;
   }

   bool ParseInt(const TSharedPtr<FJsonObject>& jsonObj, const FString& fieldName, int& outInt)
   {
      return jsonObj->TryGetNumberField(fieldName, outInt);
   }

   bool ParseAttributeStr(const TSharedPtr<FJsonObject>& jsonObj, const FString& fieldName, FString& outStr)
   {
      /*
         "field_name": {
         "attributeType": "STRING",
         "valueAttribute": "MyStringValue"
         }
      */

      const TSharedPtr<FJsonObject>* fieldObj;
      if (jsonObj->TryGetObjectField(fieldName, fieldObj))
      {
         return ParseStr(*fieldObj, TEXT("valueAttribute"), outStr);
      }
      return false;
   }

   bool ParseAttributeInt(const TSharedPtr<FJsonObject>& jsonObj, const FString& fieldName, int& outInt)
   {
      /*
         "field_name": {
            "attributeType": "DOUBLE",
            "valueAttribute": 0.0
         },
      */

      const TSharedPtr<FJsonObject>* fieldObj;
      if (jsonObj->TryGetObjectField(fieldName, fieldObj))
      {
         return ParseInt(*fieldObj, TEXT("valueAttribute"), outInt);
      }
      return false;
   }
}

void FGameliftMatchmakerData::FillFrom(const FString& matchmakerJSONPayloadStr)
{
   TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(matchmakerJSONPayloadStr);
   TSharedPtr<FJsonObject> matchmakingDataJsonObj;
   if (!FJsonSerializer::Deserialize(reader, matchmakingDataJsonObj))
   {
      UE_LOG(LogOSEDedicatedServer, Error, TEXT("Failed to deserialize matchmaking data into a JSON object: '%s'"), *matchmakerJSONPayloadStr);
      return;
   }

   FGameliftMatchmakerData matchmakerData;

   check(matchmakingDataJsonObj.IsValid());

   // match id
   MatchmakingParseUtl::ParseStr(matchmakingDataJsonObj, TEXT("matchId"), matchmakerData.MatchId);

   // matchmaking config arn
   if (MatchmakingParseUtl::ParseStr(matchmakingDataJsonObj, TEXT("matchmakingConfigurationArn"), matchmakerData.MatchmakingConfigurationArn))
   {
      // AWS docs say this is how we get the matchmaker config
      {
         const FString searchStr = TEXT("matchmakingconfiguration/");
         int strIdx = matchmakerData.MatchmakingConfigurationArn.Find(searchStr);
         if (strIdx != INDEX_NONE)
         {
            matchmakerData.MatchmakingConfiguration = matchmakerData.MatchmakingConfigurationArn.Mid(strIdx + searchStr.Len());
         }
      }

      // this seems like the only place we can pull our region out...?
      {
         const FString startSearchStr = TEXT("arn:aws:gamelift:");
         int arnStrIdx = matchmakerData.MatchmakingConfigurationArn.Find(startSearchStr);
         if (arnStrIdx != INDEX_NONE)
         {
            const FString searchValue = TEXT(":");
            const int subStrIdxStart = arnStrIdx + startSearchStr.Len();
            int subStrIdxEnd = matchmakerData.MatchmakingConfigurationArn.Find(searchValue, ESearchCase::IgnoreCase, ESearchDir::FromStart, subStrIdxStart);
            if (subStrIdxEnd != INDEX_NONE)
            {
               check(subStrIdxEnd > arnStrIdx);
               const int subStrLen = subStrIdxEnd - subStrIdxStart;
               matchmakerData.MatchmakingRegion = matchmakerData.MatchmakingConfigurationArn.Mid(subStrIdxStart, subStrLen);
            }
         }
      }
   }

   // TODO: When we're ready, we need to build APIS into the server mgr class to pass this json blob
   // back over to parse out game-specific details from the matchmaking ticket
}

void FGameliftMatchmakerData::DumpLog()
{
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("\n"));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("[Matchmaker Data]"));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("MatchId: %s"), *MatchId);
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("MatchmakingConfigurationArn: %s"), *MatchmakingConfigurationArn);
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("MatchmakingConfiguration: %s"), *MatchmakingConfiguration);
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("MatchmakingRegion: %s"), *MatchmakingRegion);
}

void FOSEDedicatedServerGameliftState::FillFrom(const Aws::GameLift::Server::Model::GameSession& gameSession)
{
   GameSession = gameSession;

   // pull any game property state out from gamelift
   int gamePropertiesCount = 0;
   const Aws::GameLift::Server::Model::GameProperty* gameProperties = gameSession.GetGameProperties(gamePropertiesCount);
   for (int gamePropertyIdx = 0; gamePropertyIdx < gamePropertiesCount; ++gamePropertyIdx)
   {
      const Aws::GameLift::Server::Model::GameProperty& gp = gameProperties[gamePropertyIdx];
      const FString key = UTF8_TO_TCHAR(gp.GetKey());
      const FString value = UTF8_TO_TCHAR(gp.GetValue());

      // TODO: We can use these key/value pairs to configure state on the gamelift webpage to inform dedicated server or game behavior here
      // For instance, we could feed in:
      // - Specific game modes or rules
      // - Information about how this instance can reach our future backend(s) via S2S rest apis
      // - Information about how to send BI data about this session
   }

   // arn
   GameSessionArn = UTF8_TO_TCHAR(gameSession.GetGameSessionId());

   // parse out any matchmaking state
   MatchmakerData.FillFrom(UTF8_TO_TCHAR(gameSession.GetMatchmakerData()));
}

void FOSEDedicatedServerGameliftState::DumpLog()
{
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("[Game Session]"));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_gameSessionId : \"%s\""), UTF8_TO_TCHAR(GameSession.GetGameSessionId()));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_name : \"%s\""), UTF8_TO_TCHAR(GameSession.GetName()));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_fleetId : \"%s\""), UTF8_TO_TCHAR(GameSession.GetFleetId()));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_maximumPlayerSessionCount : \"%d\""), GameSession.GetMaximumPlayerSessionCount());
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_status : \"%s\""), UTF8_TO_TCHAR(Aws::GameLift::Server::Model::GameSessionStatusMapper::GetNameForGameSessionStatus(GameSession.GetStatus())));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_gameProperties :"));
   int gamePropertiesCount = 0;
   const Aws::GameLift::Server::Model::GameProperty* gameProperties = GameSession.GetGameProperties(gamePropertiesCount);
   for (int gamePropertyIdx = 0; gamePropertyIdx < gamePropertiesCount; ++gamePropertyIdx)
   {
      const Aws::GameLift::Server::Model::GameProperty& gp = gameProperties[gamePropertyIdx];
      const FString key = UTF8_TO_TCHAR(gp.GetKey());
      const FString value = UTF8_TO_TCHAR(gp.GetValue());
      UE_LOG(LogOSEDedicatedServer, Log, TEXT("idx: %d"), gamePropertyIdx);
      UE_LOG(LogOSEDedicatedServer, Log, TEXT("   -key: %s"), *key);
      UE_LOG(LogOSEDedicatedServer, Log, TEXT("   -value: %s"), *value);
   }
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_ipAddress : \"%s\""), UTF8_TO_TCHAR(GameSession.GetIpAddress()));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_port : \"%d\""), GameSession.GetPort());
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_dnsName : \"%s\""), UTF8_TO_TCHAR(GameSession.GetDnsName()));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_gameSessionData:"));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("%s"), UTF8_TO_TCHAR(GameSession.GetGameSessionData()));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("m_matchmakerData:"));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("%s"), UTF8_TO_TCHAR(GameSession.GetMatchmakerData()));

   UE_LOG(LogOSEDedicatedServer, Log, TEXT("\n"));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("[Parsed Game Session Data]"));
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("GameSessionArn: %s"), *GameSessionArn);
   UE_LOG(LogOSEDedicatedServer, Log, TEXT("BackfillTicketId: %s"), *BackfillTicketId);

   // and finally dump the matchmaking data
   MatchmakerData.DumpLog();
}
