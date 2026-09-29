// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Perception/TATResettableAIKnowledgeContainer.h"
   
// ose
#include "OSECommon.h"
#include "AI/Alertness/AlertnessEnums.h"
#include "AI/Alertness/DetectionEnums.h"
#include "AI/Perception/StimInfo.h"
#include "AI/OSEKnowledgeComponent.h"
#include "Character/OSETeamInterface.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"

#include "TATKnowledgeComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTATKnowledge, Log, All);

class ATATCharacterAIBase;
class ATATAIController;
class UAbilitySystemComponent;
class UOSEStimDatabase;
class USmartObjectComponent;
class UTATAIPerceptionComponent;
class UTATDetectionSettingsAsset;
class UTATKnowledgeComponent;

struct FStimDatabaseQuery;
struct FTATHearingEventStimSettings;
struct FGameplayTag;
struct FGameplayEventData;
struct FTATDetectionSettings;

///////////////////////////////////////////////////////////////////
///        EKnowledgeSource
///////////////////////////////////////////////////////////////////

/// Indicates where knowledge about a specific actor came from.
UENUM()
enum class EKnowledgeSource : uint8
{
   /// From perception system - sight/sound
   Sense,
   /// From being hit, triggering _OnDamageEvent.
   OnDamage,
   /// From SphereCast to find nearby allies.
   AllyScan,
   /// From SphereCast to find nearby enemies
   EnemyScan,
   /// From SphereCast to find nearby neutrals
   NeutralScan,
   /// From another AI, possibly asking for help
   Shared,
   /// Code-driven alert forcing an AI to know something they wouldn't have otherwise
   Alert
};

///////////////////////////////////////////////////////////////////
///        FVisibilityLog
///////////////////////////////////////////////////////////////////

USTRUCT()
struct FVisibilityLog
{
   GENERATED_BODY()

public:

   /// The final computed detection rate per second.
   UPROPERTY()
   float DetectionRate = 0.0f;

   /// The multiplier to detection rate based on distance.
   UPROPERTY()
   float DistanceMultiplier = 0.0f;

   /// Distance from viewer to actor.
   UPROPERTY()
   float Distance = 0.0f;

   /// World time these numbers were taken (not necessarily closely synched).
   UPROPERTY()
   float Timestamp = -1.0f;
};

