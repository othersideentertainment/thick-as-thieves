// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/SmartObjects/TATAIIncorrectObjectStateInterface.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "AI/SmartObjects/TATSmartObjectTagInterface.h"
#include "Breakables/TATBreakableBase.h"
#include "Interactables/TATSupportInteractionByInterface.h"
#include "Interactables/TATLockConfig.h"
#include "TATInteractionGateInterface.h"
#include "Lockpicking/LockpickableInterface.h"
#include "Variation/TATSpawnerFwd.h"
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"
#include "TATLockableInterface.h"
#include "AI/Navigation/TATNavLinkOwnerInterface.h"
#include "Tools/TATWardableInterface.h"

// ose
#include "Interactables/InteractableInterface.h"
#include "OSEVoiceLineTraitInterface.h"

// ue4
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NativeGameplayTags.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISightTargetInterface.h"

#include "TATSwingingDoor.generated.h"

class UAkAudioEvent;
class UTATActionNodeComponent_IncorrectObjectState;
class UTATAudioPortalComponent;
class UBillboardComponent;
class UBoxComponent;
class UNavLinkCustomComponent;
class USwingingDoorNavLinkComponent;
class UTATSecurityLockdownComponent;
struct FTATAuthorityBreakContext;

UENUM(BlueprintType)
enum class ESwingingDoorPosition : uint8
{
   Closed,
   OpenFront, // front and back are arbitrary directions for lack of a better name
   OpenBack
};

UENUM(BlueprintType)
enum class EDoorLockDirection : uint8
{
   Both = 3,
   LockFrontOnly = 2, // front and back are arbitrary directions for lack of a better name
   LockBackOnly = 1,
   None = 0
};

UENUM(BlueprintType)
enum class EDoorOpenDirection : uint8
{
   Both = 3,
   OpenFrontOnly = 2, // front and back are arbitrary directions for lack of a better name
   OpenBackOnly = 1,
   None = 0
};

USTRUCT(BlueprintType)
struct FTATSwingingDoorState
{
   GENERATED_BODY()

public:
   // Server time of the last time that the toggle was changed
   UPROPERTY(Transient)
   float ChangedServerTime = 0;

   UPROPERTY(EditAnywhere)
   ESwingingDoorPosition Position = ESwingingDoorPosition::Closed;

   UPROPERTY(Transient)
   bool bIsBroken { false };

   bool IsOld(UObject* worldContext, float thresholdSeconds) const;
};

USTRUCT()
struct FTATSwingingDoorAllowedDirections
{
   GENERATED_BODY()

public:
   // which directions the door can be re-locked from
   UPROPERTY(EditAnywhere, Category = "Lock", DisplayName = "Allowed Relock Direction")
   EDoorLockDirection RelockDirection = EDoorLockDirection::Both;

   // which direction the door can be lockpicked from
   UPROPERTY(EditAnywhere, Category = "Lock", DisplayName = "Allowed Lockpick Direction")
   EDoorLockDirection LockpickableDirection = EDoorLockDirection::Both;

   // which direction a key can be used from
   UPROPERTY(EditAnywhere, Category = "Lock", DisplayName = "Allowed Key Direction")
   EDoorLockDirection KeyDirection = EDoorLockDirection::Both;

   // which direction the door can be opened (or interacted with when closed) from
   UPROPERTY(EditAnywhere, Category = "Lock")
   EDoorOpenDirection AllowedOpenDirection = EDoorOpenDirection::Both;
};

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_BreakDownDoor_Events_StartBreakingDownDoor)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_BreakDownDoor_Events_DoBreakDownDoor)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Object_Broken_Door)

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAuthorityOnDoorRecentlyChangedState);

