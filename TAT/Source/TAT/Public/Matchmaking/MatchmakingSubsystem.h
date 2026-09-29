// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Subsystems/GameInstanceSubsystem.h"

#include "MatchmakingSubsystem.generated.h"

UENUM(BlueprintType)
enum class EMatchmakingState : uint8
{
   None,
   StartingMatchmaking,
   SearchingForMatch,
   PlacingMatchOnServer,
   WaitingForMatchmakingTicketResponse,
   AcceptingMatchmaking,
   TravelToServer,
};

UENUM(BlueprintType)
enum class EMatchmakingErrorState : uint8
{
   None,
   FailedToStartMatchmaking,
   FailedToDescribeMatchmaking,
   MatchmakingTimeout,
   FailedToAcceptMatch,
};

struct FMatchmakingConnectionInfo
{
   // matchmaking
   FString MatchmakingTicketId;

   // server info once we've found a match
   FString GameSessionId;
   FString PlayerSessionId;
   FString DNSName;
   FString IPAddress;
   int Port = 0;
   bool MatchAccepted = false;
};

UCLASS(BlueprintType)
class TAT_API UTATMatchmakingSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingStateChanged, EMatchmakingState, state);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingErrorStateChanged, EMatchmakingErrorState, errorState);

public:
   UTATMatchmakingSubsystem();

   // from ULocalPlayerSubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

   UFUNCTION(BlueprintCallable, Category = "Matchmaking|TAT")
   bool StartMatchmaking();
   UFUNCTION(BlueprintCallable, Category = "Matchmaking|TAT")
   bool RespondToMatchmakingTicket(bool accept);
   UFUNCTION(BlueprintCallable, Category = "Matchmaking|TAT")
   bool StopMatchmaking();

   UFUNCTION(BlueprintPure, Category = "Matchmaking|TAT")
   EMatchmakingState GetMatchmakingState() const { return _matchmakingState; }
   UFUNCTION(BlueprintPure, Category = "Matchmaking|TAT")
   EMatchmakingErrorState GetErrorState() const { return _matchmakingErrorState; }

   UPROPERTY(BlueprintAssignable, Category = "Matchmaking|TAT")
   FOnMatchmakingStateChanged OnMatchmakingStateChanged;

   UPROPERTY(BlueprintAssignable, Category = "Matchmaking|TAT")
   FOnMatchmakingErrorStateChanged OnMatchmakingErrorStateChanged;

   // from FTickableObjectBase
   virtual bool IsTickable() const { return !HasAnyFlags(RF_ClassDefaultObject); }
   virtual TStatId GetStatId() const override { return Super::GetStatID(); }
   virtual void Tick(float deltaTime) override;

private:
   void _SetState(EMatchmakingState state);
   void _SetErrorState(EMatchmakingErrorState errorState);
   void _JoinSessionFromMatchmakingTicket();
   void _ResetMatchmaking();
   FString _GetUniqueNetIdStr() const;

private:
   FMatchmakingConnectionInfo _matchmakingConnectionInfo;
   EMatchmakingState _matchmakingState = EMatchmakingState::None;
   EMatchmakingErrorState _matchmakingErrorState = EMatchmakingErrorState::None;
   float _describeMatchmakingSecondsRemaining = 0.0f;
   bool _isDescribingMatchmaking = false;
};

DECLARE_LOG_CATEGORY_EXTERN(LogTATMatchmaking, Log, All);
