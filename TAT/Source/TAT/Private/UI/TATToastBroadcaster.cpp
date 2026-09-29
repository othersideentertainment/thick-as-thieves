// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATToastBroadcaster.h"

// tat
#include "UI/TATToastSubsystem.h"
#include "GameFramework/TATGameMode.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerState.h"
#include "Developer/TATLootSettings.h"
#include "Loot/TATLootTypes.h"

// ue5
#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToastBroadcaster)

#define LOCTEXT_NAMESPACE "TATToastBroadcaster"

DEFINE_LOG_CATEGORY_STATIC(LogTATToastBroadcaster, Log, All);

ATATToastBroadcaster::ATATToastBroadcaster()
   : Super()
{
   bReplicates = true;
   bAlwaysRelevant = true;
   SetReplicatingMovement(false);
}

//static
ATATToastBroadcaster* ATATToastBroadcaster::Get(UObject* contextObject)
{
   if (UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull))
   {
      if (ATATGameMode* gameMode = world->GetAuthGameMode<ATATGameMode>())
      {
         return gameMode->GetToastBroadcaster();
      }
   }
   return nullptr;
}

void ATATToastBroadcaster::AuthoritySendUnformattedToastToAllPlayers(FGameplayTag toastId, FText message, TSoftObjectPtr<UPaperSprite> iconOverride, bool discardIfNotShownImmediately, bool sendReliable)
{
   check(HasAuthority());

#if WITH_EDITOR
   //NB. This does not check for all cases where replicating an FText is undesirable, but it does help with simple cases like a string converted directly to FText
   if (!message.ShouldGatherForLocalization())
   {
      UE_LOG(LogTATToastBroadcaster, Error, TEXT("AuthoritySendUnformattedToastToAllPlayers got a non-localized text value '%s'"), *message.ToString());
   }
#endif

   if (sendReliable)
   {
      _ClientSendUnformattedToastToAllPlayersReliable(toastId, message, iconOverride, discardIfNotShownImmediately);
   }
   else
   {
      _ClientSendUnformattedToastToAllPlayersUnreliable(toastId, message, iconOverride, discardIfNotShownImmediately);
   }
}

void ATATToastBroadcaster::ClientToastBroadcast_PlayerDisconnected_Implementation(const FString& disconnectingPlayerName)
{
   UE_LOG(LogTATToastBroadcaster, Log, TEXT("ClientToastBroadcast_PlayerDisconnected_Implementation | '%s'"), *disconnectingPlayerName);
   
   FFormatNamedArguments args;
   args.Add(TEXT("PlayerName"), FText::AsCultureInvariant(disconnectingPlayerName));
   
   _SendToastToAllLocalClients(
      PlayerDisconnectedEarly.Type,
      FText::Format(PlayerDisconnectedEarly.Text, args),
      PlayerDisconnectedEarly.IconOverride,
      PlayerDisconnectedEarly.DiscardIfNotShownImmediately);
}

void ATATToastBroadcaster::ClientToastBroadcast_PlayerKnockedOutByPlayer_Implementation(APlayerState* knockedOutPlayerState, APlayerState* instigatorPlayerState)
{
   if (knockedOutPlayerState == nullptr || instigatorPlayerState == nullptr)
   {
      return;
   }

   UE_LOG(LogTATToastBroadcaster, Log,
      TEXT("ClientToastBroadcast_PlayerKnockedOutByPlayer_Implementation | '%s' '%s'"),
      *knockedOutPlayerState->GetPlayerName(), *instigatorPlayerState->GetPlayerName());

   FFormatNamedArguments args;
   args.Add(TEXT("PlayerName"), FText::AsCultureInvariant(knockedOutPlayerState->GetPlayerName()));
   args.Add(TEXT("InstigatorName"), FText::AsCultureInvariant(instigatorPlayerState->GetPlayerName()));
   _SendToastToAllLocalClients(
      PlayerKnockedOutByPlayer.Type,
      FText::Format(PlayerKnockedOutByPlayer.Text, args),
      PlayerKnockedOutByPlayer.IconOverride,
      PlayerKnockedOutByPlayer.DiscardIfNotShownImmediately);
}

void ATATToastBroadcaster::ClientToastBroadcast_PlayerKnockedOutByGuard_Implementation(APlayerState* knockedOutPlayerState)
{
   if (knockedOutPlayerState == nullptr)
   {
      return;
   }

   UE_LOG(LogTATToastBroadcaster, Log,
      TEXT("ClientToastBroadcast_PlayerKnockedOutByGuard_Implementation | '%s'"),
      *knockedOutPlayerState->GetPlayerName());

   FFormatNamedArguments args;
   args.Add(TEXT("PlayerName"), FText::AsCultureInvariant(knockedOutPlayerState->GetPlayerName()));
   _SendToastToAllLocalClients(
      PlayerKnockedOutByGuard.Type,
      FText::Format(PlayerKnockedOutByGuard.Text, args),
      PlayerKnockedOutByGuard.IconOverride,
      PlayerKnockedOutByGuard.DiscardIfNotShownImmediately);
}