UCLASS(Blueprintable)
class TAT_API ATATSwingingDoor : public ATATBreakableBase,
   public IInteractableInterface,
   public ILockpickableInterface,
   public ITATSupportInteractionByInterface,
   public INavAgentInterface,
   public ITATLockableInterface,
   public ITATNavLinkOwnerInterface,
   public ITATUtilityAITargetingGroupInterface,
   public IOSEVoiceLineTraitInterface,
   public IAISightTargetInterface,
   public ITATWardableInterface,
   public ITATAIIncorrectObjectStateInterface,
   public ITATSmartObjectTagInterface,
   public ITATSmartObjectOwnerInterface
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   ATATSwingingDoor();

   // from AActor
   virtual void OnConstruction(const FTransform& transform) override;
   virtual void PostInitializeComponents() override;
   virtual void PostRegisterAllComponents() override;

   // from UObject
#if WITH_EDITOR
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostLoad() override;
#endif // WITH_EDITOR
 
   // from INavAgentInterface
   virtual FVector GetNavAgentLocation() const override;
   virtual void GetMoveGoalReachTest(const AActor* MovingActor,
                                     const FVector& MoveOffset,
                                     FVector& GoalOffset,
                                     float& GoalRadius,
                                     float& GoalHalfHeight) const override;
   
   // from IOSEVoiceLineTraitInterface
   virtual void GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const override;

   // ITATUtilityAITargetingGroupInterface start
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;
   // ITATUtilityAITargetingGroupInterface end
   
   // IAISightTargetInterface start
   virtual bool CanBeSeenFrom(const FVector& observerLocation,
                              FVector& outSeenLocation,
                              int32& numberOfLoSChecksPerformed,
                              float& outSightStrength,
                              const AActor* ignoreActor = nullptr,
                              const bool* wasVisible = nullptr, int32* userData = nullptr) const override;
   // IAISightTargetInterface end

   // from ITATWardableInterface
   virtual bool CanActivateWard_Implementation() const override;
   virtual FTATWardPlacementInfo GetWardPlacementInfo_Implementation() const override { return FTATWardPlacementInfo(_visualBounds); }
   virtual void OnWardActivated_Implementation(AActor* wardActor, APawn* instigator) override;
   virtual void OnWardDeactivated_Implementation(AActor* wardActor) override;

   // from ITATAIIncorrectObjectStateInterface
   virtual bool AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const override;

   // from ITATSmartObjectTagInterface
   virtual FGameplayTagCountContainer& GetGameplayTagCountContainer() override;

 #if WITH_EDITOR
   virtual void CheckForErrors() override;
 #endif

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

   // A spawner component can drive whether or not this actor is locked based on mission rules
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock|Spawner")
   UTATSpawnerComponent* _lockedSpawnerComponent = nullptr;

   /// Drives whether this door spawns open or closed (spawn = open, not-spawn = closed)
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|Open")
   UTATSpawnerComponent* _openSpawnerComponent = nullptr;

   /// The visual bounding box of the door.
   /// Used for the door ward, and anything else that can target doors.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Door")
   TObjectPtr<UBoxComponent> _visualBounds;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|AI")
   UTATActionNodeComponent_IncorrectObjectState* _incorrectStateActionNodeComponent = nullptr;