///////////////////////////////////////////////////////////////////
///        FTATActorKnowledge
///////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct TAT_API FTATActorKnowledge
{
   GENERATED_BODY()

public:

   FTATActorKnowledge();

   void Init(UTATKnowledgeComponent* owner, TWeakObjectPtr<AActor> actor);
   void OnAboutToBeRemoved(ATATCharacterAIBase& ownerCharacter);

   void TickVisibility(float deltaTime, ATATCharacterAIBase& ownerCharacter, bool isCurrentlySeen, float sightStrength);
   void TickDetection(float deltaTime, const FTATDetectionSettings& detectionSettings, ATATCharacterAIBase& ownerCharacter);
   
   void ResetDetectionState(ATATCharacterAIBase& ownerCharacter);
   void ResetVisibilityState(ATATCharacterAIBase& ownerCharacter);

   bool IsEnemy() const { return _attitude == EOSETeamAttitude::Hostile; }
   bool IsPlayer() const { return _isPlayer; }
   bool IsSupiciousAlly() const;

   AActor* GetActor() const { return _actor.Get(); }

   void SetKnowledgeSource(EKnowledgeSource knowledgeSource) { _knowledgeSource = knowledgeSource; }
   EKnowledgeSource GetKnowledgeSource() const { return _knowledgeSource; }
   
   /// Last Known Location apis
   void SetLastKnownStimLocation(const FVector& lastKnownStimLocation);
   void SetLastKnownVisibleLocation(const FVector& lastKnownVisibleLocation);
   void SetLastKnownStimLocationWithTimestamp(const FVector& lastKnownStimLocation, float timeStamp);
   void SetLastKnownVisibleLocationWithTimestamp(const FVector& lastKnownVisibleLocation, float timeStamp);
   const FVector& GetLastKnownLocation() const;
   const FVector& GetLastKnownStimLocation() const { return _lastKnownStimLocation; }
   const FVector& GetLastKnownVisibleLocation() const { return _lastKnownVisibleLocation; }

   ///  The last time we received a location from any stim source
   float GetLastStimTimestamp() const { return _lastStimTimestamp; }

   /// The last time we received a location via visibility
   float GetLastVisibilityTimestamp() const { return _lastVisibilityTimestamp; }

   /// Whether we have a valid path to the last known location of the target.
   void SetHasAnyPathToLastKnownLocation(bool hasPathToLastKnownLocation) { _hasAnyPathToLastKnownLocation = hasPathToLastKnownLocation; }
   bool GetHasAnyPathToLastKnownLocation(const float checkPathToLocationFrequencyInSeconds);
   bool GetHasAnyPathToLastKnownLocation() const { return _hasAnyPathToLastKnownLocation; }
   
   void SetHasFullPathToLastKnownLocation(bool hasFullPathToLastKnownLocation) { _hasFullPathToLastKnownLocation = hasFullPathToLastKnownLocation; }
   bool GetHasFullPathToLastKnownLocation(const float checkPathToLocationFrequencyInSeconds);
   bool GetHasFullPathToLastKnownLocation() const { return _hasFullPathToLastKnownLocation; }

   void SetPathLengthToLastKnownLocation(const float pathLength) { _pathLengthToLastKnownLocation = pathLength; }
   float GetPathLengthToLastKnownLocation(const float checkPathToLocationFrequencyInSeconds); 
   float GetPathLengthToLastKnownLocation() const { return _pathLengthToLastKnownLocation; }

   void UpdatePathToLastKnownLocation(float checkPathToLocationFrequencyInSeconds);

   /// Last timestamp that the path to target location was checked.
   void SetPathCheckedTimeStamp(float pathCheckedTimeStamp) { _pathCheckedTimeStamp = pathCheckedTimeStamp; }
   float GetPathCheckedTimeStamp() const { return _pathCheckedTimeStamp; }

   /// The actor's detection value.
   float GetDetectionValue() const;
   void SetDetection(float x);

   /// The actor's detection state
   EActorDetectionState GetDetectionState() const;

   /// Timestamp for entering our current detection level
   float GetDetectionStateChangedTimestamp() const { return _detectionStateChangedTimestamp; }

   /// Is decay for the identified state paused?
   void SetIdentifiedStateDecayPaused(bool paused);
   bool GetIdentifiedStateDecayPaused() const { return _identifiedStateDecayPaused; }

   /// In-game time when this actor last attempted to attack us, whether it hit or not.
   void SetLastAttackTimestamp(float lastAttackTimestamp) { _lastAttackTimestamp = lastAttackTimestamp; }
   float GetLastAttackTimestamp() const { return _lastAttackTimestamp; }

   /// In-game time when this actor last hit us with an attack.
   void SetLastHitTimestamp(float lastHitTimestamp) { _lastHitTimestamp = lastHitTimestamp; }
   float GetLastHitTimestamp() const { return _lastHitTimestamp; }

   /// Whether the actor has any "do not see" tag, which shuts down perception-level visibility for this actor
   bool GetHasAnyDoNotSeeTag() const { return _hasAnyDoNotSeeTag; }

   /// Whether the actor is both perceived and visible.
   bool GetIsVisible() const { return _isVisible; }

   /// Whether a visible actor is in the correct state
   bool GetIsVisibleActorInCorrectState() const { return _isVisibleActorInCorrectState; }   

   /// Get owning AI attitude tatards this actor
   void UpdateAttitudeToOwner();
   void SetAttitude(EOSETeamAttitude attitude) { _attitude = attitude; }
   EOSETeamAttitude GetAttitude() const { return _attitude; }

   /// Are we investigating this target?
   void SetIsInvestigatingActor(bool isInvestigating) { _isInvestigatingActor = isInvestigating; }
   bool GetIsInvestigatingActor() const { return _isInvestigatingActor; }

   /// What locations have we previously investigated for this actor?
   const TArray<FVector>& GetInvestigationLocations() const { return _investigationLocations; }
   void AddInvestigationLocation(const FVector& investigationLocation) { _investigationLocations.Add(investigationLocation); }
   void ClearInvestigationLocations() { _investigationLocations.Reset(); }

#if WITH_GAMEPLAY_DEBUGGER
   FVisibilityLog& GetVisibilityLog() const { return _visibilityLog; }
#endif

   bool IsNegativeConditionApplied() const { return _hasNegativeConditionApplied; }
   void SetNegativeConditionApplied(bool val) { _hasNegativeConditionApplied = val; }

private:
   void _TickVisibleActorInCorrectState();
   bool _HasAnyDoNotSeeTag() const;
   bool _HasTargetBlockDetectionTags() const;
   bool _IsCrouching() const;
   float _GetNowTimestamp() const;
   EActorDetectionState _CalculateDetectionState(EAlertnessLevel alertnessLevel) const;
   float _CalculateDeltaDetectionValue(float deltaTime, const FTATDetectionSettings& detectionSettings, const ATATCharacterAIBase& ownerCharacter) const;
   void _TrySendDetectionStateToRelevantCharacters(ATATCharacterAIBase& ownerCharacter);
   void _TrySendDetectionChangedUpdateToTargetActor(ATATCharacterAIBase& ownerCharacter, EActorDetectionState previousState) const;
   void _TrySendVisibilityStateToOwningCharacter(ATATCharacterAIBase& ownerCharacter, bool wasVisible, bool isVisible) const;
   void _ClearDetectionValueForOtherAlertnessLevel(EAlertnessLevel alertnessLevel);
   static EActorDetectionState _GetMinDetectionStateForAlertnessLevel(EAlertnessLevel alertnessLevel);
   bool _CanActorBeDetected(const AActor* actor, EAlertnessLevel alertnessLevel) const;

private:
   EKnowledgeSource _knowledgeSource = EKnowledgeSource::Sense;

   TWeakObjectPtr<AActor> _actor;

   TWeakObjectPtr<UTATKnowledgeComponent> _ownerComponent = nullptr;

   /// Whether the actor is controlled by a player.
   bool _isPlayer = false;

   /// Whether we have a valid path (either full or partial) to the last known location of the target.
   bool _hasAnyPathToLastKnownLocation = false;
   
   /// Whether we have a valid full path to the last known location of the target.
   bool _hasFullPathToLastKnownLocation = false;

   /// The path length to the last known location
   float _pathLengthToLastKnownLocation = 0;

   ///  Last timestamp that the path to target location was checked.
   float _pathCheckedTimeStamp = 0.0;

   /// In-game time we last saw the actor. Negative if we've never seen them.
   float _lastSeenTimestamp = -1.0f;

   /// The actor's detection values [0 = observing, > 0 && < 1 == identifying, 1 = identified]
   float _detectionValue;

   /// Timestamp for entering our current detection level
   float _detectionStateChangedTimestamp = 0.0f;

   /// Is decay for the identified state paused?
   bool _identifiedStateDecayPaused = false;

   /// In-game time when this actor last attempted to attack us, whether it hit or not.
   float _lastAttackTimestamp = -1.0f;

   /// In-game time when this actor last hit us with an attack.
   float _lastHitTimestamp = -1.0f;

   /// Whether the actor is both perceived and visible.
   bool _isVisible = false;

   /// Whether the actor is both perceived and visible.
   float _sightStrength = 0.0f;

   /// Get owning AI attitude tatards this actor
   EOSETeamAttitude _attitude = EOSETeamAttitude::Neutral;

   /// Updates if actor has any "do not see" tag, which shuts down perception-level visibility for this actor
   bool _hasAnyDoNotSeeTag = false;

   /// The last time we received a location from any stim source
   float _lastStimTimestamp = -1.0f;

   /// The last time we received a location via visibility. Negative if we've never seen them.
   float _lastVisibilityTimestamp = -1.0f;

   /// The actor's last known stim location
   FVector _lastKnownStimLocation = FAISystem::InvalidLocation;

   /// The location we last saw the actor at
   FVector _lastKnownVisibleLocation = FAISystem::InvalidLocation;

   /// Is investigating target
   bool _isInvestigatingActor = false;

   /// What locations have we previously investigated for this actor?
   TArray<FVector> _investigationLocations;

   /// Is visible actor in the correct state
   bool _isVisibleActorInCorrectState = false;

   // Is this actor currently afflicted by a negative condition
   bool _hasNegativeConditionApplied = false;

#if WITH_GAMEPLAY_DEBUGGER
   mutable FVisibilityLog _visibilityLog;
#endif
};

