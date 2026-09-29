// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ThievesDen/TATThievesDenGameMode.h"

// tat
#include "Developer/TATEditorSettings.h"
#include "Developer/TATProjectSettings.h"
#include "Online/TATGameSession.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "Player/TATCharacter.h"
#include "Player/TATSpectatorPawn.h"
#include "UI/TATHUD.h"
#include "TATGameInstance.h"
#include "Character/TATCharacterMetadata.h"
#include "GameFramework/TATPlayerStart.h"
#include "Settings/TATMatchSettingsPropertyDef.h"
#include "Character/TATTeams.h"
#include "Analytics/TATAnalyticsManager.h"

// ose
#include "ServerManager/OSEDedicatedServerManagerBase.h"
#include "OSECoreCheats.h"

// ue
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/PlayerStartPIE.h"
#include "Math/Color.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThievesDenGameMode)

DEFINE_LOG_CATEGORY_STATIC(LogTATThievesDenGameMode, Log, All);

ATATThievesDenGameMode::ATATThievesDenGameMode()
   : Super()
{
   // Use our custom classes
   DefaultPawnClass = ATATCharacter::StaticClass();
   GameSessionClass = ATATGameSession::StaticClass();
   GameStateClass = ATATGameState::StaticClass();
   HUDClass = ATATHUD::StaticClass();
   PlayerControllerClass = ATATPlayerController::StaticClass();
   PlayerStateClass = ATATPlayerState::StaticClass();
   SpectatorClass = ATATSpectatorPawn::StaticClass();
}

// static
ATATThievesDenGameMode* ATATThievesDenGameMode::Get(const UObject* worldContext)
{
   if (UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull))
   {
      return world->GetAuthGameMode<ATATThievesDenGameMode>();
   }
   return nullptr;
}

void ATATThievesDenGameMode::StartPlay()
{
   Super::StartPlay();
}

void ATATThievesDenGameMode::PostInitializeComponents()
{
   Super::PostInitializeComponents();
}

void ATATThievesDenGameMode::BeginPlay()
{
   Super::BeginPlay();

#if OSE_CHEATS_ENABLED
   // If we have any default match settings to apply, do it now
   UTATGameInstance& gameInstance = UTATGameInstance::Get(this);
   UTATEditorSettings::Get().ApplyDefaultMatchSettings(gameInstance.GetMatchSettings(), FTATMatchSettingsQueryContext::MakeFromWorldContext(this));
   gameInstance.AuthorityNotifyCheatUpdatedMatchSettings();
#endif
   
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      if (save->HasSentUnlocksAnalytics() == false)
      {
         if (UTATAnalyticsManager* analyticsManager = save->GetAnalyticsManager())
         {
            const FTATPlayerProgression& playerData = save->GetPlayerProgression();
            for (FGameplayTag unlockedContent : playerData.UnlockedContent)
            {
               analyticsManager->HandleContentUnlocked(unlockedContent);
            }
            analyticsManager->HandlePlayerLevelChanged(playerData.XP.Level);
         }
         save->SetHasSentUnlocksAnalytics();
      }
   }
}

bool ATATThievesDenGameMode::AllowCheats(APlayerController* pc)
{
   if (GetNetMode() == NM_Standalone || GIsEditor)
   {
      return true;
   }

#if UE_BUILD_DEVELOPMENT
   // dev builds allow cheating across the board
   return true;
#else
   return false;
#endif
}

APlayerController* ATATThievesDenGameMode::Login(UPlayer* newPlayer, ENetRole inRemoteRole, const FString& portal, const FString& options, const FUniqueNetIdRepl& uniqueId, FString& errorMessage)
{
   APlayerController* pc = Super::Login(newPlayer, inRemoteRole, portal, options, uniqueId, errorMessage);

   if (pc && IsNetMode(NM_DedicatedServer))
   {
      UOSEDedicatedServerManagerBase& serverMgr = UOSEDedicatedServerManagerBase::Get(*this);
      serverMgr.Login(options, uniqueId, errorMessage);
   }

   return pc;
}

void ATATThievesDenGameMode::PostLogin(APlayerController* newPlayer)
{
   Super::PostLogin(newPlayer);
   check(newPlayer != nullptr);
   const UTATProjectSettings& settings = UTATProjectSettings::Get();
   ATATPlayerState* playerState = newPlayer->GetPlayerState<ATATPlayerState>();
   check(playerState != nullptr);
   const uint8 playerTeamVal = UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Player);
   playerState->AuthoritySetTeam(playerTeamVal);
}

void ATATThievesDenGameMode::Logout(AController* exitingController)
{
   Super::Logout(exitingController);

   if (IsNetMode(NM_DedicatedServer))
   {
      // The subsystem may have been torn down already when ending PIE (as of 5.4)
      if (UOSEDedicatedServerManagerBase* serverMgr = UOSEDedicatedServerManagerBase::TryGet(*this))
      {
         serverMgr->Logout(exitingController);
      }
   }
}

void ATATThievesDenGameMode::GetSeamlessTravelActorList(bool toTransition, TArray<AActor*>& actorList)
{
   Super::GetSeamlessTravelActorList(toTransition, actorList);

   // server can add actors to seamless travel here
   for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
   {
      APlayerController* pc = it->Get();
      actorList.AddUnique(pc);
      if (APawn* pawn = pc->GetPawn())
      {
         actorList.AddUnique(pawn);
      }
   }
}