public:   
   // Called every frame
   virtual void Tick(float DeltaTime) override;

   // from ITATSmartObjectOwnerInterface
   UFUNCTION(BlueprintCallable)
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const override;

   UFUNCTION(BlueprintCallable)
   void SetLocked(bool newIsLocked);
   UFUNCTION(BlueprintCallable)
   void SetClosedState(bool newIsClosed);
   UFUNCTION(BlueprintCallable, BlueprintPure)
   int32 GetLockLevel() const { return _lockConfig.GetFinalLockLevel(); }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   FORCEINLINE bool IsLocked() const { return _locked; }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   FORCEINLINE bool IsOpen() const { return _state.Position != ESwingingDoorPosition::Closed; }
   UFUNCTION(BlueprintCallable, BlueprintPure)
   bool IsBroken() const;
   void SetLockDirection(EDoorLockDirection lockDirection);

   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;

   // ILockpickableInterface
   virtual void Unlock() override { SetLocked(false); }
   virtual void Lock() override { SetLocked(true); }
   virtual void LockWithKey() override;
   virtual void OnLockpickTrackCompleted(int32 trackIndex) override;
   virtual int32 GetLockpickCurrentTrack() override { return _lockpickCurrentTrack; }
   virtual FTATOnRequestCancelLockpicking* GetOnRequestCancelLockpickingDelegate() override { return &_onRequestCancelLockpicking; }

   // ITATSupportInteractionByInterface
   virtual bool DoesSupportInteractionBy_Implementation(ACharacter* interactor, const FGameplayTag& interactorIdentity) const override;

   UFUNCTION(BlueprintPure)
   USceneComponent* GetPivot() const { return _pivot; }

   // For nav-link
   bool CanTraverseDoor(const UObject* probablyController, bool isFront) const;

   UFUNCTION(BlueprintCallable)
   bool CanControllerBreakDoor(const AController* controller) const;

   // if interaction is completely blocked (largely from interaction gates)
   // Initially for door traversal ability
   UFUNCTION(BlueprintCallable)
   bool IsInteractionBlocked() const;

   // from ITATNavLinkOwnerInterface
   virtual UTATNavLinkOwnerComponent* GetNavLinkOwnerComponent_Implementation() const override { return _navLinkOwnerComponent; };

   UPROPERTY(BlueprintAssignable)
   FAuthorityOnDoorRecentlyChangedState OnAuthorityDoorRecentlyChangedState;
protected:
   UFUNCTION(BlueprintNativeEvent, Category=Door, meta = (BlueprintProtected=true))
   void OnDoorRecentlySwung(ESwingingDoorPosition newPosition);
   void OnDoorRecentlySwung_Implementation(ESwingingDoorPosition newPosition);

   UFUNCTION(BlueprintNativeEvent, Category=Door, meta = (BlueprintProtected=true))
   void OnDoorPositionChanged(ESwingingDoorPosition newPosition);
   void OnDoorPositionChanged_Implementation(ESwingingDoorPosition newPosition) {}

   UFUNCTION(BlueprintPure, Category=Door, meta = (BlueprintProtected = true))
   bool ShouldVisualizeLockInDirection(bool isFrontSide) const;

   UFUNCTION(BlueprintPure, Category = Door, meta = (BlueprintProtected = true))
   bool ShouldVisualizeUnopenableDirection(bool isFrontSide) const;

   UFUNCTION(BlueprintImplementableEvent, Category=Door, meta = (BlueprintProtected=true))
   void UpdateLockdownVisual(bool bIsInLockdown);

   UFUNCTION(BlueprintCallable, Category = Door, meta = (BlueprintProtected = true))
   void SetSecondaryPivot(USceneComponent* secondaryPivot);

   UFUNCTION(BlueprintNativeEvent, Category = Door, meta = (BlueprintProtected = true))
   void OnRecentlyBroken(bool wasClosed);
   void OnRecentlyBroken_Implementation(bool wasClosed);

   UFUNCTION(BlueprintNativeEvent, Category = Door, meta = (BlueprintProtected = true))
   void OnBrokenChanged(bool isBroken);
   void OnBrokenChanged_Implementation(bool isBroken);

   UFUNCTION(BlueprintCallable)
   virtual FLockInteractContext MakeLockContext(ACharacter* interactingCharacter) const override;

