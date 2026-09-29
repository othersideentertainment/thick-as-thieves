// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Maps/TATMapNodeSettings.h"

// ose
#include "OSEGameInstance.h"

// ue5
#include "CoreMinimal.h"
#include "GameFramework/OnlineReplStructs.h"
#include "GameplayTagContainer.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "TATGameInstance.generated.h"

class FOSEMetricsSystem;
class FOSEGameSessionMetrics;

class UTATTravelMgr;
class UTATAnalyticsMgr;
class UTATMatchSettingsBase;

struct FTATPartyContracts
{
   FGameplayTag ContractTag;
   TArray<FUniqueNetIdRepl> PlayersOnContract;

   void Reset()
   {
      ContractTag = FGameplayTag();
      PlayersOnContract.Reset();
   }
};

UCLASS()
class TAT_API UTATGameInstance : public UOSEGameInstance
{
   GENERATED_BODY()

public:
   // statics
   static UTATGameInstance& Get(const UObject* contextObject);

   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT", meta = (WorldContext = "contextObject"))
   static UTATGameInstance* GetTATGameInstance(const UObject* contextObject);

   // from UGameInstance
   virtual void Init() override;
   virtual void OnWorldChanged(UWorld* oldWorld, UWorld* newWorld) override;
   virtual TSubclassOf<UOnlineSession> GetOnlineSessionClass() override;
   virtual void Shutdown() override;

   UTATTravelMgr& GetTravelMgr() const { check(_travelMgr); return *_travelMgr; }
   UTATAnalyticsMgr& GetAnalyticsMgr() { check(_analyticsMgr); return *_analyticsMgr; }
   const UTATAnalyticsMgr& GetAnalyticsMgr() const { check(_analyticsMgr); return *_analyticsMgr; }

   UFUNCTION(BlueprintCallable, Category = "Game Instance|TAT")
   void ShowLoadingScreen();

   // NOTE: should only be used in cases where the loading screen was opened prematurely by ShowLoadingScreen
   UFUNCTION(BlueprintCallable, Category = "Game Instance|TAT")
   void HideLoadingScreen();
   
   // Checks if we have match settings
   bool HasMatchSettings() const { return _matchSettings != nullptr; }
   /// Gets the match settings (native-friendly version that returns a reference)
   UTATMatchSettingsBase& GetMatchSettings() const { check(_matchSettings); return *_matchSettings; }

   /// Gets the match settings.
   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT", DisplayName = "Get Match Settings")
   UTATMatchSettingsBase* BP_GetMatchSettings() const { return _matchSettings; }

   /// Deserialize new match settings data, replacing all current match settings with the new data.
   /// Optionally updates the replicated data in the game state if specified.
   void UpdateMatchSettings(const TArray<uint8>& newMatchSettingsData, bool autoUpdateGameState = true);