void ATATThievesDenGameMode::HandleSeamlessTravelPlayer(AController*& pc)
{
   Super::HandleSeamlessTravelPlayer(pc);
}

bool ATATThievesDenGameMode::MustSpectate_Implementation(APlayerController* newPlayerController) const
{
   if (Super::MustSpectate_Implementation(newPlayerController))
   {
      return true;
   }

   if (ATATPlayerState* playerState = newPlayerController->GetPlayerState<ATATPlayerState>())
   {
      const ETATCharacter characterType = playerState->GetTATCharacter();
      return characterType == ETATCharacter::None;
   }

   return false;
}

UClass* ATATThievesDenGameMode::GetDefaultPawnClassForController_Implementation(AController* controller)
{
   const ETATCharacter characterType = GetCharacterType(controller);
   const UTATCharactersMetadata* metadata = GetCharactersMetadata();
   if (characterType != ETATCharacter::None && metadata != nullptr)
   {
      const FTATCharacterMetadata& characterMetadata = metadata->GetCharacterMetadata(characterType);
      if (!characterMetadata.BlueprintClass.IsNull())
      {
         return characterMetadata.BlueprintClass.LoadSynchronous();
      }
   }

   return Super::GetDefaultPawnClassForController_Implementation(controller);
}

AActor* ATATThievesDenGameMode::ChoosePlayerStart_Implementation(AController* controller)
{
#if WITH_EDITOR
   // Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
   for (TActorIterator<APlayerStartPIE> it(GetWorld()); it; ++it)
   {
      APlayerStartPIE* playerStartPIE = (*it);
      if (playerStartPIE != nullptr)
      {
         return playerStartPIE;
      }
   }
#endif

   UWorld* world = GetWorld();
   UClass* pawnClass = GetDefaultPawnClassForController(controller);
   APawn* pawnToFit = pawnClass ? pawnClass->GetDefaultObject<APawn>() : nullptr;
   TArray<APlayerStart*, TInlineAllocator<8>> allPlayerStarts;

   // Use the first player start that the pawn has space to spawn in
   for (TActorIterator<APlayerStart> it(world); it; ++it)
   {
      APlayerStart* playerStart = *it;
      if (playerStart == nullptr)
      {
         continue;
      }
      allPlayerStarts.Add(playerStart);
   }

   auto isReasonablePlayerStart = [world, pawnToFit](APlayerStart* playerStart) -> bool
   {
      check(playerStart != nullptr);
      FVector actorLocation = playerStart->GetActorLocation();
      const FRotator actorRotation = playerStart->GetActorRotation();
      if (!world->EncroachingBlockingGeometry(pawnToFit, actorLocation, actorRotation))
      {
         return true;
      }
      if (world->FindTeleportSpot(pawnToFit, actorLocation, actorRotation))
      {
         return true;
      }
      return false;
   };

   // Pick the player start with the lowest index
   APlayerStart* bestPlayerStart = nullptr;
   int32 bestPlayerStartIndex = std::numeric_limits<int32>::max();
   for (APlayerStart* playerStart : allPlayerStarts)
   {
      check(playerStart != nullptr);
      if (!isReasonablePlayerStart(playerStart))
      {
         continue;
      }
      ATATPlayerStart* tatPlayerStart = Cast<ATATPlayerStart>(playerStart);
      const int32 index = (tatPlayerStart != nullptr) ? tatPlayerStart->GetAreaIndex() : 999999;
      if (index < bestPlayerStartIndex)
      {
         bestPlayerStart = playerStart;
         bestPlayerStartIndex = index;
      }
   }

   if (bestPlayerStart != nullptr)
   {
      return bestPlayerStart;
   }

   // Fallback - pick the first reasonable player start at random
   allPlayerStarts.Sort([](const APlayerStart& lhs, const APlayerStart& rhs) { return FMath::FRand() < 0.5f; });
   for (APlayerStart* playerStart : allPlayerStarts)
   {
      if (isReasonablePlayerStart(playerStart))
      {
         return playerStart;
      }
   }

   return nullptr;
}

ETATCharacter ATATThievesDenGameMode::GetCharacterType(AController* controller) const
{
   auto isValidCharacterType = [](ETATCharacter charType) -> bool
   {
      return charType != ETATCharacter::None && static_cast<int32>(charType) < static_cast<int32>(ETATCharacter::MAX);
   };

   if (isValidCharacterType(OverridePawnCharacterType))
   {
      return OverridePawnCharacterType;
   }

   if (UsePawnCharacterTypeFromPlayerState)
   {
      if (ATATPlayerState* playerState = controller->GetPlayerState<ATATPlayerState>())
      {
         const ETATCharacter characterType = playerState->GetTATCharacter();
         if (isValidCharacterType(characterType))
         {
            return characterType;
         }
      }
   }

   if (isValidCharacterType(DefaultPawnCharacterType))
   {
      return DefaultPawnCharacterType;
   }

   return ETATCharacter::None;
}

const UTATCharactersMetadata* ATATThievesDenGameMode::GetCharactersMetadata() const
{
   if (_charactersMetadata == nullptr)
   {
      // We need access to this very early on in the init process, so loading this async isn't all that practical
      const_cast<ATATThievesDenGameMode*>(this)->_charactersMetadata = UTATProjectSettings::Get().DefaultCharacterMetadata.LoadSynchronous();
   }
   return _charactersMetadata;
}