///////////////////////////////////////////////////////////////////
///        FTATLocationKnowledge
///////////////////////////////////////////////////////////////////

struct TAT_API FTATLocationKnowledge
{
public:
   void Init(const UTATKnowledgeComponent* owner, const UObject* contextObject, uint32 contextHash = 0);

   bool Matches(const UObject* contextObject, uint32 contextHash = 0) const;

   void SetLocation(FVector location);
   FVector GetLocation() const { return _location; }

   // Get/Set whether we have a valid full or partial path to the location.
   void SetHasAnyPathToLocation(bool hasPathToLocation) { _hasAnyPathToLocation = hasPathToLocation; }
   bool GetHasAnyPathToLocation(const float checkPathToLocationFrequencyInSeconds);

   // Get/Set Whether we have a valid full path to the location.
   void SetHasFullPathToLocation(bool hasFullPathToLocation) { _hasFullPathToLocation = hasFullPathToLocation; }
   bool GetHasFullPathToLocation(const float checkPathToLocationFrequencyInSeconds);

   // Get/Set the path length to the location.
   void SetPathLengthToLocation(const float pathLength) { _pathLengthToLocation = pathLength; }
   float GetPathLengthToLocation(const float checkPathToLocationFrequencyInSeconds);

   // Try to update the cached pathing data, depending on the polling frequency.
   void UpdatePathToLocation(float checkPathToLocationFrequencyInSeconds);