void ATATToastBroadcaster::ClientToastBroadcast_PlayerDepositedMajorLoot_Implementation(APlayerState* depositingPlayer, const FTATLootIdentifier& lootIdentifier)
{
   if (depositingPlayer == nullptr)
   {
      return;
   }

   UE_LOG(LogTATToastBroadcaster, Log,
      TEXT("ClientToastBroadcast_PlayerDepositedMajorLoot_Implementation | '%s' '%s'"),
      *depositingPlayer->GetPlayerName(), *lootIdentifier.LootTag.ToString());

   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, lootIdentifier);
   if (lootInfo == nullptr)
   {
      UE_LOG(LogTATToastBroadcaster, Error, TEXT("ClientToastBroadcast_PlayerDepositedMajorLoot got an invalid loot identifier '%s'"), *lootIdentifier.LootTag.ToString());
      return;
   }

   FFormatNamedArguments args;
   args.Add(TEXT("PlayerName"), FText::AsCultureInvariant(depositingPlayer->GetPlayerName()));
   args.Add(TEXT("MajorLootName"), lootInfo->DisplayName);

   // Pick a different message for local players
   const FTATToastMessageConfig& toastMessage = _IsLocalPlayer(depositingPlayer) ? LocalPlayerDepositedMajorLoot : RemotePlayerDepositedMajorLoot;
   _SendToastToAllLocalClients(
      toastMessage.Type,
      FText::Format(toastMessage.Text, args),
      toastMessage.IconOverride ? toastMessage.IconOverride : lootInfo->DisplaySprite,
      toastMessage.DiscardIfNotShownImmediately);
}



void ATATToastBroadcaster::ClientToastBroadcast_PlayerDroppedMajorLoot_Implementation(APlayerState* droppingPlayer, const FTATLootIdentifier& lootIdentifier)
{
   if (droppingPlayer == nullptr)
   {
      return;
   }

   UE_LOG(LogTATToastBroadcaster, Log,
      TEXT("ClientToastBroadcast_PlayerDroppedMajorLoot_Implementation | '%s' '%s'"),
      *droppingPlayer->GetPlayerName(), *lootIdentifier.LootTag.ToString());

   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, lootIdentifier);
   if (lootInfo == nullptr)
   {
      UE_LOG(LogTATToastBroadcaster, Error, TEXT("ClientToastBroadcast_PlayerDroppedMajorLoot got an invalid loot identifier '%s'"), *lootIdentifier.LootTag.ToString());
      return;
   }

   FFormatNamedArguments args;
   args.Add(TEXT("PlayerName"), FText::AsCultureInvariant(droppingPlayer->GetPlayerName()));
   args.Add(TEXT("MajorLootName"), lootInfo->DisplayName);
   
   const FTATToastMessageConfig& toastMessage = PlayerDroppedMajorLoot;
   _SendToastToAllLocalClients(
      toastMessage.Type,
      FText::Format(toastMessage.Text, args),
      toastMessage.IconOverride ? toastMessage.IconOverride : lootInfo->DisplaySprite,
      toastMessage.DiscardIfNotShownImmediately);
}

void ATATToastBroadcaster::ClientToastBroadcast_PlayerClaimedSafeRoom_Implementation(APlayerState* claimingPlayer, ATATSafeRoom* safeRoom)
{
   if (claimingPlayer == nullptr)
   {
      return;
   }

   UE_LOG(LogTATToastBroadcaster, Log,
      TEXT("ClientToastBroadcast_PlayerClaimedSafeRoom_Implementation | '%s'"),
      *claimingPlayer->GetPlayerName());

   FFormatNamedArguments args;
   args.Add(TEXT("PlayerName"), FText::AsCultureInvariant(claimingPlayer->GetPlayerName()));
   _SendToastToAllLocalClients(
      PlayerClaimedSafeRoom.Type,
      FText::Format(PlayerClaimedSafeRoom.Text, args),
      PlayerClaimedSafeRoom.IconOverride,
      PlayerClaimedSafeRoom.DiscardIfNotShownImmediately);
}

