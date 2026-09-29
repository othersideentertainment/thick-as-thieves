// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "GameFramework/OSEGameStateAwareActor.h"

// ue
#include "GameplayTagContainer.h"

#include "DemoHubMissionMgr.generated.h"

class AOSEPlayerState;
class ATATMatchLobbyDummyPlayer;
class UTATMissionAsset;

enum class ETATDifficulty : uint8;

USTRUCT()
struct FTATMapChangeData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<UWorld> SelectedMap;
   
   UPROPERTY(EditAnywhere, Category = DefaultModes, meta = (MetaClass = "/Script/Engine.GameModeBase"))
   FSoftClassPath GameMode;

   UPROPERTY()
   TOptional<ETATDifficulty> Difficulty;

   UPROPERTY()
   FGameplayTag MissionTag;
};

UCLASS(Blueprintable, BlueprintType)
class TAT_API ATATDemoHubMissionMgr : public AOSEGameStateAwareActor
{
   GENERATED_BODY()

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif // WITH_EDITOR
   
public:   
   ATATDemoHubMissionMgr();

   UFUNCTION(BlueprintPure, Meta = (WorldContext = "contextObject"))
   static ATATDemoHubMissionMgr* GetDemoHubMissionManager(const UObject* contextObject);

   UFUNCTION(Exec, BlueprintCallable, Category="TAT")
   void LeaveLobby();

   // from AActor
   virtual void BeginPlay() override;
   virtual void Tick(float deltaSeconds) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

public:
   UPROPERTY(EditDefaultsOnly)
   float SecondsToTravel = 6.0f;

   // Amount of time before matchmaking should timeout and move player to Solo mode
   UPROPERTY(EditDefaultsOnly)
   float MatchmakingTimeoutDuration = 60.0f;

   // Amount of time after the matchmaking timeout the client should wait before moving the player to Solo mode
   // This is to avoid any late messages to say we have joined a match.
   UPROPERTY(EditDefaultsOnly)
   float MatchmakingTimeoutCooldownDuration = 2.0f;

   UFUNCTION(BlueprintPure)
   float GetTravelWorldTime() const { return _authorityTravelWorldTime; }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTravelWorldTimeChanged, float, serverTravelWorldTime);
   UPROPERTY(BlueprintAssignable)
   FOnTravelWorldTimeChanged OnTravelWorldTimeChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingStatusChanged, bool, bInMatchmaking);
   UPROPERTY(BlueprintAssignable)
   FOnMatchmakingStatusChanged OnMatchmakingStatusChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchmakingTimeout);
   UPROPERTY(BlueprintAssignable)
   FOnMatchmakingTimeout OnMatchmakingTimeout;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRefreshDummyAssignments);
   UPROPERTY(BlueprintAssignable)
   FOnRefreshDummyAssignments OnRefreshDummyAssignments;
   
   UFUNCTION(BlueprintPure)
   TSoftObjectPtr<UWorld> GetSelectedMap() const
   {
      return _mapChangeData.SelectedMap;
   }

   UFUNCTION(BlueprintPure)
   FSoftClassPath GetSelectedMapGameMode() const
   {
      return _mapChangeData.GameMode;
   }

   // Only valid non-pragma
   const FGameplayTag& GetMissionTag() const
   {
      return _mapChangeData.MissionTag;
   }

   TOptional<ETATDifficulty> GetDifficulty() const
   {
      return _mapChangeData.Difficulty;
   }
   
   void AuthoritySetSelectedMap(const TSoftObjectPtr<UWorld>& newMap, const FSoftClassPath& gameMode);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAuthoritySelectedMapChanged);
   UPROPERTY(BlueprintAssignable)
   FOnAuthoritySelectedMapChanged OnSelectedMapChanged;

protected:
   void _OnAuthorityTravelToMap();
private:
   UFUNCTION()
   void _OnPlayerStateAdded(AOSEPlayerState* playerState);
   
   UFUNCTION()
   void _OnPlayerStateRemoved(AOSEPlayerState* playerState);

   UFUNCTION()
   void _OnPlayerStateChanged(AOSEPlayerState* playerState);

   UFUNCTION() 
   void _OnRep_MapChangeData();

   UFUNCTION()
   void _OnRep_ServerTravelWorldTime();
   void _UpdateHUDMissionCountdown();

   void _AuthorityUpdateParty();

   void _RefreshPlayerDummyAssignments();
   ATATMatchLobbyDummyPlayer* _FindUnclaimedDummyForRemotePlayer() const;

   UFUNCTION()
   void _OnTimeoutCooldownComplete();

   void _ClearMatchmakingTimeout(bool clearCooldown = false);

private:
#if WITH_EDITORONLY_DATA
   UPROPERTY(EditInstanceOnly, Category = "TAT|Player Dummies")
   bool _validatePlayerDummyInstances = false;
#endif // WITH_EDITORONLY_DATA

   UPROPERTY(ReplicatedUsing = _OnRep_ServerTravelWorldTime)
   float _authorityTravelWorldTime = float(INDEX_NONE);

   UPROPERTY(ReplicatedUsing = _OnRep_MapChangeData)
   FTATMapChangeData _mapChangeData;
   
   // authority state
   bool _authorityTravelStarted = false;

   UPROPERTY(EditInstanceOnly, Category = "TAT|Player Dummies")
   TObjectPtr<ATATMatchLobbyDummyPlayer> _localPlayerDummy;

   UPROPERTY(EditInstanceOnly, Category = "TAT|Player Dummies")
   TArray<TObjectPtr<ATATMatchLobbyDummyPlayer>> _remotePlayerDummies;

   FTimerHandle _matchmakingTimeoutHandle;
   FTimerHandle _matchmakingCooldownHandle;
   
};