private:
   void _AuthorityOnBroken(const FTATAuthorityBreakContext& context);

   void _SetPivotRotation(FRotator newRotation);
   void _PopulateAnimationTagForSwing(FInteractStartResult& result) const;
   bool _IsOnFrontSide(const FVector& characterPosition) const;
   ESwingingDoorPosition OpenDirectionFor(const FVector& characterPosition) const;
   FRotator RotatorFor(ESwingingDoorPosition position) const;
   float AngleFor(ESwingingDoorPosition position) const;
   void SnapTo(ESwingingDoorPosition position);
   FTATSwingingDoorAllowedDirections _GetAllowedDirections() const;
   bool _DoesLockApplyInActorDirection(const AActor* actor) const;
   bool _DoesLockApplyInDirection(bool isOnFront) const;
   bool _IsOpenableFromActorDirection(const AActor* actor) const;
   bool _IsOpenableFromDirection(bool isOnFront) const;
   void _SwingForInteract(ACharacter* interactingCharacter, bool bSetBroken);
   void _SetPosition(ESwingingDoorPosition newPosition);
   void _UpdatePortalObstructionForAngle(float angle);

   UFUNCTION()
   void _OnSecurityLockdownStateChanged(bool bIsInLockdown);

   UFUNCTION()
   void OnRep_State(const FTATSwingingDoorState& previous);

   UFUNCTION()
   void _AuthorityOnLockedSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);
   UFUNCTION()
   void _AuthorityOnLockedSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

   UFUNCTION()
   void _AuthorityOnOpenSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

   UFUNCTION()
   void _AuthorityOnOpenSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

   void _RecomputePivotBounds();

#if WITH_EDITOR
   void _RefreshAkPostLocationEditorPreview();
#endif // WITH_EDITOR

   void _RefreshPivotComponentOffsets();

   void _UpdateIncorrectStateInitialState();

   UPROPERTY(EditDefaultsOnly)
   UAIPerceptionStimuliSourceComponent* _perceptionStimuliSource = nullptr;

protected:
   // hopefully, this can be overridden with a mesh component in the BP
   UPROPERTY(EditDefaultsOnly, meta = (DisplayName="Pivot", ScriptName="Pivot"))
   USceneComponent* _pivot;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   UTATAudioPortalComponent* _portalComponent = nullptr;

   // If true, the AkPortal is always open whether the door is open or closed
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Door|Audio")
   bool PortalAlwaysOpen = false;

private:
   UPROPERTY(EditDefaultsOnly)
   UTATNavLinkOwnerComponent* _navLinkOwnerComponent { nullptr };
   
   UPROPERTY(ReplicatedUsing=OnRep_State, EditAnywhere, Category = "Door", meta = (DisplayName = "Pivot"))
   FTATSwingingDoorState _state;

   UPROPERTY(Replicated, EditAnywhere, Category = "Lock")
   bool _locked;

   // Whether it has been locked down by an alarm, stays locked down
   UPROPERTY(Transient, Replicated)
   bool _lockedDown = false;

   UPROPERTY(Replicated, Transient)
   int32 _lockpickCurrentTrack = 0;;

   UPROPERTY(EditAnywhere, Category="Lock")
   FTATLockConfig _lockConfig;

   UPROPERTY(EditAnywhere, Category = "Lock", DisplayName = "Initial Lock Direction", Replicated, meta=(InvalidEnumValues="None"))
   EDoorLockDirection _lockDirection = EDoorLockDirection::Both;

#if WITH_EDITORONLY_DATA
   UPROPERTY()
   EDoorLockDirection _relockDirection_DEPRECATED = EDoorLockDirection::Both;

   UPROPERTY()
   EDoorOpenDirection _allowedOpenDirection_DEPRECATED = EDoorOpenDirection::Both;