   // Get/Set the last timestamp that the path to location was checked.
   void SetPathCheckedTimeStamp(float pathCheckedTimeStamp) { _pathCheckedTimeStamp = pathCheckedTimeStamp; }
   float GetPathCheckedTimeStamp() const { return _pathCheckedTimeStamp; }

private:
   float _GetNowTimestamp() const;

   // Object that the location being checked is related to.
   TWeakObjectPtr<const UObject> _contextObject = nullptr;

   // An optional, additional identifier. Useful for targeting multiple locations
   // under a single object.
   uint32 _contextHash;

   TWeakObjectPtr<const UTATKnowledgeComponent> _ownerComponent = nullptr;

   FVector _location = FVector::ZeroVector;

   // Whether we have a valid path (either full or partial) to the location.
   bool _hasAnyPathToLocation = false;

   // Whether we have a valid full path to the location.
   bool _hasFullPathToLocation = false;

   // The path length to the location.
   float _pathLengthToLocation = 0.0f;

   //  Last timestamp that the path to location was checked.
   float _pathCheckedTimeStamp = 0.0f;
};

///////////////////////////////////////////////////////////////////
///        FTATSharedTarget
///////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct TAT_API FTATSharedTarget
{
   GENERATED_BODY()

public:
   enum class EState : uint8
   {
      // We have not yet cached any targets we intended to share.
      None,
      // We have a target and need to share it with someone.
      PartnerNeeded,
      // We have a target and have found a partner to share them with.
      PartnerFound,
      // We have shared this target without partner.
      SharedWithPartner
   };

   // A "shared target" can either be an actor, stim, or a location that an AI wishes to share
   // knowledge of with another AI. Two of the three may be null but at least one must be valid.
   TWeakObjectPtr<AActor> TargetActor = nullptr;
   int TargetStimId = INDEX_NONE;
   TOptional<FVector> TargetLocation;

   // Notifies the recipient as to how to proceed.
   FGameplayTag TypeTag;

   // Contextual information about the target.
   FGameplayTagContainer ContextTags;

   // The AI that generated this shared target.
   TWeakObjectPtr<UTATKnowledgeComponent> Instigator = nullptr;

   // The AI class type we wish to share this knowledge with.
   TSoftClassPtr<ATATCharacterAIBase> PartnerClass = nullptr;

   // The AI that this shared target was selected to be given to.
   TWeakObjectPtr<UTATKnowledgeComponent> Partner = nullptr;

   // If true, when searching for a partner to share this target with it is OK to use magical knowledge to find them.
   // Typically will be true for the first partner search. If we fail to find a partner, will be disabled.
   bool CanPartnerSearchUseMagicKnowledge = true;

   EState State = EState::None;

   FORCEINLINE bool IsValid() const { return TargetActor.IsValid() || TargetStimId != INDEX_NONE || TargetLocation.IsSet(); }

   void Reset();

   bool HasTargetBeenHandled() const;

   FString ToString() const;
};

