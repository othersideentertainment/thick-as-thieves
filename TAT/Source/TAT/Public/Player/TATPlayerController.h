// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Character/TATCharacterMetadata.h"
#include "GameFramework/TATCheatManager.h"
#include "GameFramework/TATMatchPersistentTypes.h"
#include "Indicators/TATClientProxyInfo.h"
#include "Interactables/TATEscapePoint.h"

// ose
#include "AI/Perception/TATHearingStimSourceReactor.h"

#include "Damage/TATLocalDamageTrackerComponent.h"

#include "Player/OSEPlayerController.h"

#include "TATPlayerController.generated.h"

class ATATRespawnAreaOverlapVolume;
class AGameStateBase;
class ATATHUD;
class ATATPlayerState;
class UTATScreenMgr;
class UCharacterMovementComponent;
class UInputAction;
class UTATEnhancedInputComponent;
class UTATReticleStateComponent;
class UTATPlayerPostProcStackComponent;
class UTATPlayerPerceptionManagerComponent;
struct FTATAINoiseEvent;

UCLASS()
class TAT_API ATATPlayerController : public AOSEPlayerController, public ITATHearingStimSourceReactor
{
   GENERATED_BODY()
   
public:
   ATATPlayerController(const FObjectInitializer& objectInitializer);

   // From AActor
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // statics
   UFUNCTION(BlueprintPure, Category = "Player Controller|TAT", meta = (WorldContext = "contextObj"))
   static ATATPlayerController* GetTATPlayerController(const UObject* contextObj, int index);
   UFUNCTION(BlueprintPure, Category = "Player Controller|TAT", meta = (WorldContext = "contextObj"))
   static ATATPlayerController* GetLocalTATPlayerController(const UObject* contextObj);

   UFUNCTION(BlueprintPure)
   ATATHUD* GetTATHUD() const;
   UFUNCTION(BlueprintPure)
   UTATScreenMgr* GetTATScreenMgr() const;
   UFUNCTION(BlueprintPure)
   ATATPlayerState* GetTATPlayerState() const;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTATScreenMgrSpawned, UTATScreenMgr*, screenMgr);
   UPROPERTY(BlueprintAssignable, Category = "TAT Screen")
   FOnTATScreenMgrSpawned OnTATScreenMgrSpawned;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnteredTransitionMap);
   UPROPERTY(BlueprintAssignable)
   FOnEnteredTransitionMap OnEnteredTransitionMap;

   UPROPERTY(EditDefaultsOnly)
   float SecondsToEnterFirstPersonCamera = 3.0f;

   UFUNCTION(Reliable, Client)
   void ClientAboutToChangeMaps();

   UFUNCTION(Reliable, Client)
   void ClientStashMatchData(const FMatchPersistentData& data);

   UFUNCTION(Reliable, Client)
   void ClientNotifyAnotherPlayerEscaped(const FString& playerName, ATATEscapePoint* escapeRoute, int lootValue);
   
   UFUNCTION(Reliable, Client)
   void ClientNotifyLeaderboardData(const TArray<FMatchPersistentRankingPlayerData>& rankingData);

   UFUNCTION(Reliable, Client)
   void ClientFinishEscaping();

   void OnNetCleanup(UNetConnection* connection) override;

   /// Return the client to the Thieves' Den gracefully
   UFUNCTION(Reliable, Client)
   virtual void ClientReturnToThievesDen(const FText& returnReason);

   /// Setter for Pawn. Normally should only be used internally when possessing/unpossessing a Pawn.
   virtual void SetPawn(APawn* inPawn) override;

   // an approximate locally-computed initial spawn location for analytics purposes
   const FVector& GetApproximateInitialSpawnLocation() const { return _approximateInitialSpawnLocation; }

   bool GetIsPlayerStandingStill() const { return _isStandingStill; }

   virtual bool IsCharacterReady() const;

   void HandleLocalPlayerGeneratedNoiseStim(const FTATAINoiseEvent& stim);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalPlayerGeneratedStim, const FTATAINoiseEvent&, stim);
   UPROPERTY(BlueprintAssignable)
   FOnLocalPlayerGeneratedStim OnLocalPlayerGeneratedNoiseStim;

   UFUNCTION(BlueprintCallable, Category="TAT|Spectating")
   void BeginTempSpectating();

   UFUNCTION(Server, Reliable)
   void ServerBeginTempSpectating();

   UFUNCTION(BlueprintCallable, Category="TAT|Spectating")
   void EndTempSpectating();

   UFUNCTION(Server, Reliable)
   void ServerEndTempSpectating();

   void AuthoritySetRespawnPoint(AActor* newRespawnPoint);

   void AuthorityAddRespawnArea(ATATRespawnAreaOverlapVolume* respawnAreaOverlapVolume);
   void AuthorityRemoveRespawnArea(ATATRespawnAreaOverlapVolume* respawnAreaOverlapVolume);
   void AuthorityClearRespawnAreas();
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIDetectedInCurrentBestRespawnAreaChanged, bool, detected);
   UPROPERTY(BlueprintAssignable, Category="TAT|Events")
   FOnAIDetectedInCurrentBestRespawnAreaChanged OnAIDetectedInCurrentBestRespawnAreaChanged;
   
   UFUNCTION(BlueprintCallable, Category="TAT|Respawn")
   AActor* GetRespawnPoint() const;
   
   UFUNCTION(BlueprintPure)
   bool IsAIDetectedWithinRespawnZone() const { return _aiDetectedInCurrentBestRespawnArea; }   

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpectatingStateChanged, bool, isSpectating);
   UPROPERTY(BlueprintAssignable, Category="TAT|Events")
   FOnSpectatingStateChanged OnSpectatingStateChanged;

   /// Applies a multiplier value to the player's look-speed. Tag identifier allows for ClearLookSpeedMultiplier() to remove multiplier.
   /// Multiple multiplier values are multiplied together, not added.
   UFUNCTION(BlueprintCallable, Category = "TAT|Input")
   void SetLookSpeedMultiplier(FName multiplierId, float mouseMultiplier, float gamepadMultiplier);

   /// Removes a previously-set multiplier value.
   UFUNCTION(BlueprintCallable, Category = "TAT|Input")
   void ClearLookSpeedMultiplier(FName multiplierId);

   // begin ITATHearingStimSourceReactor
   virtual void HandleReactToOwnStim(const FGameplayTag& stimTag, float loudness) override;
   // end ITATHearingStimSourceReactor

   UFUNCTION(Client, Reliable)
   void ClientJoinOnlineSession(const FString& sessionId);