#endif

   UPROPERTY(EditAnywhere, Category = "Lock", meta = (InlineEditConditionToggle))
   bool _overrideLockdownDirections = false;

   UPROPERTY(EditAnywhere, Category = "Lock")
   FTATSwingingDoorAllowedDirections _normalAllowedDirections;
   
   UPROPERTY(EditAnywhere, Category = "Lock", meta = (EditCondition = "_overrideLockdownDirections"))
   FTATSwingingDoorAllowedDirections _lockdownAllowedDirections;

   UPROPERTY(EditDefaultsOnly, Category = "Door", AdvancedDisplay)
   FText _openPrompt;

   UPROPERTY(EditDefaultsOnly, Category = "Door", AdvancedDisplay)
   FText _closePrompt;

   UPROPERTY(EditDefaultsOnly, Category = "Door", AdvancedDisplay)
   FText _unopenableFromSidePrompt;

   UPROPERTY(EditDefaultsOnly, Category = Door, AdvancedDisplay, meta = (Categories = "InteractAnimation.Instant"))
   FGameplayTag _openInteractAnimationTag;

   UPROPERTY(EditDefaultsOnly, Category = Door, AdvancedDisplay, meta = (Categories = "InteractAnimation.Instant"))
   FGameplayTag _closeInteractAnimationTag;

   UPROPERTY()
   USceneComponent* _secondaryPivot = nullptr;

   UPROPERTY(VisibleAnywhere)
   USwingingDoorNavLinkComponent* _navLinkFront;

   UPROPERTY(VisibleAnywhere)
   USwingingDoorNavLinkComponent* _navLinkBack;

   UPROPERTY(EditAnywhere, Category="TAT|Security", meta=(DisplayPriority=2))
   UTATSecurityLockdownComponent* _securityLockdownComponent = nullptr;

   // angle in degrees around swing axis when closed
   UPROPERTY(EditDefaultsOnly, Category="Door|Swing")
   float _closedAngle;

   // angle in degrees around swing axis when open to the front
   UPROPERTY(EditDefaultsOnly, Category = "Door|Swing")
   float _openFrontAngle;

   // angle in degrees around swing axis when open to the back
   UPROPERTY(EditDefaultsOnly, Category = "Door|Swing")
   float _openBackAngle;

   // angle in degrees to offset the secondary pivot by
   UPROPERTY(EditDefaultsOnly, Category = "Door|Swing")
   float _secondaryAngleOffset;

   // duration of open/close in seconds
   UPROPERTY(EditDefaultsOnly, Category = "Door|Swing")
   float _swingDuration;

   // multiplier on swing speed when broken
   UPROPERTY(EditDefaultsOnly, Category = "Door|Swing")
   float _brokenSwingSpeedMultiplier;

   // Direction pointing in the front of the door
   UPROPERTY(EditDefaultsOnly, Category = "Door|Swing", meta = (MakeEditWidget=true))
   FVector _frontDirection;

   UPROPERTY(EditDefaultsOnly, Category = "Door|Swing")
   TEnumAsByte<EAxis::Type> _swingAxis;

   UPROPERTY(EditDefaultsOnly, Category = "Door|Interaction", meta = (Categories = "Ability.Interact.Interactor"))
   FGameplayTagContainer _allowedInteractors;

   UPROPERTY(EditDefaultsOnly, Category = "Door|Audio")
   UAkAudioEvent* _openedAudioEvent = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "Door|Audio")
   UAkAudioEvent* _closedAudioEvent = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "Door|Audio")
   UAkAudioEvent* _brokenWhileOpenAudioEvent = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "Door|Audio")
   UAkAudioEvent* _brokenWhileClosedAudioEvent = nullptr;

   // Relative location to post audio events from
   UPROPERTY(EditDefaultsOnly, Category = "Door|Audio")
   FVector _audioEventPostLocation;

   FTATOnRequestCancelLockpicking _onRequestCancelLockpicking;

   UPROPERTY(Transient)
   FTATInteractionGateCollection _interactionGates;

#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   UBillboardComponent* _akEventPostLocationEditorBillboard = nullptr;
#endif // WITH_EDITORONLY_DATA

   UPROPERTY(EditDefaultsOnly)
   UBoxComponent* _portalBoxTrigger = nullptr;

   // If the door is warded, this is the ward actor
   TWeakObjectPtr<AActor> _currentWard;

   FORCEINLINE bool _IsDoorWarded() const { return _currentWard.Get() != nullptr; }

   FBoxSphereBounds _pivotBounds;
};
