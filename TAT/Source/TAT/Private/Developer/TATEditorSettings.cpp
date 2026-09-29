// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Developer/TATEditorSettings.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Settings/TATMatchSettingsBase.h"

// ue
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEditorSettings)

DEFINE_LOG_CATEGORY_STATIC(LogTATEditorSettings, Log, All);

void FTATEditorSettingsCheatCommand::RunCheat(UWorld* world, APlayerController* pc) const
{
   check(world != nullptr);
   if (!Enabled || FStringView(Cheat).TrimStartAndEnd().Len() == 0)
   {
      return;
   }

   auto printCheatMessage = [this]()
   {
      const FString displayMessage = FString::Printf(TEXT("AutoExec Cheat: %s"), *Cheat);
      if (UTATEditorSettings::Get().AutoExecCheatScreenMessageDuration > 0)
      {
         GEngine->AddOnScreenDebugMessage(0, UTATEditorSettings::Get().AutoExecCheatScreenMessageDuration, FColor::Emerald, displayMessage);
      }
      UE_LOG(LogTATEditorSettings, Log, TEXT("%s"), *displayMessage);
   };

   if (world == nullptr && pc != nullptr)
   {
      world = pc->GetWorld();
   }

   // We need to avoid running the same cheat twice (eg. when starting PIE as a client, this will run on both the server and client)
   bool meetsAuthorityReq = false;
   if ((world != nullptr && world->GetNetMode() == NM_Standalone) || Context == ETATEditorSettingsCheatCommandContext::Engine)
   {
      meetsAuthorityReq = true;
   }
   else if (world != nullptr)
   {
      meetsAuthorityReq = RunOnClients ? (world->GetNetMode() == NM_Client) : (world->GetNetMode() < NM_Client);
   }

   if (!meetsAuthorityReq)
   {
      return;
   }

   switch (Context)
   {
   case ETATEditorSettingsCheatCommandContext::Player:
      if (pc != nullptr)
      {
         printCheatMessage();
         pc->ClientMessage(pc->ConsoleCommand(Cheat));
      }
      break;
   case ETATEditorSettingsCheatCommandContext::World:
      if (world != nullptr)
      {
         printCheatMessage();
         world->Exec(world, *Cheat);
      }
      break;
   case ETATEditorSettingsCheatCommandContext::Engine:
      printCheatMessage();
      GEngine->Exec(world, *Cheat);
      break;
   default:
      break;
   }
}

ETATCharacter UTATEditorSettings::GetOverrideCharacter(const UWorld* world) const
{
   ETATCharacter character = OverrideCharacter_Player0;

   // seems like pie instance zero is always going to be the listen server and/or dedicated server
   // so we can figure out if our character indicies start at 0 or 1 from that
   bool isListenServer = true;
   if (const FWorldContext* context = GEngine->GetWorldContextFromPIEInstance(0))
   {
      isListenServer = !context->RunAsDedicated;
   }

   if (const FWorldContext* context = GEngine->GetWorldContextFromWorld(world))
   {
      int characterIndex = isListenServer ? context->PIEInstance : (context->PIEInstance - 1);
      switch (characterIndex)
      {
      case 0:
         character = OverrideCharacter_Player0;
         break;
      case 1:
         character = OverrideCharacter_Player1;
         break;
      case 2:
         character = OverrideCharacter_Player2;
         break;
      case 3:
         character = OverrideCharacter_Player3;
         break;
      default:
         character = OverrideCharacter_Player0;
         break;
      }
   }

   return character;
}

bool UTATEditorSettings::GetOverrideGearLoadout(const UWorld* world, TArray<FGameplayTag>& outGearLoadout) const
{
   if (!OverrideGearLoadouts)
   {
      return false;
   }

   bool isListenServer = true;
   if (const FWorldContext* context = GEngine->GetWorldContextFromPIEInstance(0))
   {
      isListenServer = !context->RunAsDedicated;
   }

   if (const FWorldContext* context = GEngine->GetWorldContextFromWorld(world))
   {
      int32 characterIndex = isListenServer ? context->PIEInstance : (context->PIEInstance - 1);

      if (characterIndex == 0)
      {
         outGearLoadout = OverrideGearLoadout_Player0;
         return true;
      }
      else if (characterIndex == 1)
      {
         outGearLoadout = OverrideGearLoadout_Player1;
         return true;
      }
      else if (characterIndex == 2)
      {
         outGearLoadout = OverrideGearLoadout_Player2;
         return true;
      }
   }

   return false;
}

void UTATEditorSettings::ApplyDefaultMatchSettings(UTATMatchSettingsBase& matchSettings, const FTATMatchSettingsQueryContext& context) const
{
   FString validationError;
   for (const auto& pair : DefaultMatchSettings)
   {
      // If the key has a type annotation in it, strip it out and use everything to the left of a ':' char as the property name.
      FStringView keyView(pair.Key);
      int32 sepIndex = INDEX_NONE;
      if (keyView.FindChar(':', sepIndex))
      {
         keyView = keyView.SubStr(0, sepIndex);
      }

      bool invalidValue = false;
      if (pair.Value.Value.Len() == 0)
      {
         invalidValue = true;
         validationError = TEXT("Value was empty, did you forget to enter one?");
      }
      else if (!matchSettings.SetMatchSettingsValueAsString(FName(keyView), pair.Value.Value, context, &validationError))
      {
         invalidValue = true;
      }

      if (invalidValue)
      {
         // Show an on-screen error; otherwise nobody will notice the value was invalid
         const FString errorMessage = FString::Printf(TEXT("EditorSettings.DefaultMatchSettings: Property '%s' not applied: %s"), *FString(keyView), *validationError);
         constexpr float displayTime = 15.0f;
         GEngine->AddOnScreenDebugMessage(INDEX_NONE, displayTime, FColor::Orange, errorMessage);
         UE_LOG(LogTATEditorSettings, Error, TEXT("%s"), *errorMessage);
      }
   }
}

TArray<FString> UTATEditorSettings::_GetMatchSettingsProperties() const
{
   TArray<FString> result;
   TSubclassOf<UTATMatchSettingsBase> matchSettingsClass = UTATProjectSettings::Get().GetMatchSettingsClass();
   check(matchSettingsClass != nullptr);
   UTATMatchSettingsBase* matchSettingsCDO = matchSettingsClass->GetDefaultObject<UTATMatchSettingsBase>();
   check(matchSettingsCDO != nullptr);
   UEnum* propertyTypeEnum = StaticEnum<ETATMatchSettingsPropertyType>();
   check(propertyTypeEnum != nullptr);
   TArray<FTATMatchSettingsPropertyDef> props;
   matchSettingsCDO->GetAllMatchSettingsProperties(props);
   for (const FTATMatchSettingsPropertyDef& prop : props)
   {
      // Add a type annotation to the property name for readability (ApplyDefaultMatchSettings knows to strip that part out)
      result.Add(FString::Printf(TEXT("%s: %s"),
         *prop.Name.ToString(),
         *propertyTypeEnum->GetNameStringByValue(static_cast<int64>(prop.Type))));
   }
   return result;
}
