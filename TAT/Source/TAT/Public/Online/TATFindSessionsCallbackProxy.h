// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "Interfaces/OnlineSessionDelegates.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "OnlineSessionSettings.h"

#include "TATFindSessionsCallbackProxy.generated.h"

class APlayerController;
enum class ETATDifficulty : uint8;

USTRUCT(BlueprintType)
struct FTATSessionSearchQuery
{
   GENERATED_BODY()

   /// Max number of sessions to retrieve
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   int32 MaxResults = 20;

   /// Amount of time to wait in seconds before the search times out. Not supported on all platforms.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   float TimeoutInSeconds = 0.0f;

   /// Search only on the local area network
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   bool SearchLANOnly = false;

   /// Search only for dedicated servers
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   bool SearchDedicatedOnly = false; // SEARCH_DEDICATED_ONLY

   /// Search for empty servers only
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   bool SearchEmptyServersOnly = false; // SEARCH_EMPTY_SERVERS_ONLY

   /// Search for non empty servers only
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   bool SearchNonEmptyServersOnly = false; // SEARCH_NONEMPTY_SERVERS_ONLY

   // /// Search for secure servers only
   // UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   // bool SEARCH_SECURE_SERVERS_ONLY;

   /// Search for presence sessions only
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   bool SearchPresence = true; // SEARCH_PRESENCE

   /// Search for a match with min player availability
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   int32 SearchMinSlotsAvailable = 0; // SEARCH_MINSLOTSAVAILABLE

   // /// Exclude all matches where any unique ids in a given array are present (value is string of the form "uniqueid1;uniqueid2;uniqueid3")
   // UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   // FString SEARCH_EXCLUDE_UNIQUEIDS;

   // /// User ID to search for session of
   // UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   // bool SEARCH_USER;

   /// Keywords to match in session search
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   FString SearchKeywords; // SEARCH_KEYWORDS

   // /// The matchmaking queue name to matchmake in, e.g. "TeamDeathmatch"
   // UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   // FString SEARCH_MATCHMAKING_QUEUE;

   // /// If set, use the named Xbox Live hopper to find a session via matchmaking
   // UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   // FString SEARCH_XBOX_LIVE_HOPPER_NAME;

   // /// Which session template from the service configuration to use
   // UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   // FString SEARCH_XBOX_LIVE_SESSION_TEMPLATE_NAME;

   // /// Selection method used to determine which match to join when multiple are returned (valid only on Switch)
   // UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   // FString SEARCH_SWITCH_SELECTION_METHOD;

   /// Whether to use lobbies vs sessions
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session Query")
   bool SearchLobbies = true; // SEARCH_LOBBIES
};

/// This is very similar to UFindSessionsCallbackProxy, but allows customizing the session query
UCLASS(MinimalAPI)
class UTATFindSessionsCallbackProxy : public UOnlineBlueprintCallProxyBase
{
   GENERATED_BODY()

   UTATFindSessionsCallbackProxy();

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFindSessionsResultDelegate, const TArray<FBlueprintSessionResult>&, results);

   // Called when there is a successful query
   UPROPERTY(BlueprintAssignable)
   FFindSessionsResultDelegate OnSuccess;

   // Called when there is an unsuccessful query
   UPROPERTY(BlueprintAssignable)
   FFindSessionsResultDelegate OnFailure;

   // Searches for advertised sessions with the default online subsystem
   UFUNCTION(BlueprintCallable, DisplayName = "[TAT] Find Sessions", meta=(BlueprintInternalUseOnly = "true", WorldContext = "worldContext"), Category = "Online|Session")
   static UTATFindSessionsCallbackProxy* FindSessions(UObject* worldContext, APlayerController* playerController, const FTATSessionSearchQuery& sessionQuery);

   UFUNCTION(BlueprintPure, Category = "Online|Session")
   static bool GetSessionSettingStringValue(const FBlueprintSessionResult& result, FName settingName, FString& value);

   UFUNCTION(BlueprintPure, Category = "Online|Session")
   static bool GetSessionSettingIntValue(const FBlueprintSessionResult& result, FName settingName, int32& value);

   UFUNCTION(BlueprintPure, Category = "Online|Session")
   static bool GetSessionSettingMap(const FBlueprintSessionResult& result, FGameplayTag& mapTag);

   UFUNCTION(BlueprintPure, Category = "Online|Session")
   static bool GetSessionSettingDifficulty(const FBlueprintSessionResult& result, ETATDifficulty& difficulty);

   // UOnlineBlueprintCallProxyBase interface
   virtual void Activate() override;
   // End of UOnlineBlueprintCallProxyBase interface

private:
   /// Internal callback when the session search completes, calls out to the public success/failure callbacks
   void OnCompleted(bool success);

private:
   /// The player controller triggering things
   TWeakObjectPtr<APlayerController> _playerControllerWeak;

   /// The delegate executed by the online subsystem
   FOnFindSessionsCompleteDelegate _delegate;

   /// Handle to the registered OnFindSessionsComplete delegate
   FDelegateHandle _delegateHandle;

   /// Object to track search results
   TSharedPtr<FOnlineSessionSearch> _searchObject;

   /// Search parameters
   FTATSessionSearchQuery _sessionQuery;

   UPROPERTY(Transient)
   UObject* _worldContext = nullptr;
};
