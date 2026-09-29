// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"
#include "Tutorial/TATTutorialCondition.h"

// ose
#include "AI/Alertness/AlertnessEnums.h"

// ue
#include "GameplayTagContainer.h"

#include "TATCommonTutorialConditions.generated.h"

class UTATScreenWidget;
class UCommonActivatableWidget;
struct FTATLootIdentifier;
class ATATCharacterSpawner;
class ATATSwingingDoor;
class UToolComponent;
class UTATItemInfo;

enum class ETATContractState : uint8;


UCLASS(Abstract, Blueprintable)
class TAT_API UTATTutorialCondition_BlueprintBase : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   UFUNCTION(BlueprintImplementableEvent)
   bool BP_IsMet(const FTATTutorialParams& params) const;

   UFUNCTION(BlueprintImplementableEvent)
   FString BP_GetDebugName() const;

   UFUNCTION(BlueprintImplementableEvent)
   void BP_Validate(const FTATTutorialValidationParams& params) const;
};

// The player has the specified tool
UCLASS(DisplayName = "Has Tool")
class TAT_API UTATTutorialCondition_HasTool : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UPROPERTY(EditAnywhere)
   TSoftClassPtr<UToolComponent> ToolClass;
};

// All of these conditions are true
UCLASS(DisplayName = "All Of")
class TAT_API UTATTutorialCondition_AllOf : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UPROPERTY(EditAnywhere)
   TArray<TObjectPtr<UTATTutorialCondition>> Conditions;
};

// Any of these conditions are true
UCLASS(DisplayName = "Any Of")
class TAT_API UTATTutorialCondition_AnyOf : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UPROPERTY(EditAnywhere)
   TArray<TObjectPtr<UTATTutorialCondition>> Conditions;
};

// The player has equipped the specified tool
UCLASS(DisplayName = "Has Tool Equipped")
class TAT_API UTATTutorialCondition_ToolEquipped : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UPROPERTY(EditAnywhere)
   TSoftClassPtr<UToolComponent> ToolClass;

   UPROPERTY(EditAnywhere)
   bool ShouldBeEquipped = true;
};

// The player has the specified key
UCLASS(DisplayName = "Has Key")
class TAT_API UTATTutorialCondition_HasKey : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UPROPERTY(EditAnywhere)
   TSoftClassPtr<UTATItemInfo> KeyType;
};


// The player has the specified key
UCLASS(DisplayName = "Has Loot")
class TAT_API UTATTutorialCondition_HasLoot : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UPROPERTY(EditAnywhere, meta = (Categories = "Loot"))
   FTATLootIdentifier Loot;
};

// The player has entered the specified volume
UCLASS(DisplayName = "Is in Volume")
class TAT_API UTATTutorialCondition_IsInVolume : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UPROPERTY(EditAnywhere, meta=(AllowedClasses="/Script/TAT.TATTutorialTriggerVolumeInterface"))
   TSoftObjectPtr<AActor> TriggerVolume;
};

// The player has learned the specified clue fact
UCLASS(DisplayName = "Has Clue Fact")
class TAT_API UTATTutorialCondition_HasClueFact : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

   UPROPERTY(EditAnywhere, meta=(Categories="ClueFact"))
   FGameplayTag FactTag;
};

// There are no open UI screens in the screen stack, or pending UIQueues running
UCLASS(DisplayName = "No Open Screens")
class TAT_API UTATTutorialCondition_NoOpenScreens : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
};

// CommonUI activatable widget is active and shown
UCLASS(DisplayName = "Is Widget Active")
class TAT_API UTATTutorialCondition_IsWidgetActive : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   // The widget that should be open
   UPROPERTY(EditAnywhere)
   TSoftClassPtr<UCommonActivatableWidget> WidgetClass;
};

// TATScreenWidget is active and shown (on top)
UCLASS(DisplayName = "Is Screen Open")
class TAT_API UTATTutorialCondition_IsScreenOpen : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   // The screen that should be open
   UPROPERTY(EditAnywhere)
   TSoftClassPtr<UTATScreenWidget> ScreenClass;
};

UCLASS(DisplayName = "Is Content Unlocked")
class TAT_API UTATTutorialCondition_IsContentUnlocked : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;

public:
   // The content that is required to be unlocked
   UPROPERTY(EditAnywhere, meta=(Categories="UnlockableCategory"))
   FGameplayTagContainer RequiredContent;
};

// The player character is ready
UCLASS(DisplayName = "Is Character Ready")
class TAT_API UTATTutorialCondition_IsCharacterReady : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
};