///////////////////////////////////////////////////////////////////
///        UTATKnowledgeComponent
///////////////////////////////////////////////////////////////////

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATKnowledgeComponent : public UOSEKnowledgeComponent, public ITATResettableAIKnowledgeContainer
{
   GENERATED_BODY()
   
   friend struct FTATActorKnowledge;
   friend struct FTATSharedTarget;

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetShared, AActor*, actor);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorVisibilityChanged, AActor*, actor, bool, newIsVisible);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlertnessTransitionAvailableChanged, bool, isAlertnessTransitionAvailable);
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorDetectionKnowledgeEvent, const FTATActorKnowledge&, actorKnowledge, EActorDetectionState, prevDetectionState);
   DECLARE_MULTICAST_DELEGATE_OneParam(FOnActorVisibilityKnowledgeEvent, const FTATActorKnowledge& actorKnowledge);
   DECLARE_MULTICAST_DELEGATE_OneParam(FOnActorKnowledgeAboutToBeAdded, const FTATActorKnowledge&);
   DECLARE_MULTICAST_DELEGATE_OneParam(FOnActorKnowledgeAboutToBeRemoved, const FTATActorKnowledge&);

public:
   UTATKnowledgeComponent();

   // static C++ facing
   static UTATKnowledgeComponent* TryGet(const AActor* actor);

   // static blueprint facing
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge", meta = (ExpandEnumAsExecs = "outValidity"))
   static UTATKnowledgeComponent* TryGetKnowledgeComponent(AActor* actor, EBranchValidity& outValidity);

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   ///////////////////////////////////////
   ///  Actor Knowledge (generic)
   ///////////////////////////////////////

   const TArray<FTATActorKnowledge>& GetKnownActors() const { return _actors; }
   TArray<FTATActorKnowledge>& GetMutableKnownActors() { return _actors; }

   /// Returns whether we have any fully identified target
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   bool HasAnyIdentifiedTargetWithAttitude(EOSETeamAttitude attitude) const;

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   TArray<AActor*> FindKnownEnemyActors() const;

   UPROPERTY(EditAnywhere)
   bool UseScheduler { false };
   
   UPROPERTY(EditAnywhere, Category="AI|Knowledge|Touch")
   float ValidTouchDirectionDotValue { 0.1f };
   ///////////////////////////////////////
   ///  Actor Knowledge (targeted)
   ///////////////////////////////////////
   
   const FTATActorKnowledge* GetActorKnowledge(const AActor* actor) const;
   FTATActorKnowledge* GetActorKnowledge(const AActor* actor) { return const_cast<FTATActorKnowledge*>(const_cast<const UTATKnowledgeComponent*>(this)->GetActorKnowledge(actor)); }

   /// Get the last known location for the given actor. Returns false if we haven't seen this actor.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   bool GetLastKnownActorLocation(AActor* actor, FVector& lastKnownLocation) const;

   /// Get the amount of time since we last saw the given actor. Returns -1 if we currently see this actor.
   /// Returns false if we haven't seen this actor.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   bool GetTimeSinceActorVisible(AActor* actor, float& timeSinceVisible) const;

   /// Returns whether we can currently see this actor.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   bool IsActorVisible(const AActor* actor) const;
   
   UPROPERTY(BlueprintAssignable, Category = "AI|Knowledge")
   FOnActorVisibilityChanged OnActorVisibilityChanged;

   FOnActorVisibilityKnowledgeEvent OnVisibilityKnowledgeChanged;
   FOnActorKnowledgeAboutToBeAdded OnActorKnowledgeAboutToBeAdded;
   FOnActorKnowledgeAboutToBeRemoved OnActorKnowledgeAboutToBeRemoved;
   
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   bool DoesAnyPathToLastKnownLocationExist(const AActor* actor);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   bool DoesAnyPathToSmartObjectSlotLocationExist(const USmartObjectComponent* smartObjectComponent, const FSmartObjectSlotHandle& slotHandle);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   bool DoesFullPathToLastKnownLocationExist(const AActor* actor);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   bool DoesFullPathToSmartObjectSlotLocationExist(const USmartObjectComponent* smartObjectComponent, const FSmartObjectSlotHandle& slotHandle);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   float GetPathLengthToLastKnownLocation(const AActor* actor);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   float GetPathLengthToSmartObjectSlotLocation(const USmartObjectComponent* smartObjectComponent, const FSmartObjectSlotHandle& slotHandle);

   /// Returns whether we can currently see this actor.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   bool IsVisibleActorInCorrectState(const AActor* actor) const;

   ///////////////////////////////////////
   ///  Detection (targeted)
   ///////////////////////////////////////
   UPROPERTY(BlueprintAssignable, Category = "AI|Knowledge|Detection")
   FOnActorDetectionKnowledgeEvent OnDetectionStateChanged;
   
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Detection")
   EActorDetectionState GetActorDetectionState(const AActor* actor) const;

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Detection")
   void ForceActorDetectionStateIdentified(const AActor* actor, FVector lastKnownLocation, float lastSeenTimestamp, bool isSharedKnowledge = false);

   /// Get the current detection value for the given actor. Returns -1 if we don't know about the actor, 0-1 if we do
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Detection")
   float GetDetectionValue(const AActor* actor) const;

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Detection")
   void SetIdentifiedStateDecayPaused(const AActor* actor, bool paused);

   ///////////////////////////////////////
   ///  Shared Target
   ///////////////////////////////////////

   // Cache a target that we intend to share with someone else.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Shared")
   bool RememberTargetToShare(const FTATSharedTarget& sharedTarget);

   // Clear any cached targets that we intended to share with someone else.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Shared")
   void ForgetTargetToShare();

   // Clear any cached targets that someone else shared with us.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Shared")
   void ForgetTargetShardWithMe();

   // Cache the partner we will share our shared target with.
   // If pass a null partner, the state will be set to PartnerNotFound.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Shared")
   bool TrySetPartnerForSharedTarget(ATATCharacterAIBase* newPartner);

   // Share our cached target with another AI.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Shared")
   bool ShareRememberedTarget(UTATKnowledgeComponent* otherKnowledge);

   // Has our cached share target been handled (i.e. destroyed)?
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Shared")
   bool HasSharedTargetBeenHandled() const;

   bool IsActorValidSharePartner(AActor* actor) const;

   // Another AI has shared a cached target with us.
   bool ReceiveSharedTarget(FTATSharedTarget& sharedTarget);

   UFUNCTION(BlueprintImplementableEvent, Category = "AI|Knowledge|Shared")
   void OnSharedKnowledgeReceived(const FTATSharedTarget& sharedTarget);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Shared")
   bool HasTargetToShare() const { return GetTargetToShare().IsValid(); }

   // Retrieve the cached target we intend to share/we have shared with someone else. 
   // May be invalid, indicating we do not have any.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Shared")
   const FTATSharedTarget& GetTargetToShare() const { return _targetToShare; }

   // Retrieve the cached target that has been shared with us. 
   // May be invalid, indicating we do not have any.
   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Shared")
   const FTATSharedTarget& GetTargetSharedWithMe() const { return _targetSharedWithMe; }

   ///////////////////////////////////////
   ///  Stims
   ///////////////////////////////////////

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Stims")
   void SetStimInvestigationState(int stimId, AActor* investigatingActor, EStimInvestigationState investigationState);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Stims")
   void SetStimCanBeForgotten(int stimId, bool canBeForgotten);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge|Stims")
   FStimInfo GetStimInfo(int stimId);
   const FStimInfo* GetStimInfo(int stimId) const;
   
   void SetStimInvestigationState(int stimID, EStimInvestigationState state);

   ///////////////////////////////////////
   ///  Fake Cylical Intelligence
   ///////////////////////////////////////

   UFUNCTION(BlueprintPure, Category = "AI|Knowledge|Fake Cyclical Intelligence")
   float GetFakeCylicalIntelligenceTimeOffset() const { return _fakeCylicalIntelligenceTimeOffset;}

   ///////////////////////////////////////
   ///  Investigation
   ///////////////////////////////////////

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   void SetIsInvestigatingActor(const AActor* actor, bool isInvestigating);

   UFUNCTION(BlueprintPure, Category = "AI|Knowledge")
   const TArray<FVector>& GetInvestigationLocations(const AActor* actor) const;

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   void AddInvestigationLocation(const AActor* actor, const FVector& location);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   void ClearInvestigationLocations(const AActor* actor);

