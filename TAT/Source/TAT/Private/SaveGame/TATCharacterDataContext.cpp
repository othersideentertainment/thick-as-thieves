// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "SaveGame/TATCharacterDataContext.h"

// tat
#include "Player/TATPlayerState.h"
#include "SaveGame/TATSaveGame.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterDataContext)

const FTATCharacterProgression& FTATCharacterDataContext::GetCharacterDataChecked() const
{
   check(IsValid());
   return SaveGame->GetCharacterProgression(SaveId);
}

const FTATPlayerProgression& FTATCharacterDataContext::GetPlayerDataChecked() const
{
   check(IsValid());
   return SaveGame->GetPlayerProgression();
}

FString FTATCharacterDataContext::ToString() const
{
   if (!IsValid())
   {
      return TEXT("TATCharacterDataContext(Invalid)");
   }
   return FString::Printf(TEXT("TATCharacterDataContext(SaveGame=[%s], SaveId=%s)"), *SaveGame->GetName(), *SaveId.ToString());
}

// static
FTATCharacterDataContext UTATCharacterDataContextUtils::GetCharacterDataContextFromPlayerState(APlayerState* playerState)
{
   ATATPlayerState* tatPlayerState = Cast<ATATPlayerState>(playerState);
   if (tatPlayerState == nullptr || !tatPlayerState->IsLocalPlayerState())
   {
      return FTATCharacterDataContext{};
   }
   UTATSaveGame* saveGame = nullptr;
   if (UOSESaveGameSystem* saveGameSystem = UOSESaveGameSystem::Get(playerState))
   {
      saveGame = Cast<UTATSaveGame>(saveGameSystem->GetSaveData());
      if (saveGame == nullptr)
      {
         return FTATCharacterDataContext{};
      }
   }
   const FTATCharacterSaveId saveId = tatPlayerState->GetCharacterSaveId();
   if (!saveId.IsValid())
   {
      return FTATCharacterDataContext{};
   }
   return FTATCharacterDataContext{ saveGame, saveId };
}

// static
FTATCharacterDataContext UTATCharacterDataContextUtils::GetCharacterDataContextFromController(APlayerController* controller)
{
   return (controller != nullptr)
      ? GetCharacterDataContextFromPlayerState(controller->PlayerState)
      : FTATCharacterDataContext{};
}
