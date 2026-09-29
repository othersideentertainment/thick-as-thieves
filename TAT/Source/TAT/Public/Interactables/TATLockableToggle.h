// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//tat
#include "Interactables/TATInteractableToggle.h"
#include "Interactables/TATLockConfig.h"
#include "Lockpicking/LockpickableInterface.h"
#include "Variation/TATSpawnerFwd.h"

//ue4
#include "CoreMinimal.h"

#include "TATLockableToggle.generated.h"

class UTATSpawnerComponent;

// TODO: if this front-back assumption does not fit non-window locked toggles, move to something window-specific?
UENUM(BlueprintType)
enum class EToggleLockDirection : uint8
{
   Both = 3,
   LockFrontOnly = 2, // front and back are arbitrary directions for lack of a better name
   LockBackOnly = 1,
   None = 0 UMETA(Hidden)
};

UENUM()
enum class ETATLockableToggleDirection : uint8
{
   Both = 3,
   FrontOnly = 2, // front and back are arbitrary directions for lack of a better name
   BackOnly = 1,
   None = 0
};

USTRUCT()
struct FTATLockableToggleAllowedDirections
{
   GENERATED_BODY()

public:
   // which directions the toggle can be re-locked from
   UPROPERTY(EditAnywhere, Category = "Lock", DisplayName = "Allowed Relock Direction")
   ETATLockableToggleDirection RelockDirection = ETATLockableToggleDirection::Both;

   // which direction the toggle can be lockpicked from
   UPROPERTY(EditAnywhere, Category = "Lock", DisplayName = "Allowed Lockpick Direction")
   ETATLockableToggleDirection LockpickableDirection = ETATLockableToggleDirection::Both;

   // which direction a key can be used from
   UPROPERTY(EditAnywhere, Category = "Lock", DisplayName = "Allowed Key Direction")
   ETATLockableToggleDirection KeyDirection = ETATLockableToggleDirection::Both;

   // which direction the toggle can be turned on (gates interaction when off)
   UPROPERTY(EditAnywhere, Category = "Lock")
   ETATLockableToggleDirection AllowedTurnOnDirection = ETATLockableToggleDirection::Both;
};

// An interactable toggle that can be locked (e.g. windows)
// NOTE: if windows need more specific stuff, make that a separate class
UCLASS()
class TAT_API ATATLockableToggle : public ATATInteractableToggle, public ILockpickableInterface
{
   GENERATED_BODY()
public:
   ATATLockableToggle();
   virtual void BeginPlay() override;

   UFUNCTION(BlueprintCallable)
   void SetLocked(bool newIsLocked);

   UFUNCTION(BlueprintCallable, BlueprintPure)
   FORCEINLINE bool IsLocked() const { return _locked; }

   // from AActor
   virtual void PostInitializeComponents() override;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

   // interactable interface
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;

   // lockpickable interface
   virtual void Unlock() override { SetLocked(false); }
   virtual void Lock() override { SetLocked(true); }
   virtual void LockWithKey() override { SetLocked(true); }
   virtual void OnLockpickTrackCompleted(int32 trackIndex) override;
   virtual int32 GetLockpickCurrentTrack() override { return _lockpickCurrentTrack; }

   virtual void ToggleForInteraction(ACharacter* interactingCharacter) override;

   UFUNCTION(BlueprintPure, Category = Door, meta = (BlueprintProtected = true))
   bool ShouldVisualizeLockInDirection(bool isFrontSide) const;

   void SetLockDirection(EToggleLockDirection lockDirection);

protected:
   // A configured spawner component can drive whether this actor is locked based on mission rules
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lock|Spawner")
   UTATSpawnerComponent* _lockedSpawnerComponent = nullptr;

   // Called when dropped by the local player.
   UFUNCTION(BlueprintImplementableEvent, Category = "Lock")
   void OnLockChanged(bool isLocked);

   UFUNCTION()
   void OnRep_Locked();

protected:
   virtual bool _TryPriorityInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual void _AddNormalInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;

   virtual bool _TryPriorityStartInteract(ACharacter* interactingCharacter, FInteractStartResult& outResult) override;
   virtual FInteractStartResult _NormalStartInteract(ACharacter* interactingCharacter) override;
   
   FLockInteractContext _MakeLockContext(ACharacter* interactingCharacter) const;
   virtual FTATLockableToggleAllowedDirections _GetAllowedDirections() const;
private:
   bool _AllowLockInState(bool isOn) const;
   bool _IsOnFrontSide(const FVector& characterPosition) const;
   bool _BlockedInDirection(const ACharacter* interactingCharacter) const;

   UFUNCTION()
   void _AuthorityOnLockedSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);
   UFUNCTION()
   void _AuthorityOnLockedSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);
   
private:
   UPROPERTY(EditAnywhere, Category = "Lock", meta=(DisplayAfter="_locked", DisplayName="Lock Config"))
   FTATLockConfig _lockConfig;

   UPROPERTY(Replicated, Transient)
   int32 _lockpickCurrentTrack = 0;
   
   UPROPERTY(ReplicatedUsing = OnRep_Locked, EditAnywhere, Category = "Lock", meta=(DisplayName = "Locked"))
   bool _locked;

   UPROPERTY(EditAnywhere, Category = "Lock", DisplayName = "Initial Lock Direction", Replicated)
   EToggleLockDirection _lockDirection = EToggleLockDirection::Both;

   UPROPERTY(EditDefaultsOnly, Category = "Lock", AdvancedDisplay)
   uint8 _allowLockWhenOn : 1;

   UPROPERTY(EditDefaultsOnly, Category = "Lock", AdvancedDisplay)
   uint8 _allowLockWhenOff : 1;

   UPROPERTY(EditAnywhere, Category = Interactable, AdvancedDisplay)
   FText _cannotTurnOnFromDirectionMessage;

protected:
   UPROPERTY(EditAnywhere, Category = "Lock", meta=(DisplayName = "Normal Allowed Directions"))
   FTATLockableToggleAllowedDirections _normalAllowedDirections;

   // Direction pointing in the front interactable for lock-side purposes
   UPROPERTY(EditDefaultsOnly, Category = "Lock", AdvancedDisplay, meta = (MakeEditWidget = true))
   FVector _frontDirection;

   // Offset from the actor origin that should be considered the center for the purposes of determining 
   // which side the player is on
   UPROPERTY(EditDefaultsOnly, Category = "Lock", AdvancedDisplay, meta = (MakeEditWidget = true))
   FVector _centerOffset = FVector(ForceInit);
};
