// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "StateTrees/Targeting/TATStateTreeTargetingComponent.h"
#include "TATKnowledgeComponent.h"
#include "Combat/TATCombatPositioningComponent.h"

// ose
#include "AI/OSEAIController.h"
#include "AI/Perception/OSEStimDatabaseInterface.h"
#include "OSEIndividualAttitudeInterface.h"
#include "OSEIndividualKnowledgeInterface.h"
#include "OSEVoiceLineTraitInterface.h"
#include "OSEVoiceLineKnowledgeInterface.h"

// ue4
#include "GameplayAbilitySpecHandle.h"

#include "TATAIController.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTATAIController, Log, All);

class UOSEGameplayAbility;
class UOSEIndividualAttitudeComponent;
class UOSEIndividualKnowledgeComponent;
class UOSEStimDatabase;
class ATATCharacterAIBase;
class UTATNavLinkCustomComponent;
class UTATKnowledgeComponent;
class ATATPrivateSpaceVolume;
class UTATEscalationComponent;
struct FUtilityStateEvaluatorInstance;
struct FUtilityStateTarget;
class UAITask_UseSpecificGameplayBehaviorOnSmartObject;

USTRUCT()
struct TAT_API FTATAIDynamicBehaviorTreeInjection
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag InjectionTag;

   UPROPERTY(EditAnywhere)
   UBehaviorTree* BehaviorTree = nullptr;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnStimAdded, FStimInfo&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStimAboutToBeRemoved, const FStimInfo&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTATOnAIResourceLockChanged, bool movementLocked, bool logicLocked);

