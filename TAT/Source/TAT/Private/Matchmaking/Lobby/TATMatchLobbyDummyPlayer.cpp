// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Matchmaking/Lobby/TATMatchLobbyDummyPlayer.h"

// tat
#include "Character/TATCharacterMetadata.h"
#include "CharacterCustomization/TATCharacterOutfits.h"
#include "Developer/TATOutfitSettings.h"
#include "Player/TATPlayerState.h"

// ue
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMatchLobbyDummyPlayer)
DEFINE_LOG_CATEGORY_STATIC(LogTATMatchLobbyDummyPlayer, Log, All);

void ATATMatchLobbyDummyPlayer::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (ATATPlayerState* player = _player.Get())
   {
      player->OnTATCharacterChanged.RemoveAll(this);
      player->OnCharacterOutfitChanged.RemoveAll(this);
      player->OnIsReadyCheckedChanged.RemoveAll(this);
   }

   Super::EndPlay(endPlayReason);
}

void ATATMatchLobbyDummyPlayer::AssignPlayer(ATATPlayerState* newPlayer)
{
   if (newPlayer == _player)
   {
      return;
   }

   UE_LOG(LogTATMatchLobbyDummyPlayer, Verbose, TEXT("Player changed (%s -> %s)")
      , *GetNameSafe(_player.Get())
      , *GetNameSafe(newPlayer));

   if (ATATPlayerState* oldPlayer = _player.Get())
   {
      oldPlayer->OnTATCharacterChanged.RemoveAll(this);
      oldPlayer->OnCharacterOutfitChanged.RemoveAll(this);
      oldPlayer->OnIsReadyCheckedChanged.RemoveAll(this);
   }

   _player = newPlayer;
   OnPlayerAssigned();

   if (newPlayer)
   {
      check(IsForLocalPlayer == newPlayer->IsLocalPlayerState());

      newPlayer->OnIsReadyCheckedChanged.AddDynamic(this, &ATATMatchLobbyDummyPlayer::_OnIsPlayerReadyChanged);

      // NOTE: Player state can be added before character type is retrieved from save data.
      // So check for this and subscribe to subsequent character-change.
      newPlayer->OnTATCharacterChanged.AddDynamic(this, &ATATMatchLobbyDummyPlayer::_OnTATCharacterChanged);
      newPlayer->OnCharacterOutfitChanged.AddDynamic(this, &ATATMatchLobbyDummyPlayer::_OnCharacterOutfitChanged);
      if (newPlayer->GetTATCharacter() == ETATCharacter::None)
      {
         return;
      }
   }

   OnPlayerChanged(newPlayer);
}

void ATATMatchLobbyDummyPlayer::ClearPlayer()
{
   ATATPlayerState* newPlayer = nullptr;
   AssignPlayer(newPlayer);
}

void ATATMatchLobbyDummyPlayer::OnPlayerChanged_Implementation(const ATATPlayerState* player)
{
   check(!IsNetMode(NM_DedicatedServer));
   if (player == nullptr)
   {
      ClearMesh();
      return;
   }

   // Reflect whether player is ready
   SetIsReadyChecked(player->IsReadyChecked());

   // Load mesh from character metadata and pipe up to BP
   const UTATCharactersMetadata* characterMetadataAsset = UTATCharacterMetadataFunctionLibrary::GetCharactersMetadataAsset();
   check(characterMetadataAsset);
   const ETATCharacter character = player->GetTATCharacter();
   if (character == ETATCharacter::None)
   {
      UE_LOG(LogTATMatchLobbyDummyPlayer, Error, TEXT("[%s] OnPlayerChanged() called for player %s without a valid ETATCharacter selection!")
         , *GetName()
         , *player->GetName());
      ClearMesh();
      return;
   }

   // Load character mesh from character metadata
   const FTATCharacterMetadata& metadata = characterMetadataAsset->GetCharacterMetadata(character);
   TSoftObjectPtr<USkeletalMesh> characterBodyMesh = metadata.CharacterBodyMesh;
   TSoftObjectPtr<USkeletalMesh> characterHeadMesh = metadata.CharacterHeadMesh;

   //Look for outfit loadout metadata and set the meshes to be loaded from there instead
   for (const FTATCharacterLoadoutEntry& entry : player->GetCurrentOutfitLoadout())
   {
      const FTATOutfitsMetadataTableRow* entryMetadata = UTATOutfitSettings::Get().FindOutfitMetadata(entry.LoadoutTag);

      if (entryMetadata && !entryMetadata->OutfitSkeletalMesh.IsNull())
      {
         if (entry.OutfitSlot == ETATCharacterOutfitSlot::Head)
         {
            characterHeadMesh = entryMetadata->OutfitSkeletalMesh;
         }
         else if (entry.OutfitSlot == ETATCharacterOutfitSlot::Body)
         {
            characterBodyMesh = entryMetadata->OutfitSkeletalMesh;
         }
      }
   }

   TSoftObjectPtr<UAnimSequence> characterPose = metadata.CharacterLobbyPose;

   if (UseWardrobePose)
   {
      characterPose = metadata.CharacterWardrobePose;
   }
   else if (!player->IsLocalPlayerState() && !metadata.CharacterAlternateLobbyPose.IsNull())
   {
      characterPose = metadata.CharacterAlternateLobbyPose;
   }
   
   TArray<FSoftObjectPath> pathsToLoad { characterBodyMesh.ToSoftObjectPath(), characterHeadMesh.ToSoftObjectPath(), characterPose.ToSoftObjectPath() };
   UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(pathsToLoad), 
      [weakThis = MakeWeakObjectPtr(this), characterBodyMesh, characterHeadMesh, characterPose] {
      if (ATATMatchLobbyDummyPlayer* self = weakThis.Get())
      {
         self->SetMesh(characterBodyMesh.Get(), characterHeadMesh.Get(), characterPose.Get());
      }
   });
}

void ATATMatchLobbyDummyPlayer::_OnTATCharacterChanged(ATATPlayerState* playerState, ETATCharacter character)
{
   check(playerState);
   check(playerState == _player);
   OnPlayerChanged(playerState);
}

void ATATMatchLobbyDummyPlayer::_OnCharacterOutfitChanged(ATATPlayerState* playerState, const TArray<FTATCharacterLoadoutEntry>& outfitLoadout)
{
   check(playerState);
   check(playerState == _player);
   OnPlayerChanged(playerState);
}

void ATATMatchLobbyDummyPlayer::_OnIsPlayerReadyChanged(bool isReady)
{
   // Should never be called without valid player!
   check(_player.IsValid());
   SetIsReadyChecked(isReady);
}
