// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "GameFramework/OSEGameMode.h"

#include "TATThievesDenGameMode.generated.h"

class APlayerStart;
class UTATCharactersMetadata;
enum class ETATCharacter : uint8;

UCLASS()
class TAT_API ATATThievesDenGameMode : public AOSEGameMode
{
   GENERATED_BODY()

public:
   ATATThievesDenGameMode();

   UFUNCTION(BlueprintPure, DisplayName = "Get TAT Thieves' Den Game Mode", Category = "Thieves Den", meta = (WorldContext = "worldContext"))
   static ATATThievesDenGameMode* Get(const UObject* worldContext);

   // from AGameModeBase
   virtual void StartPlay() override;
   virtual void PostInitializeComponents() override;
   virtual void BeginPlay() override;
   virtual bool AllowCheats(APlayerController* pc) override;
   virtual APlayerController* Login(UPlayer* newPlayer, ENetRole inRemoteRole, const FString& portal, const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage) override;
   virtual void PostLogin(APlayerController* newPlayer) override;
   virtual void Logout(AController* exitingController);
   virtual void GetSeamlessTravelActorList(bool toTransition, TArray<AActor*>& actorList) override;
   virtual void HandleSeamlessTravelPlayer(AController*& pc) override;
   virtual UClass* GetDefaultPawnClassForController_Implementation(AController* controller) override;
   bool MustSpectate_Implementation(APlayerController* newPlayerController) const override;
   virtual AActor* ChoosePlayerStart_Implementation(AController* controller) override;

   UFUNCTION(BlueprintPure, Category = "Thieves Den Game Mode")
   ETATCharacter GetCharacterType(AController* controller) const;

protected:
   /// If enabled, checks the game instance to see if the player has selected a character type.
   /// If so, uses the pawn class specified in CharactersMetadata for that type.
   UPROPERTY(EditDefaultsOnly, Category = "Thieves Den Game Mode - Character Type")
   bool UsePawnCharacterTypeFromPlayerState = true;

   /// If we can't otherwise determine what character type the player is, fall back on this type (if specified).
   UPROPERTY(EditDefaultsOnly, Category = "Thieves Den Game Mode - Character Type")
   ETATCharacter DefaultPawnCharacterType = static_cast<ETATCharacter>(0);

   /// Always force this character type to be used
   UPROPERTY(EditDefaultsOnly, Category = "Thieves Den Game Mode - Character Type")
   ETATCharacter OverridePawnCharacterType = static_cast<ETATCharacter>(0);

private:
   const UTATCharactersMetadata* GetCharactersMetadata() const;

   UPROPERTY(Transient)
   const UTATCharactersMetadata* _charactersMetadata = nullptr;
};
