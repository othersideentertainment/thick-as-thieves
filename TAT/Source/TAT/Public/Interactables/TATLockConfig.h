// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat
#include "TATParameterizedTextCache.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATLockConfig.generated.h"

class ACharacter;
class UTATItemInfo;
class ILockpickableInterface;
class ITATAlternateLockInterface;

struct FInteractPrompt;
struct FInteractStartResult;

USTRUCT(BlueprintType)
struct FLockInteractContext
{
   GENERATED_BODY()
   // Is it actually locked?
   bool bIsLocked = false;
   
   // Is it in a state where being locked is relevant (e.g. not an open door)
   bool bIsLockRelevant = false;
   
   // Is a key specified that could be used to unlock?
   bool bAllowsKey = false;
   
   // Does the interacting character have a relevant key?
   bool bHasKey = false;
   
   // Can it be manually locked by the character in the direction they are in?
   bool bCanBeRelockedInCurrentDirection = false;
   
   // Is it locked for the interacting character in the direction they are in?
   bool bIsLockedInCurrentDirection = false;
   
   // does the interactor have the ability to lockpick (e.g. has a lockpick component)
   bool bCanInteractorLockpick = false;

   // Can lock be picked in the current direction?
   bool bCanBePickedInCurrentDirection = false;

   // does the object have another way to get into it if bIsLockedInCurrentDirection is true?
   bool bAreAllSidesLocked = false;

   FORCEINLINE bool CanLockpick() const { return bIsLockedInCurrentDirection && !bHasKey && bCanInteractorLockpick && bCanBePickedInCurrentDirection; }
   FORCEINLINE bool CanUnlockWithKey() const { return bIsLockedInCurrentDirection && bHasKey; }
   FORCEINLINE bool RequiresKey() const { return bIsLockedInCurrentDirection && !bCanInteractorLockpick && bAllowsKey && !bHasKey ; }
   FORCEINLINE bool CanLockWithKey() const { return bHasKey && bIsLockRelevant && bCanBeRelockedInCurrentDirection && !bIsLocked; }
   FORCEINLINE bool CanLockWithoutKey() const { return !bHasKey && bIsLockRelevant && bCanBeRelockedInCurrentDirection && !bIsLocked; }
};

USTRUCT(BlueprintType)
struct TAT_API FTATLockConfig
{
   GENERATED_BODY()

   // The lock can be any variation from this value to MaxLockLevel
   UPROPERTY(EditAnywhere, Meta = (ClampMin = 1, ClampMax = 6, UIMin = 1, UIMax = 6))
   uint8 MinLockLevel = 1;
   
   // The lock can be any variation from MinLockLevel to this value
   UPROPERTY(EditAnywhere, Meta = (ClampMin = 1, ClampMax = 6, UIMin = 1, UIMax = 6))
   uint8 MaxLockLevel = 1;

   UPROPERTY(EditAnywhere)
   bool CanBeLockpicked = true;

   // The tag of the key that unlocks this lock
   UPROPERTY(EditAnywhere, Meta = (Categories = "Item.Key"))
   FGameplayTag KeyTag;

   // The item info for the key to display name in UI (TBD replace KeyTag above [dmcdonough 042424])
   UPROPERTY(EditAnywhere, Meta = (Categories = "Item.Key"))
   TSubclassOf<UTATItemInfo> KeyItemInfo;

   // Tags of that should show an alternate message if you have them, but not the right key
   // It is fine to also include the correct key here as well
   //! TODO: nice to have an edit-condition so you can only see them if it requires a key
   UPROPERTY(EditAnywhere, Meta = (Categories = "Item.Key", EditCondition = "!CanBeLockpicked", EditConditionHides))
   FGameplayTagContainer IncorrectKeyTags;

   // Message shown if the player does not have the right key, but does have one of the configured incorrect keys
   UPROPERTY(EditAnywhere, meta = (EditCondition = "!CanBeLockpicked", EditConditionHides))
   FText IncorrectKeyMessage;

   UPROPERTY(EditDefaultsOnly, Meta = (Categories = "UI"))
   FGameplayTag LockedInteractStatusTag;

   UPROPERTY(EditDefaultsOnly, Meta = (Categories = "UI"))
   FGameplayTag UnlockedInteractStatusTag;

   // An alternate lock to delegate behavior to
   UPROPERTY(Transient)
   TScriptInterface<ITATAlternateLockInterface> AlternateLock;

   void RandomizeLockLevel(const AActor* actor);
   int GetFinalLockLevel() const;

   static bool CanActorLockpick(const AActor* actor);

   bool DoesCharacterHaveKey(const ACharacter* character) const;
   bool DoesCharacterHaveIncorrectKey(const ACharacter* character) const;

   void AddToPrompt(FInteractPrompt& prompt, FLockInteractContext lockContext, const ACharacter* character) const;

   bool TryHandleInteractStart(
      TScriptInterface<ILockpickableInterface> lockable,
      ACharacter* interactingCharacter,
      const FLockInteractContext& lockContext,
      FInteractStartResult& outResult) const;
   
   void HandleInteractComplete(
      TScriptInterface<ILockpickableInterface> lockable,
      ACharacter* interactingCharacter,
      const FLockInteractContext& lockContext) const;
   
   void StartLockpicking(
      ACharacter* character,
      TScriptInterface<ILockpickableInterface> target) const;

   // To be called in post-initialize-components
   void InitializeAlternateLock(const AActor* owningActor);

#if WITH_EDITOR
   void CheckForErrors(TFunctionRef<void (FText)> reportError) const;
#endif

private:
   FTATSimpleTextCache _lockLevelPromptCache;

   // The initialized level used during play. This is either a default value or the result of randomizin between 1 and MaxLockLevel.
   int _finalLockLevel = 1;
   bool _isInitialized = false;
};

UCLASS(meta=(ScriptName="LockConfigLibrary"))
class ULockConfigBlueprintLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "Lock Config|Keys")
   static bool CanUnlockWithKey(const FLockInteractContext& context){ return context.CanUnlockWithKey(); };
   UFUNCTION(BlueprintPure, Category = "Lock Config|Keys")
   static bool CanLockWithKey(const FLockInteractContext& context){ return context.CanLockWithKey(); };
   UFUNCTION(BlueprintPure, Category = "Lock Config")
   static bool IsLocked(const FLockInteractContext& context){ return context.bIsLocked; };
   UFUNCTION(BlueprintPure, Category = "Lock Config")
   static bool IsLockedInCurrentDirection(const FLockInteractContext& context){ return context.bIsLockedInCurrentDirection; };
};