#if WITH_GAMEPLAY_DEBUGGER
   const FVisibilityLog* GetVisibilityLogForActor(const AActor* actor) const;
#endif

   ///////////////////////////////////////
   ///  Utl
   ///////////////////////////////////////

   UFUNCTION(BlueprintGetter)
   ATATCharacterAIBase* GetAICharacter() const { return _aiCharacter; }

   // begin ITATResettableAIKnowledgeContainer
   virtual void ResetKnowledgeOfActor(AActor* actor) override;
   // end ITATResettableAIKnowledgeContainer
   
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Detection")
   TObjectPtr<const UTATDetectionSettingsAsset> DetectionSettingsAsset;

   /// How often the path to target location is calculated.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Knowledge")
   float CheckPathToLocationFrequencyInSeconds = 0.5f;

   // Enable scans for nearby actors?  Disabling this saves a potentially big sphere overlap
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Knowledge|Nearby Actors")
   bool EnableScanForNearbyActors = true;

   /// The radius used for the SphereCast that looks for nearby actors
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Knowledge|Nearby Actors")
   float NearbyActorsSphereRadius = 1300.0;

   /// How frequently to scan for allies
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Knowledge|Nearby Actors")
   float NearbyActorsDetectionFrequencyInSeconds = 2.0f;

   /// Nearby checks for allies?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Knowledge|Nearby Actors")
   bool NearbyActorsDetectAllies = true;

   /// Nearby checks for enemies?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Knowledge|Nearby Actors")
   bool NearbyActorsDetectEnemies = false;

   /// Nearby checks for neutrals?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Knowledge|Nearby Actors")
   bool NearbyActorsDetectNeutrals = false;

   /// Min random time offset for use by UConsiderationInput_FakeCyclicalIntelligence
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Knowledge|Fake Cyclical Intelligence")
   float MinFakeCylicalIntelligenceTimeOffset = 0.0f;

   /// Max random time offset for use by UConsiderationInput_FakeCyclicalIntelligence
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Knowledge|Fake Cyclical Intelligence")
   float MaxFakeCylicalIntelligenceTimeOffset = 100.0f;

   void OnIndividualAttitudeChanged(const AActor* actor);

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   FGameplayTag GetHearingStimTag(FName tag) const;

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   EStimSeverity GetHearingStimSeverity(FName tag) const;

   UFUNCTION(BlueprintCallable, Category = "AI|Knowledge")
   EStimSeverity GetGenericStimSeverity(FName tag) const;

   UFUNCTION(BlueprintCallable)
   void ResetKnowledgeOfAllActors();
   
   UFUNCTION(BlueprintCallable)
   void ResetDetectionOfAllActors();   