protected:

   UFUNCTION(BlueprintImplementableEvent, Category="TAT")
   void BP_OnReceivedMatchPersistentData();

   UFUNCTION()
   void _HandleInstantDamageTaken(FTATDamageWithType damage, FTATLocalDamageSource source);

   // from PlayerController
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void ClientSetHUD_Implementation(TSubclassOf<AHUD> newHUDClass) override;
   virtual void TickActor(float deltaTime, enum ELevelTick tickType, FActorTickFunction& thisTickFunction) override;
   virtual void SetPlayer(UPlayer* inPlayer) override;
   virtual void SetupInputComponent() override;
   virtual void GetSeamlessTravelActorList(bool toEntry, TArray<class AActor*>& actorList) override;
   virtual void NotifyLoadedWorld(FName worldPackageName, bool finalDest) override;
   virtual void SeamlessTravelFrom(APlayerController* oldPC) override;
   virtual void SeamlessTravelTo(APlayerController* newPC) override;
   virtual bool InputKey(const FInputKeyParams& params) override;
   virtual bool IsMoveInputIgnored() const override;
   virtual bool IsLookInputIgnored() const override;
   virtual void UpdateHiddenComponents(const FVector& viewLocation, TSet<FPrimitiveComponentId>& hiddenComponents) override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   UTATReticleStateComponent* ReticleState = nullptr;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   UTATPlayerPostProcStackComponent* PlayerPostProcStack = nullptr;

   UPROPERTY(EditDefaultsOnly)
   UTATPlayerPerceptionManagerComponent* PlayerPerceptionComponent = nullptr;

   UFUNCTION(BlueprintImplementableEvent)
   void OnAnotherPlayerEscaped(const FString& playerName, ATATEscapePoint* escapeRoute, int lootValue);

   // from APlayerController
   virtual void BeginSpectatingState() override;
   virtual void EndSpectatingState() override;

   UFUNCTION()
   void _AuthorityAIDetectedWithinRespawnSafeZone(ATATRespawnAreaOverlapVolume* volume, bool bDetected);   
