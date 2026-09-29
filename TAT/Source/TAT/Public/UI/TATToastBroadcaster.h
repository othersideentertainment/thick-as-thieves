// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameFramework/Actor.h"
#include "Paper2D/Classes/PaperSprite.h"
#include "GameplayTagContainer.h"

#include "TATToastBroadcaster.generated.h"

struct FTATLootIdentifier;
class ATATSafeRoom;

USTRUCT(BlueprintType)
struct TAT_API FTATToastMessageConfig
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Toast Message Config", Meta = (Categories = "Toast.Type"))
   FGameplayTag Type;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Toast Message Config")
   FText Text;

   /// Icon to use in the message. If not specified, the message will use the default icon for the toast type.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Toast Message Config", AdvancedDisplay)
   TSoftObjectPtr<UPaperSprite> IconOverride;

   /// If enabled and the message can't be immediately displayed (eg. no available space on-screen), discard it instead of queuing it.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Toast Message Config", AdvancedDisplay)
   bool DiscardIfNotShownImmediately = false;
};

// "Singleton" actor, marked "always relevant", that is used to handle broadcasting toast messages to all players.
UCLASS(Blueprintable)
class TAT_API ATATToastBroadcaster : public AActor
{
   GENERATED_BODY()

public:
   ATATToastBroadcaster();

   UFUNCTION(BlueprintPure, Category = "TAT|ToastBroadcaster", meta = (WorldContext = "contextObject", DisplayName = "Get Toast Broadcaster"))
   static ATATToastBroadcaster* Get(UObject* contextObject);

   /// Sends a toast to all players.
   /// @param message An unformatted localized text value (eg. not created from a string or with "Format Text")
   /// @param iconOverride Icon to use in the message. If not specified, the message will use the default icon for the toastId.
   /// @param discardIfNotShownImmediately If enabled and the message can't be immediately displayed (eg. no available space on-screen), discard it instead of queuing it.
   /// @param sendReliable If enabled, the message will guarantee delivery. This means resending repeatedly if needed, blocking other toast broadcasts until successfully sent.
   ///                     Do not enable this unless it's very important for all players to receive this message.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "TAT|ToastBroadcaster")
   void AuthoritySendUnformattedToastToAllPlayers(UPARAM(Meta = (Categories = "Toast.Type")) FGameplayTag toastId, FText message, TSoftObjectPtr<UPaperSprite> iconOverride = nullptr, bool discardIfNotShownImmediately = false, bool sendReliable = false);

   UFUNCTION(NetMulticast, Unreliable)
   void ClientToastBroadcast_PlayerKnockedOutByPlayer(APlayerState* knockedOutPlayerState, APlayerState* instigatorPlayerState);

   UFUNCTION(NetMulticast, Unreliable)
   void ClientToastBroadcast_PlayerKnockedOutByGuard(APlayerState* knockedOutPlayerState);

   UFUNCTION(NetMulticast, Unreliable)
   void ClientToastBroadcast_PlayerDepositedMajorLoot(APlayerState* depositingPlayer, const FTATLootIdentifier& lootInfo);
   
   UFUNCTION(NetMulticast, Unreliable)
   void ClientToastBroadcast_PlayerDroppedMajorLoot(APlayerState* droppingPlayer, const FTATLootIdentifier& lootInfo);

   UFUNCTION(NetMulticast, Unreliable)
   void ClientToastBroadcast_PlayerClaimedSafeRoom(APlayerState* claimingPlayer, ATATSafeRoom* safeRoom);

   UFUNCTION(NetMulticast, Unreliable)
   void ClientToastBroadcast_PlayerPickpocketedLoot(APlayerState* pickpocketingPlayer, APlayerState* victimPlayer, const FTATLootIdentifier& lootIdentifier, bool droppedOnGround);

   UFUNCTION(NetMulticast, Unreliable)
   void ClientToastBroadcast_PlayerRespawned(APlayerState* respawningPlayer);

   UFUNCTION(NetMulticast, Unreliable)
   void ClientToastBroadcast_PlayerRespawnTotemActivated(APlayerState* respawningPlayer);

   // Sending FString name directly, since the player state will no longer exist
   UFUNCTION(NetMulticast, Unreliable)
   void ClientToastBroadcast_PlayerDisconnected(const FString& disconnectingPlayerName);

protected:
   /// Toast message sent when a player is knocked out by another player
   /// The text should contain the following field(s): {PlayerName}, {InstigatorName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig PlayerKnockedOutByPlayer;

   /// Toast message sent when a player is knocked out by a guard
   /// The text should contain the following field(s): {PlayerName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig PlayerKnockedOutByGuard;

   /// Toast message sent when the local player deposits major loot to a loot stash
   /// The text should contain the following field(s): {MajorLootName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig LocalPlayerDepositedMajorLoot;

   /// Toast message sent when a remote player deposits major loot to a loot stash
   /// The text should contain the following field(s): {PlayerName} {MajorLootName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig RemotePlayerDepositedMajorLoot;

   /// Toast message sent when a player drops major loot
   /// The text should contain the following field(s): {PlayerName} {MajorLootName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig PlayerDroppedMajorLoot;

   /// Toast message sent when a player claims a new safe room
   /// The text should contain the following field(s): {PlayerName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig PlayerClaimedSafeRoom;

   /// Toast message sent when a player pickpockets loot from another player
   /// The text should contain the following field(s): {PlayerName} {VictimPlayerName} {LootName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig PlayerPickpocketedLoot;

   /// Toast message sent when a player pickpockets loot from another player, their inventory was full, and the item was dropped on the ground
   /// The text should contain the following field(s): {PlayerName} {VictimPlayerName} {LootName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig PlayerDroppedLoot;

   /// Toast message sent when a player respawns
   /// The text should contain the following field(s): {PlayerName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig PlayerRespawned;

   /// Toast message sent when a player's respawn totem activates
   /// The text should contain the following field(s): {PlayerName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig PlayerRespawnTotemActivated;

   /// Toast message sent when a player disconnects early
   /// The text should contain the following field(s): {PlayerName}
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Toasts")
   FTATToastMessageConfig PlayerDisconnectedEarly;

private:
   UFUNCTION(NetMulticast, Reliable)
   void _ClientSendUnformattedToastToAllPlayersReliable(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& iconOverride, bool discardIfNotShownImmediately);

   UFUNCTION(NetMulticast, Unreliable)
   void _ClientSendUnformattedToastToAllPlayersUnreliable(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& iconOverride, bool discardIfNotShownImmediately);

   // Sends the specified toast message to all local players
   void _SendToastToAllLocalClients(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& iconOverride = nullptr, bool discardIfNotShownImmediately = false);

   // Checks if the specified player state is local to this client
   bool _IsLocalPlayer(APlayerState* playerState) const;
};
