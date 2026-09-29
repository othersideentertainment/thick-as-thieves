// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "OSEAbilityFunctionLibrary.h"
#include "OSESyncedAnimations.h"
#include "Abilities/OSEGameplayAbility.h"

#include "Engine/DataAsset.h"

#include "OSEGameplayAbility_SyncedAnimationPlayer.generated.h"

//////////////////////////////////////////////////////////////////////////
///                  UOSESyncedAnimationDataAsset
//////////////////////////////////////////////////////////////////////////

// Asset that holds the synced animation data struct so we can pass it around in gameplay ability payloads
// This is an asset on disc so we can reference it over the network
UCLASS(BlueprintType)
class OSECORE_API UOSESyncedAnimationDataAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(TitleProperty=AnimationType))
   FSyncedAnimationEntry SyncedAnimationData;

   UFUNCTION(BlueprintPure)
   bool MeetsConstraints(AActor* sourceCharacter, AActor* targetCharacter, bool requiresTarget) const;
};

//////////////////////////////////////////////////////////////////////////
///                  FOSESyncedAnimationsTargetDataFilter
//////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct OSECORE_API FOSESyncedAnimationsTargetDataFilter : public FOSETeamAttitudeTargetDataFilter
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Filter)
   TArray<UOSESyncedAnimationDataAsset*> SyncedAnimations;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Filter)
   bool RequiresTarget = true;

   // Returns true if the actor passes the filter and will be targeted
   virtual bool FilterPassesForActor(const AActor* actorToBeFiltered) const override;
};

//////////////////////////////////////////////////////////////////////////
///                  UOSESyncedAnimationsTargetDataFilterFunctionLibrary
//////////////////////////////////////////////////////////////////////////

UCLASS()
class OSECORE_API UOSESyncedAnimationsTargetDataFilterFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   // Make the handle to use our synced animation filter w/ a targeting ability
   UFUNCTION(BlueprintPure, Category = "Animation|Synced|OSE")
   static FGameplayTargetDataFilterHandle MakeSyncedAnimationTargetDataFilter(const FOSESyncedAnimationsTargetDataFilter& filter, AActor* filterActor);
};

//////////////////////////////////////////////////////////////////////////
///                  UOSEGameplayAbility_SyncedAnimationPlayer
//////////////////////////////////////////////////////////////////////////

// Base class for playing back synced animations
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_SyncedAnimationPlayer : public UOSEGameplayAbility
{
   GENERATED_BODY()

public:
   UOSEGameplayAbility_SyncedAnimationPlayer();

   // from UGameplayAbility
   virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* actorInfo, const FGameplayEventData* payload) const override;
};