// The game is offline, and not connected
UCLASS(DisplayName = "Is Offline")
class TAT_API UTATTutorialCondition_IsOffline : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
};


// The player is targeting the specified interactable actor in the level
UCLASS(DisplayName = "Is Targeting Interactable")
class TAT_API UTATTutorialCondition_IsTargetingInteractable : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   //UPROPERTY(EditAnywhere, meta=(AllowedClasses="/Script/OSEInteraction.InteractableInterface"))
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<AActor> Interactable;
};

// The player is targeting an interactable of the specifies class
UCLASS(DisplayName = "Is Targeting Interactable Class")
class TAT_API UTATTutorialCondition_IsTargetingInteractableClass : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   UPROPERTY(EditAnywhere, meta=(MustImplement="/Script/OSEInteraction.InteractableInterface", AllowAbstract))
   TSoftClassPtr<AActor> InteractableClass;
};

UCLASS(DisplayName = "Player Matches Tags")
class TAT_API UTATTutorialCondition_PlayerMatchesTags : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;

public:
   UPROPERTY(EditAnywhere)
   FGameplayTagContainer RequiredTags;
   
   UPROPERTY(EditAnywhere)
   FGameplayTagContainer BlockedTags;
};

UCLASS(DisplayName = "NPC Has Alertness")
class TAT_API UTATTutorialCondition_NPCAlertness : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   // The spawner for the guard to check the tag on
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<ATATCharacterSpawner> GuardSpawner;
   
   UPROPERTY(EditAnywhere)
   EAlertnessLevel MinimumAlertness = EAlertnessLevel::Suspicious;
};

UCLASS(Abstract, Blueprintable)
class TAT_API UTATTutorialCondition_NPCIsOnscreen : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   // The spawner for the guard to check the tag on
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<ATATCharacterSpawner> GuardSpawner;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin=0, UIMin=0))
   float ScreenPercentage = 1;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin=0, UIMin=0))
   FVector TargetWorldOffset = FVector::ZeroVector;
};

UCLASS(DisplayName = "NPC Matches Tags")
class TAT_API UTATTutorialCondition_NPCMatchesTags : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   // The spawner for the guard to check the tag on
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<ATATCharacterSpawner> GuardSpawner;
   
   UPROPERTY(EditAnywhere)
   FGameplayTagContainer RequiredTags;
   
   UPROPERTY(EditAnywhere)
   FGameplayTagContainer BlockedTags;
};

UCLASS(DisplayName = "Actor is (Un)locked")
class TAT_API UTATTutorialCondition_ActorIsUnlocked : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   UPROPERTY(EditAnywhere, meta=(AllowedClasses="/Script/TAT.TATLockableToggle,/Script/TAT.TATSwingingDoor"))
   TSoftObjectPtr<AActor> LockableActor;
   
   UPROPERTY(EditAnywhere)
   bool ShouldBeLocked = false;
};

// Checks if a toggle is on
//
// For containers, on=open
UCLASS(DisplayName = "Toggle is On/Open")
class TAT_API UTATTutorialCondition_ToggleIsOn : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   // The spawner for the guard to check the tag on
   UPROPERTY(EditAnywhere, meta=(AllowedClasses="/Script/OSEInteraction.OSEToggleInterface"))
   TSoftObjectPtr<AActor> ToggleableActor;
   
   UPROPERTY(EditAnywhere)
   bool ShouldBeOn = true;
};

UCLASS(DisplayName = "Door is Open")
class TAT_API UTATTutorialCondition_DoorIsOpen : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const override;
#endif

public:
   // The spawner for the guard to check the tag on
   UPROPERTY(EditAnywhere)
   TSoftObjectPtr<ATATSwingingDoor> Door;
   
   UPROPERTY(EditAnywhere)
   bool ShouldBeOpen = true;
};

UCLASS(DisplayName = "Player Has Escaped")
class TAT_API UTATTutorialCondition_PlayerHasEscaped : public UTATTutorialCondition
{
   GENERATED_BODY()

   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;
};


UCLASS(DisplayName = "Contract In State")
class TAT_API UTATTutorialCondition_ContractInState : public UTATTutorialCondition
{
   GENERATED_BODY()

public:
   virtual bool IsMet(const FTATTutorialParams& params) const override;
   virtual FString GetDebugName() const override;

   UPROPERTY(EditAnywhere, meta=(Categories="Contract"))
   FGameplayTag Contract;

   UPROPERTY(EditAnywhere)
   ETATContractState RequiredState;
};