private:

#if OSE_CHEATS_ENABLED

   void _TATCheat_ChangeCharacter(ETATCharacter character);
   void _TATCheat_ChangeCharacter_1() { _TATCheat_ChangeCharacter(ETATCharacter::Character0); }
   void _TATCheat_ChangeCharacter_2() { _TATCheat_ChangeCharacter(ETATCharacter::Character1); }
   void _TATCheat_ChangeCharacter_3() { _TATCheat_ChangeCharacter(ETATCharacter::Character2); }

   void _TATCheat_CameraMode_Toggle(const EPlayerCameraMode newMode);
   void _TATCheat_CameraMode_Toggle1P();
   void _TATCheat_CameraMode_Toggle3P();

#endif //OSE_CHEATS_ENABLED

   UFUNCTION()
   void _OnEnterFirstPersonCameraTimeout();

   void _TrySpawnScreenMgr();

   void _TickLocalSubsystemState();

   void _TickPlayerMovementState(const UCharacterMovementComponent* characterMovementComponent);

   UFUNCTION()
   void _OnRep_ThiefVisionIndicators();

   UFUNCTION()
   void _OnRep_AIDetectedInCurrentBestRespawnArea();
   
   void _HandleAIDetectedInCurrentBestRespawnAreaChanged();
   
   void _OnThiefVisionIndicatorsUpdated();

   UTATEnhancedInputComponent* _GetEnhancedInputComponent() const;

   UFUNCTION()
   void _OnRep_RespawnPoint();
   UFUNCTION()
   void _OnRep_RespawnAreas();

   void _SpawnRespawnMarker();
   void _RefreshRespawnMarkerLocation();
   FVector _CalculateRespawnMarkerLocation();

private:
   bool _thiefVisionEnabled = false;

   UPROPERTY(Replicated, ReplicatedUsing="_OnRep_RespawnPoint", Transient)
   TObjectPtr<AActor> _respawnPointActor;
   
   UPROPERTY(Replicated, ReplicatedUsing="_OnRep_RespawnAreas", Transient)
   TArray<TObjectPtr<ATATRespawnAreaOverlapVolume>> _currentRespawnAreaOverlapVolumes;

   UPROPERTY(Replicated, ReplicatedUsing=_OnRep_AIDetectedInCurrentBestRespawnArea, Transient)
   bool _aiDetectedInCurrentBestRespawnArea { false };
      
   UPROPERTY(Replicated, ReplicatedUsing = _OnRep_ThiefVisionIndicators)
   FTATClientProxyInfoArray _thiefVisionIndicators;

   UPROPERTY()
   UTATScreenMgr* _screenMgr;

   // an approximate locally-computed initial spawn location for analytics purposes
   // A client may get a slightly later version of this position, but some noise is acceptable
   FVector _approximateInitialSpawnLocation;

   // Allowed movement speed within which the character will still be considered as stationary
   UPROPERTY(EditDefaultsOnly, Category = "Player Controller|TAT|Movement Detection", meta = (UIMin = 0.001))
   float _standingStillMovementEpsilon = 1.0f;

   // Amount of time required for the player to be stopped before they will be considered stationary
   UPROPERTY(EditDefaultsOnly, Category = "Player Controller|TAT|Movement Detection", meta = (UIMin = 0.001))
   float _standingStillRequiredDuration = 0.5f;

   UPROPERTY(EditDefaultsOnly, Category = "Player Controller|TAT|Movement Detection")
   FGameplayTag _standingStillTag;

   // Time at which the player last moved significantly
   float _mostRecentMovedTime = 0.f;

   bool _isStandingStill = false;

   // pawn cached when spectating, returned to after spectating
   UPROPERTY()
   TObjectPtr<APawn> _pawnCachedWhileSpectating;

   // Actor class positioned at the location where the player will likely respawn
   // should not be replicated
   UPROPERTY(EditDefaultsOnly, Category = "Player Controller|TAT")
   TSoftClassPtr<AActor> _respawnPointMarkerClass;
   
   UPROPERTY(EditDefaultsOnly, Category = "Player Controller|TAT|Damage")
   UTATLocalDamageTrackerComponent* _DamageTracker { nullptr };
   
   UPROPERTY(Transient)
   TObjectPtr<AActor> _respawnPointMarker;
};
