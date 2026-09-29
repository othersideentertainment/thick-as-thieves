// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSECharacterDetectionData.h"
#include "AI/Alertness/DetectionEnums.h"
#include "Components/ActorComponent.h"
#include "OSEDetectionComponent.generated.h"

DECLARE_DELEGATE_TwoParams(FOnLocalPlayerDetectionValueChanged, float, float);
UCLASS()
class OSEAI_API UOSEDetectionComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSEDetectionComponent();
   static bool _IsActorLocalCharacter(const AActor* actor);

   /// Get our detection value for the given player actor. Returns -1 if the player is not being detected.
   UFUNCTION(BlueprintCallable, Category = "AI|Detection")
   float GetDetectionValueForPlayer(const AActor* actor) const;
   /// Get our detection value for the given player actor.
   UFUNCTION(BlueprintCallable, Category = "AI|Detection")
   EActorDetectionState GetDetectionStateForPlayer(const AActor* actor) const;
   /// Get our highest detection value, sets params to -1/nullptr if none found
   UFUNCTION(BlueprintCallable, Category = "AI|Detection")
   bool GetHighestDetectionValue(float& highestDetectionValue, AActor*& highestDetectionActor) const;

   /// Get our visibility state for the given player actor.
   UFUNCTION(BlueprintCallable, Category = "AI|Visibility")
   void GetVisibilityStateForPlayer(const AActor* actor, bool& isVisible, float& timeSinceLastVisible) const;
   
   /// Assumed to be called by knowledge component when detection state changes
   void AuthorityUpdatePlayerActorDetectionValue(AActor* actor, EActorDetectionState state, float detectionValue);
   /// Assumed to be called by knowledge component when visibility state changes
   void AuthorityUpdatePlayerActorVisibility(AActor* actor, bool isVisible);

   void ClearDetectionEntries();

   FOnLocalPlayerDetectionValueChanged OnLocalPlayerDetectionValueChanged;
   
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
protected:
   
   // Called when a local player character's detection value changes
   virtual void _OnLocalPlayerDetectionValueChanged(float oldDetectionValue, float newDetectionValue);
   // Called when any player character's detection value changes
   virtual void _OnPlayerDetectionValueChanged(float oldDetectionValue, float newDetectionValue) { }
private:
   // detection
   FOSECharacterDetectionData& _FindOrCreatePlayerDetectionEntry(AActor* actor);
   void _ClearPlayerDetectionEntry(const AActor* actor);
   FOSECharacterDetectionData* _GetPlayerDetectionEntry(const AActor* actor);
   const FOSECharacterDetectionData* _GetPlayerDetectionEntry(const AActor* actor) const;

   UFUNCTION()
   void _OnDetectedPlayerActorEndPlay(AActor* actor, EEndPlayReason::Type endPlayReason);


   UPROPERTY(ReplicatedUsing = _OnRep_ActorDetectionEntries, Transient)
   TArray<FOSECharacterDetectionData> _actorDetectionEntries;
   UFUNCTION()
   void _OnRep_ActorDetectionEntries(const TArray<FOSECharacterDetectionData>& oldDetectionEntries);

};
