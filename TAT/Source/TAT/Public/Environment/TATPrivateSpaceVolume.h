// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "NavModifierVolume.h"
#include "GameFramework/Actor.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATPrivateSpaceVolume.generated.h"

class ITATPrivateSpaceCharacterInterface;
class UTATPrivateSpaceCharacterComponent;
class UTATSceneAsset;

UENUM()
enum class ETATPrivateSpaceType : uint8
{
   // If a character spawns in this area, they are allowed to be here. Anyone else will be told to leave the area if seen.
   PrivateArea,
   // If a character spawns in this area, they are allowed to be here. Anyone else is immediately hostile if seen.
   OffLimits,
   // If a character spawns in this area, they are allowed to be here. But anyone can be here
   PublicArea
};

/*
 * Private Space Volumes and TATAreaMarkupVolumes share _a lot_ of similar code.
 * I had wanted to pull that into a similar base class OR a component, but that proved to be a bigger job
 * and we're under time presure to deliver TOD, so this'll be backlogged as tech debt.
 */
UCLASS()
class TAT_API ATATPrivateSpaceVolume : public ANavModifierVolume
{
   GENERATED_BODY()
public:
   ATATPrivateSpaceVolume(const FObjectInitializer& objectInitializer);

   // From AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   
   void HandleInitialOverlaps();

   FGameplayTag GetPrivateZoneGameplayTag() const { return _privateZoneGameplayTag; }
protected:
   void _StartTrackActor(AActor* actor);
   virtual void NotifyActorBeginOverlap(AActor* otherActor) override;
   void _EndTrackActor(AActor* actor);
   virtual void NotifyActorEndOverlap(AActor* otherActor) override;
   void HandleActor(const TScriptInterface<ITATPrivateSpaceCharacterInterface>& characterInterface);

   void HandlePrivateSpaceActorDisguiseBegin(AActor* tatCharacter);
   void HandlePrivateSpaceActorDisguiseEnd(AActor* tatCharacter);
   void _HandleTemporaryAllowedSpaceChanged(FGameplayTag spaceTag, AActor* actor);

   UFUNCTION()
   void _OnWorldBegunPlay();

   void _AddActorToVolume(const TScriptInterface<ITATPrivateSpaceCharacterInterface>& privateSpaceCharacterInterface);
   void _RemoveActorFromVolume(const TScriptInterface<ITATPrivateSpaceCharacterInterface>& privateSpaceCharacterInterface);

   void _ApplySpacePrivacySettings(UTATPrivateSpaceCharacterComponent* privateSpaceCharacterComponent);
   void _RemoveSpacePrivacySettings(UTATPrivateSpaceCharacterComponent* privateSpaceCharacterComponent);

   bool _ShouldAddActorToVolume(const TScriptInterface<ITATPrivateSpaceCharacterInterface>& privateSpaceCharacterInterface) const;
   UFUNCTION()
   void _HandleMapStateChanged(const ETATMapVariationLoadingState currentState);
   void _WaitForWorldBegunPlayOrTrigger();

   ETATPrivateSpaceType GetSpaceType() const { return _spaceType; }

   UPROPERTY(EditAnywhere, Category = "Private Space Settings")
   ETATPrivateSpaceType _spaceType = ETATPrivateSpaceType::PrivateArea;

   UPROPERTY(EditAnywhere, Category="Private Space Settings")
   FGameplayTag _privateZoneGameplayTag;

   // If set, it will try to pull the space Type from the active scene
   UPROPERTY(EditInstanceOnly, Category = "Private Space Settings")
   TObjectPtr<UTATSceneAsset> _overrideScene;


private:
   bool _isReadyToProcessActorOverlaps { false };
   
   UPROPERTY(Transient)
   TArray<TScriptInterface<ITATPrivateSpaceCharacterInterface>> _actorsWhoEnteredIntoVolume;
};