UCLASS()
class TAT_API ATATAIController
   : public AOSEAIController
   , public IOSEStimDatabaseInterface
   , public IOSEStimDatabaseOwnerInterface
   , public IOSEIndividualAttitudeInterface
   , public IOSEVoiceLineKnowledgeInterface
   , public IOSEIndividualKnowledgeInterface
   , public IOSEVoiceLineTraitInterface
{
   GENERATED_BODY()

public:

   ATATAIController(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   // from AActor
   virtual void PostInitializeComponents() override;
   virtual void BeginPlay() override;
   virtual void Tick(float deltaTime) override;

   // from AController
   virtual void SetPawn(APawn* inPawn) override;
   virtual void OnPossess(APawn* inPawn) override;
   virtual void OnUnPossess() override;

   UFUNCTION(BlueprintPure, Category = "TAT|AI")
   UTATKnowledgeComponent* GetTATKnowledgeComponent() const { return TATKnowledgeComponent; }

   UFUNCTION(BlueprintPure, Category = "TAT|AI")
   UTATEscalationComponent* GetTATEscalationComponent() const { return _escalationComponent; }

   //---------------------------------------------------------------------------------------
   // ITATStateTreeCombatOwnerInterface
   //---------------------------------------------------------------------------------------
   void OnEnterTargetingActorForStateTree(AActor* actor);
   void OnExitTargetingActorForStateTree();
   bool IsCurrentStateTreeTarget(const AActor* actor) const { return _currentCombatTargetForStateTree == actor; }
   
   //---------------------------------------------------------------------------------------
   // IOSEStimDatabaseInterface
   //---------------------------------------------------------------------------------------

   virtual UOSEStimDatabase* AuthorityGetStimDatabase() const override;

   //---------------------------------------------------------------------------------------
   // IOSEStimDatabaseOwnerInterface
   //---------------------------------------------------------------------------------------

   virtual void AuthorityOnStimAddedToDatabase(FStimInfo& stimInfo) override { OnStimAdded.Broadcast(stimInfo); }
   virtual void AuthorityOnStimAboutToBeRemovedFromDatabase(const FStimInfo& stimInfo) override { OnStimAboutToBeRemoved.Broadcast(stimInfo); }
   virtual void AuthorityOnStimPerceivedByActorsChanged(FStimInfo& stimInfo, AActor* perceivedBy) override;

   FOnStimAdded OnStimAdded;
   FOnStimAboutToBeRemoved OnStimAboutToBeRemoved;
   
   //---------------------------------------------------------------------------------------
   // IOSEIndividualAttitudeInterface
   //---------------------------------------------------------------------------------------
   virtual UOSEIndividualAttitudeComponent* GetAttitudeComponent() const override;
   
   //---------------------------------------------------------------------------------------
   // IOSEIndividualKnowledgeInterface
   //---------------------------------------------------------------------------------------
   virtual UOSEIndividualKnowledgeComponent* GetIndividualKnowledgeComponent() const override;

   //---------------------------------------------------------------------------------------
   // IOSEVoiceLineTraitInterface
   //---------------------------------------------------------------------------------------
   virtual void GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const override;   

   //---------------------------------------------------------------------------------------
   // IOSEVoiceLineKnowledgeInterface
   //---------------------------------------------------------------------------------------
   virtual UOSEVoiceLineKnowledgeComponent* GetVoiceLineKnowledgeComponent() const override;

   #if ENABLE_VISUAL_LOG
   virtual void GrabDebugSnapshot(FVisualLogEntry* snapshot) const override;
   #endif

   bool HasTrait(const FGameplayTag& traitTag) const;

   UFUNCTION(BlueprintCallable, BlueprintPure)
   static FName GetGuardLocationBlackboardKey() { return kGuardLocationBlackboardKey; }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static FName GetAlertnessLevelBlackboardKey() { return kAlertnessLevelBlackboardKey; }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static FName GetTargetActorBlackboardKey() { return kTargetActorBlackboardKey; }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static FName GetTargetLocationBlackboardKey() { return kTargetLocationBlackboardKey; }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static FName GetMoveBlockedFromLocationBlackboardKey() { return kMoveBlockedFromLocationBlackboardKey; }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static FName GetMovedBlockedFromDirectionBlackboardKey() { return kMovedBlockedFromDirectionBlackboardKey; }
   
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static FName GetPatrolBrokenLocationBlackboardKey() { return kPatrolPathBrokenLocationKey; }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static FName GetPatrolIndexBlackboardKey() { return kPatrolPathIndexKey; }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static FName GetPatrolPointDirectionForward() { return kPatrolPointDirectionForward; }

   ATATPrivateSpaceVolume* GetPrivateSpaceControllerAssignedTo() const { return _assignedPrivateSpaceVolume; };

   void StartNavLinkBehavior(TSubclassOf<UOSEGameplayAbility> navLinkBehavior, UTATNavLinkCustomComponent* tatNavLink);
   void FinishNavLinkBehavior() const;

   bool GetMoveFromLocationRequested() const;
   UFUNCTION(BlueprintCallable)
   void SetMoveFromLocationRequested(bool locationMoveRequested);
   
   virtual void UpdateControlRotation(float deltaTime, bool updatePawn = true) override;

   bool GetShouldSeeGuardsAsHostileWhenDamaged() const { return ShouldSeeGuardsAsHostileWhenDamaged; };

   void LockAIResources(bool lockMovement, bool lockLogic);
   void UnlockAIResources(bool unlockMovement, bool unlockLogic);
   void GetAIResourceLockStates(bool& isMovementLocked, bool& isLogicLocked);

   FTATOnAIResourceLockChanged OnAIResourceLockChanged;

   UTATStateTreeTargetingComponent* GetStateTreeTargetingComponent() const { return _tatStateTreeTargetingComponent; }
   
   void SetCurrentActiveSmartObjectBehavior(UAITask_UseSpecificGameplayBehaviorOnSmartObject* newSmartObjectBehaviorTask);
   void SendStateTreeEvent(const FStateTreeEvent& event) const;

   void SetRequestedCombatPosition(const ECombatPosition combatPosition) { _combatPosition = combatPosition; };
   ECombatPosition GetRequestedCombatPosition() const { return _combatPosition; };

   virtual FPathFollowingRequestResult MoveTo(const FAIMoveRequest& moveRequest, FNavPathSharedPtr* outPath = nullptr) override;

   bool ShouldForceMoveRequestsToBlockPathfinding() const { return _forceMoveToRequestsToBlockPathfinding; }
protected:
   UFUNCTION()
   void _HandleDetectionStateChanged(const FTATActorKnowledge& actorKnowledge, EActorDetectionState prevDetectionState);
   UFUNCTION()
   void _OnNavLinkTreeRunStateChanged(EStateTreeRunStatus stateTreeRunStatus);
   UFUNCTION()
   void OnAIGoalStateEnteredWithNavLinkBehavior(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target);
   UFUNCTION()
   void OnAIBehaviorStateEnteredWithNavLinkBehavior(const FUtilityStateEvaluatorInstance& state, const FUtilityStateTarget& target);   
   FGameplayAbilitySpecHandle _navLinkBehaviorHandle;

   bool _moveFromLocationRequested { false };
   // Blackboard Keys
   inline const static FName kGuardLocationBlackboardKey = FName(TEXT("GuardLocation"));
   inline const static FName kAlertnessLevelBlackboardKey = FName(TEXT("AlertnessLevel"));
   inline const static FName kTargetActorBlackboardKey = FName(TEXT("TargetActor"));
   inline const static FName kTargetLocationBlackboardKey = FName(TEXT("TargetLocation"));
   
   inline const static FName kPatrolPathBrokenLocationKey = FName(TEXT("PatrolPathBrokenLocation"));
   inline const static FName kPatrolPathIndexKey = FName(TEXT("PatrolPointIndex"));
   inline const static FName kPatrolPointDirectionForward = FName(TEXT("PatrolPointDirectionForward"));

   inline const static FName kMoveBlockedFromLocationBlackboardKey = FName(TEXT("MoveBlockedFromLocation"));
   inline const static FName kMovedBlockedFromDirectionBlackboardKey = FName(TEXT("MoveBlockedFromDirection"));

   
   UPROPERTY(EditDefaultsOnly, Category="AI|TAT", meta=(Categories="AI.Trait"))
   FGameplayTagContainer Traits;
   
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|TAT")
   UTATKnowledgeComponent* TATKnowledgeComponent = nullptr;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|TAT")
   bool ShouldSeeGuardsAsHostileWhenDamaged { false };

   UPROPERTY(BlueprintReadOnly, Category="AI|TAT", Transient)
   UTATStateTreeTargetingComponent* _tatStateTreeTargetingComponent { nullptr };
   
   UPROPERTY(BlueprintReadWrite)
   ATATPrivateSpaceVolume* _assignedPrivateSpaceVolume;

   UPROPERTY(EditDefaultsOnly, Category="AI|Individual Knowledge")
   UOSEIndividualKnowledgeComponent* _individualKnowledgeComponent { nullptr };
   
   UPROPERTY(EditDefaultsOnly, Category="AI|Voice Lines")
   UOSEVoiceLineKnowledgeComponent* _voiceLineKnowledgeComponent { nullptr };
   
private:
   UPROPERTY()
   TWeakObjectPtr<AActor> _currentCombatTargetForStateTree { nullptr };
   
   UPROPERTY()
   UOSEStimDatabase* _stimDatabase = nullptr;

   UPROPERTY(EditDefaultsOnly)
   UTATEscalationComponent* _escalationComponent { nullptr };
   
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UTATStateTreeAIComponent> _stateTreeAIComponent { nullptr };

   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UTATStateTreeAIComponent> _offNavLinkAIComponent { nullptr };

   UPROPERTY(EditDefaultsOnly)
   bool _shouldEnableOffNavLinkBehavior { true };

   // Used for "Ghost" NPCs
   UPROPERTY(EditDefaultsOnly)
   bool _forceMoveToRequestsToBlockPathfinding { false };
   
   UPROPERTY(Transient)
   TWeakObjectPtr<UAITask_UseSpecificGameplayBehaviorOnSmartObject> _currentlyActiveSmartObjectBehaviorTask { nullptr };

   ECombatPosition _combatPosition { ECombatPosition::Close};   
};
