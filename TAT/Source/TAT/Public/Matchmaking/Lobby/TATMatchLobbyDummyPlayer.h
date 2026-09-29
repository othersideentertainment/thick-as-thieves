// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "GameFramework/Actor.h"

// ue
#include "UObject/SoftObjectPtr.h"

#include "TATMatchLobbyDummyPlayer.generated.h"

class ATATPlayerState;
class UAnimSequence;
class USkeletalMesh;
struct FTATCharacterLoadoutEntry;
enum class ETATCharacter : uint8;

/// Actor used in the match lobby screen to spawn an instance of a player's character mesh.
/// Must be enough of these in match lobby level for supported player count!
UCLASS(Blueprintable)
class TAT_API ATATMatchLobbyDummyPlayer : public AActor
{
   GENERATED_BODY()

   ATATMatchLobbyDummyPlayer() {}

   // From AActor
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
   void AssignPlayer(ATATPlayerState* newPlayer);
   void ClearPlayer();

   UFUNCTION(BlueprintPure, Category = "TAT")
   bool IsClaimed() const { return _player.IsValid(); }

   UFUNCTION(BlueprintPure, Category = "TAT")
   const ATATPlayerState* GetPlayer() const { return _player.Get(); }

protected:
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT")
   void SetMesh(const USkeletalMesh* body, const USkeletalMesh* head, const UAnimSequence* pose);
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT")
   void ClearMesh();

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT")
   void SetIsReadyChecked(bool isReady);

   UFUNCTION(BlueprintNativeEvent, Category = "TAT")
   void OnPlayerChanged(const ATATPlayerState* player);
   void OnPlayerChanged_Implementation(const ATATPlayerState* player);

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT")
   void OnPlayerAssigned();

private:
   UFUNCTION()
   void _OnTATCharacterChanged(ATATPlayerState* playerState, ETATCharacter character);

   UFUNCTION()
   void _OnCharacterOutfitChanged(ATATPlayerState* playerState, const TArray<FTATCharacterLoadoutEntry>& outfitLoadout);

   UFUNCTION()
   void _OnIsPlayerReadyChanged(bool isReady);

public:
   /// Enable to reserve this instance for the local player (so they can be distinctly highlighted)
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT")
   bool IsForLocalPlayer = false;

   /// Enable to use the wardrobe pose instead of the lobby pose
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "TAT")
   bool UseWardrobePose = false;

private:
   UPROPERTY(Transient)
   TWeakObjectPtr<ATATPlayerState> _player = nullptr;
};

