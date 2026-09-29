// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "InteractableInterface.h"

// ue5
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSEInteractionHelpers.generated.h"

class IInteractableInterface;
class UTimelineComponent;

struct FGameplayEventData;
struct FInteractPrompt;

USTRUCT(BlueprintType)
struct OSEINTERACTION_API FOSEToggleState
{
   GENERATED_BODY()

public:
   // Server time of the last time that the toggle was changed
   UPROPERTY(Transient)
   float ChangedServerTime = 0;

   UPROPERTY(EditAnywhere)
   bool bIsOn = false;

   bool IsOld(const UObject* WorldContext, float ThresholdSeconds) const;
};

// not a ustruct for now
struct FCharacterInteractHoldInfo
{
   bool bHasHold;
   float HoldDuration;
   FGameplayTag HoldAnimation;
   TSubclassOf<UGameplayEffect> HoldTargetEffect;
   const UGameplayAbility* Ability = nullptr;
};

struct FCharacterInteractInstantAnimationInfo
{
   bool bHasInstantAnimation;
   FGameplayTag InstantAnimation;
};


// Target data for just interactables
USTRUCT()
struct OSEINTERACTION_API FGameplayAbilityTargetData_Interactable : public FGameplayAbilityTargetData
{
   GENERATED_USTRUCT_BODY()

   UPROPERTY()
   TWeakObjectPtr<UObject> Interactable;

   virtual UScriptStruct* GetScriptStruct() const override
   {
      return FGameplayAbilityTargetData_Interactable::StaticStruct();
   }

   virtual FString ToString() const override
   {
      return TEXT("FGameplayAbilityTargetData_Interactable");
   }

   bool NetSerialize(FArchive& ar, class UPackageMap* map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_Interactable> : public TStructOpsTypeTraitsBase2<FGameplayAbilityTargetData_Interactable>
{
   enum
   {
      WithNetSerializer = true
   };
};

// Helper methods for interactable toggle
// TODO: Most of this is for the toggle as opposed to the interaction. Should I rename this?
UCLASS()
class OSEINTERACTION_API UOSEInteractionHelpers : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   static const FName kInteractTag;
   static const FName kInteractNoHighlightTag;

   // Helper accessor for GetServerWorldTimeSeconds on GameState
   static float GetServerTimeForWrite(const UObject* worldContext);
   static float GetServerTimeForComparison(const UObject* worldContext);

   static bool IsOld(const UObject* worldContext, float serverTimestamp, float threshold = 0.75f);

   // Synchronizes the playback state of the timeline with the toggle state
   UFUNCTION(BlueprintCallable, Category="Interactable|Toggle")
   static void SyncTimelineWithToggle(const FOSEToggleState& state, UTimelineComponent* timeline);

   // Checks if the timeline has stopped playing, with fudge on the server to tolerate some time discrepancies
   UFUNCTION(BlueprintPure, Category = "Interactable|Toggle")
   static bool IsTimelineCompleteForTransition(UTimelineComponent* timeline);

   static TScriptInterface<IInteractableInterface> CastForInteractableHits(ACharacter* interactingCharacter, FVector startPoint, FVector endPoint, FHitResult& hitResult, const FCollisionResponseParams& responseParams = FCollisionResponseParams::DefaultResponseParam);

   /** Do a physics line trace looking for interactable objects */
   static TScriptInterface<IInteractableInterface> CastForInteractables(ACharacter* interactingCharacter, FVector startPoint, FVector endPoint, const FCollisionResponseParams& responseParams = FCollisionResponseParams::DefaultResponseParam);

   static bool SphereOverlapInteractables(ACharacter* interactingCharacter, const FVector spherePos, const float sphereRadius, TArray<TScriptInterface<IInteractableInterface>>* outInteractables = nullptr, TArray<FOverlapResult>* outOverlapResults = nullptr, const FCollisionResponseParams& responseParams = FCollisionResponseParams::DefaultResponseParam);

   /** Looks inside hit result for an interactable object */
   UFUNCTION(BlueprintCallable, Category = Interactable)
   static TScriptInterface<IInteractableInterface> GetInteractableFromHit(ACharacter* interactingCharacter, const FHitResult& hit);

   /** Gets an actor from an interactable object, which may be a component */
   UFUNCTION(BlueprintCallable, Category = Interactable)
   static AActor* GetActorForInteractable(TScriptInterface<IInteractableInterface> taggable);

   // Get whether a component can count as part of an interactable that can be interacted with
   static bool IsComponentTargetableForInteraction(USceneComponent* component);

   // @TODO: Actually implementing this needs a reference point
   // UFUNCTION(BlueprintCallable, Category = Interactable)
   static TScriptInterface<IInteractableInterface> SelectNearestInteractableFromArray(ACharacter* interactingCharacter, const TArray<FHitResult>& interactablesArray);

   /** Builds target data that specifies the interactable object as the only target */
   UFUNCTION(BlueprintCallable, Category = "Interactable|Target")
   static FGameplayAbilityTargetDataHandle MakeTargetDataFromInteractable(TScriptInterface<IInteractableInterface> interactable);

   /** Gets interactable object from a target data */
   UFUNCTION(BlueprintCallable, Category = "Interactable|Target")
   static TScriptInterface<IInteractableInterface> GetInteractableFromTargetData(const FGameplayAbilityTargetDataHandle& targetData);

   UFUNCTION(BlueprintCallable, Category = "Interactable|End Context")
   static bool IsEndContextProbablyInstant(const FInteractEndContext& endContext) { return endContext.IsProbablyInstant(); };

   UFUNCTION(BlueprintCallable, Category = "Interactable|End Context")
   static bool IsEndContextComplete(const FInteractEndContext& endContext) { return endContext.IsComplete(); };

   static FText GetPromptForAbility(const UGameplayAbility* ability);
   static FGameplayTag GetPromptActionTagForAbility(const UGameplayAbility* ability);
   static bool HasCharacterInteractionAbility(ACharacter* interactor, const ACharacter* target);
   static bool HasHoldCharacterInteraction(ACharacter* interactor, const ACharacter* target);
   static FCharacterInteractHoldInfo GetHoldInfoForCharacterInteraction(ACharacter* interactor, const ACharacter* target, float defaultHoldDuration);
   static void GetPromptForCharacterInteraction(ACharacter* interactor, const ACharacter* target, FInteractPrompt& outPrompt);
   static bool TriggerAbilityForCharacterInteraction(ACharacter* interactor, const ACharacter* target, bool hold);
   static FCharacterInteractInstantAnimationInfo GetInstantAnimationForCharacterInteraction(ACharacter* interactor, const ACharacter* target);
};
