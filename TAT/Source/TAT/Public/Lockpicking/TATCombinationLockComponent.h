// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Lockpicking/TATAlternateLockInterface.h"
#include "Lockpicking/TATLockCombinationName.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "TATCombinationLockComponent.generated.h"

struct FTATLockCombinationName;
class UTATClueSetBase;

UENUM()
enum class ETATCombinationLockMode : uint8
{
   // Generates a unique combination based on the owning actor
   Unique,
   // All combination locks with this name will have the same random combination
   FromName
};

// An alternate lock that represents a four-digit combination lock
//
// The combination is currently generated deterministically via a random seed sans replication
//
// CONSIDER: The client has access to the combination at rest in order to
//           make the feedback on unlock attempts latency free (although
//           not trusted by the server). (and also simpler) Is this worth
//           the potential for exploitation?
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATCombinationLockComponent : public UActorComponent, public ITATAlternateLockInterface
{
   GENERATED_BODY()

public:
   UTATCombinationLockComponent();

protected:
   virtual void BeginPlay() override;

public:
#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif
   
   virtual bool TryAddToPrompt(FInteractPrompt& prompt, FLockInteractContext lockContext) const override;
   virtual bool TryHandleInteractStart(TScriptInterface<ILockpickableInterface> lockable,
      ACharacter* interactingCharacter, FLockInteractContext lockContext,
      FInteractStartResult& outResult) override;

   UFUNCTION(BlueprintPure)
   bool IsValidCombination(int32 combination) const;
   FText GetCombinationText() const;

#if WITH_EDITOR
   FName GetCombinationName() const { return (_combinationMode == ETATCombinationLockMode::FromName) ? _combinationName.Name : FName(); }
   bool ShouldEnforceUniqueName() const { return (_combinationMode == ETATCombinationLockMode::FromName) && !_allowedSharedName; }
#endif

private:
   int32 _GenerateSeed() const;
   
   int16 _combination = 0;

   // How the combination for the lock is generated
   UPROPERTY(EditAnywhere, Category = Combination)
   ETATCombinationLockMode _combinationMode = ETATCombinationLockMode::FromName;

   // Just a value to seed random generation of the combination. All combination
   // locks with the same name will have the same combination, but they will still
   // randomly vary between runs.
   UPROPERTY(EditAnywhere, Category = Combination, meta=(EditCondition = "_combinationMode == ETATCombinationLockMode::FromName", EditConditionHides))
   FTATLockCombinationName _combinationName;

#if WITH_EDITORONLY_DATA
   // If false, map-check will check for locks that use the same combination name
   // If you want multiple locks to share the same combination, just check it
   UPROPERTY(EditAnywhere, Category = Combination, meta=(EditCondition = "_combinationMode == ETATCombinationLockMode::FromName", EditConditionHides))
   bool _allowedSharedName = false;
#endif

   // A set of clues that get injected with the locks combination as {Combination}
   // (if you want to have multiple possible sets of clues, ask)
   UPROPERTY(EditAnywhere, Category = Clues)
   TSoftObjectPtr<UTATClueSetBase> _clueSet;

   // TODO: implement ClueLocationInterface in a second pass?
   // OPTIONAL: Clue location tag, only needed if it may want to use clue spawners that are limited to a given location
   // UPROPERTY(EditAnywhere, Category = Clues, meta=(Categories = "ClueLocation"))
   // FGameplayTag _clueLocationTag;

   // Scene requirement for generating a clue from this combination lock
   // NOTE: not currently hooking up editor vis for this, as there is a decent chance it would collide with that for spawners on the same actor
   UPROPERTY(EditAnywhere, Category = Clues)
   FTATSceneRequirement _clueSceneRequirement;
};