void ATATToastBroadcaster::ClientToastBroadcast_PlayerPickpocketedLoot_Implementation(APlayerState* pickpocketingPlayer, APlayerState* victimPlayer, const FTATLootIdentifier& lootIdentifier, bool droppedOnGround)
{
   if (pickpocketingPlayer == nullptr || victimPlayer == nullptr)
   {
      return;
   }

   UE_LOG(LogTATToastBroadcaster, Log,
      TEXT("ClientToastBroadcast_PlayerPickpocketedLoot_Implementation | pickpocketingPlayer='%s', victimPlayer='%s', lootId='%s'"),
      *pickpocketingPlayer->GetPlayerName(),
      *victimPlayer->GetPlayerName(),
      *lootIdentifier.ToString());

   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, lootIdentifier);
   if (lootInfo == nullptr)
   {
      UE_LOG(LogTATToastBroadcaster, Error, TEXT("ClientToastBroadcast_PlayerPickpocketedLoot_Implementation got an invalid loot identifier '%s'"), *lootIdentifier.LootTag.ToString());
      return;
   }

   const FTATToastMessageConfig& pickpocketMessage = droppedOnGround ? PlayerDroppedLoot : PlayerPickpocketedLoot;

   FFormatNamedArguments args;
   args.Add(TEXT("PlayerName"), FText::AsCultureInvariant(pickpocketingPlayer->GetPlayerName()));
   args.Add(TEXT("VictimPlayerName"), FText::AsCultureInvariant(victimPlayer->GetPlayerName()));
   args.Add(TEXT("LootName"), lootInfo->DisplayName);
   _SendToastToAllLocalClients(
      pickpocketMessage.Type,
      FText::Format(pickpocketMessage.Text, args),
      pickpocketMessage.IconOverride ? pickpocketMessage.IconOverride : lootInfo->DisplaySprite,
      pickpocketMessage.DiscardIfNotShownImmediately);
}

void ATATToastBroadcaster::ClientToastBroadcast_PlayerRespawned_Implementation(APlayerState* respawningPlayer)
{
   if (respawningPlayer == nullptr)
   {
      return;
   }

   UE_LOG(LogTATToastBroadcaster, Log,
      TEXT("ClientToastBroadcast_PlayerRespawned_Implementation | respawningPlayer='%s'"),
      *respawningPlayer->GetPlayerName());

   FFormatNamedArguments args;
   args.Add(TEXT("PlayerName"), FText::AsCultureInvariant(respawningPlayer->GetPlayerName()));

   _SendToastToAllLocalClients(
      PlayerRespawned.Type,
      FText::Format(PlayerRespawned.Text, args),
      PlayerRespawned.IconOverride,
      PlayerRespawned.DiscardIfNotShownImmediately);
}

void ATATToastBroadcaster::ClientToastBroadcast_PlayerRespawnTotemActivated_Implementation(APlayerState* respawningPlayer)
{
   if (respawningPlayer == nullptr)
   {
      return;
   }

   UE_LOG(LogTATToastBroadcaster, Log,
      TEXT("ClientToastBroadcast_PlayerRespawnTotemActivated_Implementation | respawningPlayer='%s'"),
      *respawningPlayer->GetPlayerName());

   FFormatNamedArguments args;
   args.Add(TEXT("PlayerName"), FText::AsCultureInvariant(respawningPlayer->GetPlayerName()));
   _SendToastToAllLocalClients(
      PlayerRespawnTotemActivated.Type,
      FText::Format(PlayerRespawnTotemActivated.Text, args),
      PlayerRespawnTotemActivated.IconOverride,
      PlayerRespawnTotemActivated.DiscardIfNotShownImmediately);
}

void ATATToastBroadcaster::_ClientSendUnformattedToastToAllPlayersReliable_Implementation(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& iconOverride, bool discardIfNotShownImmediately)
{
   _SendToastToAllLocalClients(toastId, message, iconOverride, discardIfNotShownImmediately);
}

void ATATToastBroadcaster::_ClientSendUnformattedToastToAllPlayersUnreliable_Implementation(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& iconOverride, bool discardIfNotShownImmediately)
{
   _SendToastToAllLocalClients(toastId, message, iconOverride, discardIfNotShownImmediately);
}

void ATATToastBroadcaster::_SendToastToAllLocalClients(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& iconOverride, bool discardIfNotShownImmediately)
{
   UE_LOG(LogTATToastBroadcaster, Log,
      TEXT("_SendToastToAllLocalClients | Toast mssage '%s', '%s'"),
      *toastId.ToString(), *message.ToString());

   if (GetNetMode() == NM_DedicatedServer)
   {
      return;
   }

   ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>();
   if (gameState == nullptr)
   {
      return;
   }

   for (ATATPlayerState* playerState : gameState->GetTATPlayerStates())
   {
      if (!playerState || !playerState->IsValidLowLevel())
      {
         continue;
      }

      APlayerController* ownerController = playerState->GetOwner<APlayerController>();
      if (!ownerController || !ownerController->IsLocalController())
      {
         continue;
      }

      if (ULocalPlayer* localPlayer = ownerController->GetLocalPlayer())
      {
         if (UTATToastSubsystem* toastSubsystem = localPlayer->GetSubsystem<UTATToastSubsystem>())
         {
            toastSubsystem->RequestToastMessage(toastId, message, iconOverride, discardIfNotShownImmediately);
         }
      }
   }
}

bool ATATToastBroadcaster::_IsLocalPlayer(APlayerState* playerState) const
{
   if (playerState == nullptr || !playerState->IsValidLowLevel() || GetNetMode() == NM_DedicatedServer)
   {
      return false;
   }

   APlayerController* ownerController = playerState->GetOwner<APlayerController>();
   return ownerController != nullptr && ownerController->IsLocalController();
}

#undef LOCTEXT_NAMESPACE