protected:

   UFUNCTION(BlueprintGetter)
   ATATAIController* GetTATAIController() const { return _tatAIController; }

   const FTATHearingEventStimSettings* _GetHearingEventStimSettingsForTag(FName tag) const;
   void _UpdateVisibilityLogForActor(const FTATActorKnowledge& actorKnowledge, float distance, float distanceMultiplier, float detectionRate) const;

   UPROPERTY(BlueprintGetter = "GetTATAIController", Category = "AI|Knowledge")
   ATATAIController* _tatAIController = nullptr;

   UPROPERTY(BlueprintGetter = "GetAICharacter", Category = "AI|Knowledge")
   ATATCharacterAIBase* _aiCharacter = nullptr;

   UPROPERTY(EditDefaultsOnly, Category="AI|Knowledge")
   FGameplayTagContainer _requiredTagsForVisibility;

   UPROPERTY(EditDefaultsOnly, Category = "AI|Knowledge")
   FGameplayTagContainer ProblematicActorTagsToRemember;

   // How long after the last update to a LocationKnowledge instance should we forget about it?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float ForgetLocationsCutoff = 10.0f;

private:
   // Tick:
   void _TickKnownActors(float now, float deltaTime);
   void _TickNearbyActors(float now, float deltaTime);
   void _TickKnownLocations(float now, float deltaTime);

   UFUNCTION()
   void _TargetSightPerceptionUpdated(AActor* actor, FAIStimulus stimulus);
   UFUNCTION()
   void _OnVisualStimEvent(AActor* actor, FAIStimulus stimulus);
   UFUNCTION()
   void _OnTATHearingEvent(AActor* actor, FAIStimulus stimulus);
   UFUNCTION()
   void _OnDamageEvent(AActor* actor, FAIStimulus stimulus);
   void ShareKnowledge(const FStimInfo& stimInfo) const;
   UFUNCTION()
   void _OnTeamEvent(AActor* actor, FAIStimulus stimulus);
   UFUNCTION()
   void _OnTouchEvent(AActor* actor, FAIStimulus stimulus);

   static void _UpdateActorKnowledgeFromEvent(FTATActorKnowledge& actor, const FVector& stimLocation, float timestamp);

   UFUNCTION()
   void _OnPossessedPawn(APawn* pawn);
   UFUNCTION()
   void _OnUnPossessedPawn();

   UFUNCTION()
   void _OnOwnUnconsciousTagChanged(FGameplayTag gameplayTag, int32 newTagCount);

   // Add newActorKnowledge to existing array of FTATActorKnowledge.
   FTATActorKnowledge& _FindOrAddActorKnowledge(AActor* newActor, EKnowledgeSource knowledgeSource = EKnowledgeSource::Sense);
   void _ClearKnownActors();

   // Add new stim info to existing array
   FStimInfo& _FindOrAddStimInfo(const FVector& location,
                                 AActor* instigator,
                                 const EStimType stimType,
                                 const EStimSeverity stimSeverity,
                                 const FName tag,
                                 const float strength,
                                 const int32 globalId,
                                 const FStimDatabaseQuery& query) const;
   
   const FStimInfo* _FindStimInfo(int stimId) const;
   FStimInfo* _FindStimInfo(int stimId) { return const_cast<FStimInfo*>(const_cast<const UTATKnowledgeComponent*>(this)->_FindStimInfo(stimId)); }
   
   /// Finds nearby allies/enemies and adds FTATActorKnowledge entries for them.
   void _FindNearbyActors();
   void _OnAttacked(FGameplayTag matchingTag, const FGameplayEventData* payload);
   void _OnBamboozledTagChanged(const FGameplayTag tag, int32 newTagCount);

   void _OnDetectionBlockedTagChanged(const FGameplayTag tag, int32 newTagCount);
   
   bool _CanForgetActor(const FTATActorKnowledge& actorKnowledge, float currentTime) const;
   bool _CanForgetLocation(const FTATLocationKnowledge& locationKnowledge, float currentTime) const;
   void _OnActorKnowledgeAboutToBeRemoved(const FTATActorKnowledge& actorKnowledge);
   void _TickDebugDrawLocalPlayerLastKnownLocation();
   void _OnDetectionStateChanged(const FTATActorKnowledge& actorKnowledge, EActorDetectionState prevDetectionState);

   UOSEStimDatabase& _GetStimDatabase() const;

   FTATLocationKnowledge& _FindOrAddSmartObjectSlotLocationKnowledge(const USmartObjectComponent* smartObjectComponent, const FSmartObjectSlotHandle& slotHandle);
   FTATLocationKnowledge& _FindOrAddLocationKnowledge(const UObject* contextObject, uint32 contextHash, bool& outAdded);

   void _ReevaluateSharePartner();

   void _OnSharePartnerLost();

   void _GatherSharableTargetContextTags(const FTATSharedTarget& sharedTarget, FGameplayTagContainer contextTags) const;

   UFUNCTION()
   void _OnSharePartnerDestroyed(AActor* actor);

   void _OnIsUnconsciousTagChanged(const FGameplayTag tag, int32 newTagCount);

   UFUNCTION()
   void _OnSharePartnerAlertnessChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel, AActor* alertnessIncreaseInstigator);