   /// Call this after modifying match settings during a match. This will serialize the new settings and replicate the change to all clients.
   /// Note: This is intended for use by cheats. For other use-cases, updating match settings during a match is NOT supported.
   void AuthorityNotifyCheatUpdatedMatchSettings();

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchSettingsUpdated, UTATMatchSettingsBase*, NewMatchSettings);

   UPROPERTY(BlueprintAssignable)
   FOnMatchSettingsUpdated OnMatchSettingsUpdated;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapNodeSettingsUpdated, const FTATMapNodeSettings&, NewMapNodeSettings);

   UPROPERTY(BlueprintAssignable)
   FOnMapNodeSettingsUpdated OnMapNodeSettingsUpdated;

   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT|Map Node Settings")
   const FTATMapNodeSettings& GetMapNodeSettings() const { return _mapNodeSettings; }
   UFUNCTION(BlueprintCallable, Category = "Game Instance|TAT|Map Node Settings")
   void SetMapNodeSettings(const FTATMapNodeSettings& newMapNodeSettings);

   /// Gets the maximum number of players for an online session
   /// TODO: We will want to refine this later by taking the game mode/game session parameters into account
   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT")
   int32 GetMaxNumPlayersForSession() const;

   // did we run into a network failure?
   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT")
   bool ShouldShowNetworkFailure() const { return _showNetworkFailure; }
   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT")
   ENetworkFailure::Type GetNetworkFailure() const { return _networkFailureType; }

   // did we run into a travel failure?
   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT")
   bool ShouldShowTravelFailure() const { return _showTravelFailure; }
   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT")
   ETravelFailure::Type GetTravelFailure() const { return _travelFailureType; }

   // consume our network failures on the main menu then call this to reset state
   UFUNCTION(BlueprintCallable, Category = "Game Instance|TAT")
   void SetNetworkFailuresConsumed();

   /// Clears the current party
   void ClearParty();

   /// Adds a player to the current party
   void AddPlayerToParty(const FUniqueNetIdRepl& playerId);

   // authority-only
   // Whether the game was matchmade (rather than solo, or coop with friends)
   void SetPartyWasMatchmade(bool wasMatchmade) { _wasMatchmade = wasMatchmade; }
   bool WasPartyMatchmade() const { return _wasMatchmade; }

   /// Gets the number of players in the current party
   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT")
   int32 GetPartySize() const;

   /// Checks if we have a party of players at all
   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT")
   inline bool HasPartyMembers() const { return _partyMembers.Num() > 0; }

   /// Checks if a given player is in the current party
   UFUNCTION(BlueprintPure, Category = "Game Instance|TAT")
   bool IsPlayerInParty(const FUniqueNetIdRepl& playerId) const;

   const TArray<FUniqueNetIdRepl>& GetPartyMembers() const { return _partyMembers; }
   int32 TryPredictPlayerIdForPartyMember(const FUniqueNetIdRepl& uniqueId) const;

   void ClearContractSelections();
   void SetContractSelections(FTATPartyContracts questSelections);
   const FTATPartyContracts& GetContractSelections() const;

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game Instance|TAT")
   void ReturnToThievesDen(const FText& returnReason);
   virtual void ReturnToThievesDen_Implementation(const FText& returnReason);
   
   UFUNCTION(BlueprintCallable, Category = "Game Instance|TAT")
   bool JoinPendingSession();
   virtual bool JoinSession(ULocalPlayer* localPlayer, const FOnlineSessionSearchResult& searchResult) override;
protected:
   APlayerController* _GetPlayerControllerForControllerId(const int32 controllerId) const; 
   APlayerController* _GetPlayerControllerForLocalIndex(const int32 localPlayerIndex) const;
	void _OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 controllerId, FUniqueNetIdPtr /*UserId*/, const FOnlineSessionSearchResult& searchResult);
   void _OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result);
   bool _JoinPendingSession();
   bool _TravelToSession();
   void _TravelResetParameters();
   
   int32 _travelNextPlayerIdx = INDEX_NONE;
   FName _travelNextSessionName = NAME_None;
   FOnlineSessionSearchResult _travelSearchResult;
   bool _travelReadyToTravel = false;

protected:
   UFUNCTION(BlueprintImplementableEvent, Category = "Game Instance|TAT")
   void OnMapLoadComplete();

   UFUNCTION(BlueprintImplementableEvent, Category = "Game Instance|TAT")
   void OnPreClientTravel(bool bIsSeamless);

   UFUNCTION(BlueprintCallable, meta = (BlueprintProtected), Category = "Game Instance|TAT")
   void _SetNetworkFailure(ENetworkFailure::Type failureType);
   UFUNCTION(BlueprintCallable, meta = (BlueprintProtected), Category = "Game Instance|TAT")
   void _SetTravelFailure(ETravelFailure::Type failureType);

private:
   // init
   void _InitNetworkVersionOverride();

   // core delegates
   void _OnPostLoadMap(UWorld* world);
   void _HandlePreClientTravel(const FString& pendingURL, ETravelType travelType, bool isSeamlessTravel);

   void _InstallMetricsHandlers();

private:
   double _tickDelegateLastCallTime = 0.0;
   FTSTicker::FDelegateHandle _tickDelegateHandle;

   UPROPERTY()
   UTATTravelMgr* _travelMgr;
   UPROPERTY()
   UTATAnalyticsMgr* _analyticsMgr;
   UPROPERTY(Transient)
   TObjectPtr<UTATMatchSettingsBase> _matchSettings;
   FTATMapNodeSettings _mapNodeSettings;

   // very global engine-level network failures
   ENetworkFailure::Type _networkFailureType;
   bool _showNetworkFailure = false;
   ETravelFailure::Type _travelFailureType;
   bool _showTravelFailure = false;

   // network version
   int _netVersion = 0;

   // Players currently playing in a group that persists during server travel
   UPROPERTY(Transient)
   TArray<FUniqueNetIdRepl> _partyMembers;
   bool _wasMatchmade = false;

   FTATPartyContracts _contractSelections;

   TSharedPtr<FOSEMetricsSystem> _metricsSystem;
   TSharedPtr<FOSEGameSessionMetrics> _gameSessionMetrics;
};
