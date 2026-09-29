// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATLobbySubsystem.generated.h"

class UTATMatchSettingsBase;
enum class ETATDifficulty : uint8;
enum class ETATMatchMode : uint8;

// A world subsystem that is a facade for data shown in the lobby
//
// Initially just showing contract, but more could be moved in later
// e.g. Map (tracked but not exposed), mission, proxies for players, ...
//
//
// This could come from pragma or not-pragma, and those have separate implementations.
//
// NOTE: Not super married to this structure. Internally creating the source objects and then proxy-ing
//       them in the subsystem means that it is easy to change that structure without impacting anyone
//       external.
UCLASS()
class TAT_API UTATLobbySubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;
   virtual void OnWorldBeginPlay(UWorld& inWorld) override;

   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChanged);

   UFUNCTION(BlueprintPure, Category = "TAT|Lobby")
   const FGameplayTag& GetContract() const;
   UPROPERTY(BlueprintAssignable, Category = "TAT|Lobby")
   FOnChanged OnContractChanged;

   UFUNCTION(BlueprintPure, Category = "TAT|Lobby")
   const FGameplayTag& GetMission() const;
   UPROPERTY(BlueprintAssignable, Category = "TAT|Lobby")
   FOnChanged OnMissionChanged;

   UFUNCTION(BlueprintCallable, Category = "TAT|Lobby")
   bool GetMatchMode(ETATMatchMode& found) const;
   UPROPERTY(BlueprintAssignable, Category = "TAT|Lobby")
   FOnChanged OnMatchModeChanged;

   UFUNCTION(BlueprintCallable, Category = "TAT|Lobby")
   bool GetDifficulty(ETATDifficulty& found) const;
   UPROPERTY(BlueprintAssignable, Category = "TAT|Lobby")
   FOnChanged OnDifficultyChanged;


protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

private:
   UPROPERTY(Transient)
   TObjectPtr<UTATLobbySource> _source;
};


// The base class that does the work to fetch the information
//
// It currently also has copies of values for convenience,
// It could have split the data-holder into a separate object,
// but this was simpler
UCLASS(Abstract)
class UTATLobbySource : public UObject
{
   GENERATED_BODY()

public:
   virtual void OnWorldBeginPlay();
   virtual void Cleanup();
   

   const FGameplayTag& GetContract() const { return _contract; }
   FSimpleMulticastDelegate OnContractChanged;

   const FGameplayTag& GetMission() const { return _mission; }
   FSimpleMulticastDelegate OnMissionChanged;

   const TSoftObjectPtr<UWorld>& GetMap() const { return _map; }
   FSimpleMulticastDelegate OnMapChanged;

   TOptional<ETATMatchMode> GetMatchMode() const { return _matchMode; }
   FSimpleMulticastDelegate OnMatchModeChanged;

   TOptional<ETATDifficulty> GetDifficulty() const { return _difficulty; }
   FSimpleMulticastDelegate OnDifficultyChanged;
protected:
   void _SetMatchMode(ETATMatchMode matchMode);
   void _SetDifficulty(ETATDifficulty difficulty);
   void _SetContract(const FGameplayTag& newContract);
   void _SetMission(const FGameplayTag& newMission);
   void _SetMap(const TSoftObjectPtr<UWorld>& newMap);
   virtual void _OnMapChanged() {}
   UFUNCTION()
   virtual void _RefreshMap();
   
private:
   TOptional<ETATMatchMode> _matchMode;
   TOptional<ETATDifficulty> _difficulty;
   FGameplayTag _contract;
   FGameplayTag _mission;
   TSoftObjectPtr<UWorld> _map;
};

// The source for non-pragma lobbies
//
// Likely offline in practice, but still supporting the unreal-replicated flow for now
UCLASS()
class UTATReplicatedLobbySource : public UTATLobbySource
{
   GENERATED_BODY()

public:
   virtual void OnWorldBeginPlay() override;
   virtual void Cleanup() override;

private:
   void _OnGameStateSet(AGameStateBase* gameState);
   UFUNCTION()
   void _OnPlayerStateAdded(AOSEPlayerState* playerState);
   UFUNCTION()
   void _OnPlayerStateRemoved(AOSEPlayerState* playerState);
   UFUNCTION()
   void _OnPlayerContractChanged(FGameplayTag contractTag);

   void _RefreshContracts();
   virtual void _OnMapChanged() override;
   virtual void _RefreshMap() override;

   void _BindToPlayer(const AOSEPlayerState* playerState);
};