private:
   // Knowledge we're focused on sharing, if we can.
   FTATSharedTarget _targetToShare;

   // Knowledge that's been shared to us by someone else.
   FTATSharedTarget _targetSharedWithMe;

   float _lastNearbyActorsScanTime = 0.0f;

   UPROPERTY()
   UTATAIPerceptionComponent* _perceptionComponent = nullptr;

   UPROPERTY()
   UDataTable* _genericStimSettingsDataTable = nullptr;

   UPROPERTY()
   UDataTable* _hearingStimSettingsDataTable = nullptr;
   
   bool _detectionDisabled { false };
   
   /// Knowledge of actors we have perceived.
   UPROPERTY(Transient)
   TArray<FTATActorKnowledge> _actors;

   // For locations not tied specifically/singularly to an actor, we can cache 
   // the results of pathing queries to them.
   TArray<FTATLocationKnowledge> _contextLocations;

   // Random time offset ingested by UConsiderationInput_FakeCyclicalIntelligence to offset identical behaviors between two guards
   float _fakeCylicalIntelligenceTimeOffset = 0.0f;

   FDelegateHandle _onAttackedDelegateHandle;
   FDelegateHandle _onBamboozledDelegateHandle;
   FDelegateHandle _onDetectionDisabledDelegateHandle;
   FDelegateHandle _onSharePartnerUnconsciousDelegateHandle;
   FDelegateHandle _onUnconsciousDelegateHandle;

#if DO_CHECK
   bool _bCheckNoModificationsToActorArray { false };
#endif
};
