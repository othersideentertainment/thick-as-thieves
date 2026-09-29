// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "OSEAbilitySystemComponent.h"
#include "OSEAbilityTypes.h"
#include "Character/OSETeamInterface.h"

// ue4
#include "Abilities/GameplayAbilityTargetDataFilter.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/CollisionProfile.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSEAbilityFunctionLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSEAbilityFunctionLibrary, Log, All);

class UAbilitySystemComponent;
class UOSEAbilityMetadata;
struct FOSEAbilityInfo;
struct FOSEActorsWithAppliedEffectsSet;

//////////////////////////////////////////////////////////////////////////
///            FOSETagGameplayTargetDataFilter
//////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct OSECORE_API FOSETagGameplayTargetDataFilter : public FGameplayTargetDataFilter
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Filter)
   FGameplayTagContainer PawnTagsToMatch;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Filter)
   FGameplayTagContainer PawnTagsToExclude;

   // Returns true if the actor passes the filter and will be targeted
   virtual bool FilterPassesForActor(const AActor* actorToBeFiltered) const override;
};

//////////////////////////////////////////////////////////////////////////
///            FOSETeamAttitudeTargetDataFilter
//////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct OSECORE_API FOSETeamAttitudeTargetDataFilter : public FOSETagGameplayTargetDataFilter
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Filter)
   bool RequireTargetAttitude = true;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true, EditCondition = "RequireTargetAttitude"), Category = Filter)
   EOSETeamAttitude TargetAttitude = EOSETeamAttitude::Neutral;

   // Returns true if the actor passes the filter and will be targeted
   virtual bool FilterPassesForActor(const AActor* actorToBeFiltered) const override;
};

//////////////////////////////////////////////////////////////////////////
///            FOSEEffectWithSetByCallerTag
//////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct OSECORE_API FOSEEffectWithSetByCallerTag
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TSoftClassPtr<UGameplayEffect> GameplayEffectClass; // soft instead of subclassof because we're going to use this in settings classes

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag SetByCallerMagnitudeTag;

   void ApplyEffectWithMagnitude(UAbilitySystemComponent* asc, float magnitude) const;
};

//////////////////////////////////////////////////////////////////////////
///            FOSEEffectWithSetByCallerTag
//////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct OSECORE_API FOSEEffectWithSetByCallerTagAndMagnitude
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TSoftClassPtr<UGameplayEffect> GameplayEffectClass; // soft instead of subclassof because we're going to use this in settings classes

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag SetByCallerMagnitudeTag;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float Magnitude = 0.0f;

   void ApplyEffect(UAbilitySystemComponent* asc) const;
};

//////////////////////////////////////////////////////////////////////////
///            UOSEAbilityFunctionLibrary
//////////////////////////////////////////////////////////////////////////

UCLASS()
class OSECORE_API UOSEAbilityFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category="Ability|OSE")
   static void CancelAbilitiesWithTags(UAbilitySystemComponent* abilitySystem, const FGameplayTagContainer& withTags);
   
   /** Returns tags associated with the current GameplayEvent, or empty container if handle does not support this.  
       Note this is a container; the GameplayEvent can have multiple descriptive tags this way in addition to its EventTag. */
   UFUNCTION(BlueprintPure, Category = "Ability|EffectContext|OSE")
   static FGameplayTagContainer SupplementalTagsFromEffectContext(FGameplayEffectContextHandle context);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   static TArray<FActiveGameplayEffectHandle> GetActiveEffectsByClass(UAbilitySystemComponent* abilitySystem, TSubclassOf<UGameplayEffect> effectClass);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   static FGameplayEffectContextHandle GetEffectContextForActiveEffect(FActiveGameplayEffectHandle effectHandle);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   static float GetLevelForActiveEffect(FActiveGameplayEffectHandle effectHandle);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   static int GetStackCountByGameplayEffectAssetTag(UAbilitySystemComponent* abilitySystem, FGameplayTag assetTag);

   // returns the instance, not the CDO
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   static UGameplayAbility* FindFirstMatchingAbilityInstance(UAbilitySystemComponent* abilitySystem, const FGameplayTagContainer& tagsToMatch);

   // Finds the first gameplay tag in the container, or empty
   UFUNCTION(BlueprintPure, Category = "Gameplay Tags|OSE")
   static FGameplayTag GetFirstTagInContainer(const FGameplayTagContainer& tagContainer);

   // Allows GameCode to add loose gameplaytags which are not backed by a GameplayEffect.
   // Tags added this way are not replicated!
   // It is up to the calling GameCode to make sure these tags are added on clients/server where necessary
   UFUNCTION(BlueprintCallable, Category = "Gameplay Tags|OSE")
   static void AddLooseGameplayTag(UAbilitySystemComponent* abilitySystem, FGameplayTag gameplayTag);

   UFUNCTION(BlueprintCallable, Category = "Gameplay Tags|OSE")
   static void AddLooseGameplayTags(UAbilitySystemComponent* abilitySystem, FGameplayTagContainer gameplayTags);
   
   UFUNCTION(BlueprintCallable, Category = "Gameplay Tags|OSE")
   static void RemoveLooseGameplayTag(UAbilitySystemComponent* abilitySystem, FGameplayTag gameplayTag);
   
   UFUNCTION(BlueprintCallable, Category = "Gameplay Tags|OSE")
   static void RemoveLooseGameplayTags(UAbilitySystemComponent* abilitySystem, FGameplayTagContainer gameplayTags);

   UFUNCTION(BlueprintCallable, Category = "Gameplay Tags|OSE")
   static void SetLooseGameplayTagCount(UAbilitySystemComponent* abilitySystem, FGameplayTag gameplayTag, int count);

   UFUNCTION(BlueprintPure, Category = "Gameplay Tags|OSE")
   static FGameplayTag RequestGameplayTag(const FName& gameplayTagName);

   /// Returns the tag in container that is a child of the specified parent.
   UFUNCTION(BlueprintPure, Category = "OSE")
   static FGameplayTag FindSingleChildTag(const FGameplayTagContainer& container, FGameplayTag parent);

   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   static bool IsAbilityActive(UAbilitySystemComponent* abilitySystem, TSubclassOf<UGameplayAbility> inAbilityClass);
   
   /** 
    * Performs a targeting line trace based on where the character is looking, works for both players and AI on client and server
    *
    * @return               True if OutHitResult was set to a blocking hit
    * @param SourceActor    What actor to use as the source, should be a pawn or controller/playerstate
    * @param StartLocation  Ability targeting location to use for trace start, if set to default or source actor will be ignored and use viewpoint
    * @param MaxRange       How far out to target from trace start
    * @param TraceProfile   What collision channel to use
    * @param TargetFilter   Target filter to apply when looking for valid targets
    * @param Debug          If true, will draw debug visualizations of targeting
    */
   UFUNCTION(BlueprintCallable, Category = "Ability|Targeting|OSE")
   static bool PerformTargetTrace(FHitResult& outHitResult, const AActor* sourceActor, FGameplayAbilityTargetingLocationInfo startLocation, float maxRange, FCollisionProfileName traceProfile, FGameplayTargetDataFilterHandle targetFilter, bool debug);

   /**
    * Returns the start/end points of a line that can be used to trace abilities from the camera point of view, starting at the physical player.  Useful maintaining
    * correct trace behavior when using both the 1p and 3p camera.
    *
    * @return               True if outStartPos and outEndPos were set
    * @param SourceActor    What actor to use as the source, should be a pawn or controller/playerstate
    * @param StartLocation  Ability targeting location to use for position start, if set to default or source actor will be ignored and use viewpoint
    * @param MaxRange       How far out to target from the starting point
    */
   UFUNCTION(BlueprintPure, Category = "Math|Vector|OSE")
   static bool OffsetCameraAimToPhysicalAim(const AActor* sourceActor, FGameplayAbilityTargetingLocationInfo startLocation, float maxRange, FVector& outStartPos, FVector& outEndPos);

   /**
    * Returns the start point and directional vector that can be used to trace abilities from the camera point of view, starting at the physical player.  Useful maintaining
    * correct trace behavior when using both the 1p and 3p camera. Limited version of OffsetCameraAimToPhysicalAim for when the aim vector needs to be manipulated
    *
    * @return               True if outStartPos and outAimVec were set
    * @param SourceActor    What actor to use as the source, should be a pawn or controller/playerstate
    */
   UFUNCTION(BlueprintPure, Category = "Math|Vector|OSE")
   static bool GetPhysicalAim(const AActor* sourceActor, FVector& outStartPos, FVector& outAimVec);


   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   static bool IsAbilityActorInfoLocallyControlled(const FGameplayAbilityActorInfo& actorInfo);

   /**
    * Returns the pos/rot of a point in space that lines up with the avatar view, useful for positioning melee attacks in front of the player
    *
    * @return               True if outPos and outRot were set
    * @param SourceActor    What actor to use as the source, should be a pawn or controller/playerstate
    * @param StartLocation  Ability targeting location to use for position start, if set to default or source actor will be ignored and use viewpoint
   */
   UFUNCTION(BlueprintPure, Category = "Math|Vector|OSE")
   static bool OffsetCameraAimToAvatarAim(const AActor* sourceActor, FGameplayAbilityTargetingLocationInfo startLocation, FVector& outPos, FRotator& outRot);

   /** 
    * Performs a targeting line trace between two specific points,
    *
    * @return                 True if OutHitResult was set to a blocking hit
    * @param SourceActor      What actor trace originates from, can be any actor
    * @param SourcePoint      Where trace starts
    * @param TargetPoint      Where trace ends
    * @param TraceProfile     What collision channel to use
    * @param TargetFilter     Target filter to apply when looking for valid targets
    * @param Debug            If true, will draw debug visualizations of targeting
    */
   UFUNCTION(BlueprintCallable, Category = "Ability|Targeting|OSE")
   static bool PerformTargetTraceToPoint(FHitResult& outHitResult, const AActor* sourceActor, FVector sourcePoint, FVector targetPoint, FCollisionProfileName traceProfile, FGameplayTargetDataFilterHandle targetFilter, bool debug);

   UFUNCTION(BlueprintCallable, Category = "Ability|Targeting|OSE")
   static bool PerformTargetSweepToPoint(FHitResult& outHitResult, const AActor* sourceActor, FVector sourcePoint, FVector targetPoint, FCollisionProfileName traceProfile, float radius, FGameplayTargetDataFilterHandle targetFilter, bool debug);


   /** Traces as normal, but will manually filter all hit actors and optionally treat overlaps like blocking */
   static void LineTraceWithFilter(FHitResult& outHitResult, const UWorld* world, const FGameplayTargetDataFilterHandle filterHandle, const FVector& start, const FVector& end, FName profileName, const FCollisionQueryParams params, bool treatOverlapAsBlocking);

   /** Sweeps as normal, but will manually filter all hit actors and optionally treat overlaps like blocking */
   static void SweepWithFilter(FHitResult& outHitResult, const UWorld* world, const FGameplayTargetDataFilterHandle filterHandle, const FVector& start, const FVector& end, const FQuat& rotation, const FCollisionShape collisionShape, FName profileName, const FCollisionQueryParams params, bool treatOverlapAsBlocking);

   /** Returns the character that initiated the gameplay ability/effect tracked by this handle */
   static class AOSECharacterBase* GetInstigatorCharacterFromContext(FGameplayEffectContextHandle effectContext);

   /** Resets an actor's spawn parameters to default, used for actor pooling */
   static void ResetSpawnParameters(AActor* actorToReset);

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|OSE", meta = (DeterminesOutputType = "abilityMetadataClass"))
   static const UOSEAbilityMetadata* GetAbilityMetadataByClass(const FOSEAbilityInfo& abilityInfo, const TSubclassOf<UOSEAbilityMetadata> abilityMetadataClass);

   // NOTE: This uses DynamicAbilityTags, which is probably not what you would expect (is this in use?)
   // Use with caution
   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability|OSE", meta=(DeprecatedFunction))
   static const UGameplayAbility* GetAbilityByTag(const UAbilitySystemComponent* target, const FGameplayTag& tag);
   
   /* If Ability was granted by a GameplayEffect, get the set-by-caller magnitude associated with the given tag (if any). Otherwise, give DefaultValue.  */
   UFUNCTION(BlueprintPure, Category = "Ability|Gameplay Effect", meta = (HidePin = "Ability", DefaultToSelf = "Ability"))
   static float GetGrantedByEffectSetByCallerMagnitude(const UGameplayAbility* ability, FGameplayTag tag, float defaultValue);

   // If Ability was granted by a GameplayEffect, get the level.  Otherwise, return defaultValue.  */
   UFUNCTION(BlueprintPure, Category = "Ability|Gameplay Effect", meta = (HidePin = "Ability", DefaultToSelf = "Ability"))
   static float GetGrantedByEffectLevel(const UGameplayAbility* ability, float defaultValue);

   UFUNCTION(BlueprintPure, Category = "Ability|Gameplay Effect")
   static float GetGameplayEffectDurationByClass(const AActor* actor, TSubclassOf<UGameplayEffect> gameplayEffect);

   // Allows querying the magnitude of an effect's modifier for a specified attribute, as it was entered in data. NOTE: Only applies to modifiers using ScalableFloat or any other type that can return data without context
   UFUNCTION(BlueprintPure, Category = "Ability|Gameplay Effect")
   static float GetEffectAttributeModifierMagnitudeByClass(TSubclassOf<UGameplayEffect> gameplayEffectClass, FGameplayAttribute attribute, bool& found, float level = 1.f);

   /// Returns the duration of an effect with the specified asset tag, otherwise -1
   UFUNCTION(BlueprintPure, Category = "Ability|Gameplay Effect")
   static float GetGameplayEffectDurationByEffectTag(const AActor* actor, FGameplayTag tag);

   /// Returns the period of this effect spec handle
   UFUNCTION(BlueprintPure, Category = "Ability|Gameplay Effect")
   static float GetGameplayEffectPeriodFromSpecHandle(const FGameplayEffectSpecHandle& specHandle);

   // Get owned gameplay tags from this actor, resets tagContainer
   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   static void GetOwnedGameplayTagsFromActor(const AActor* actor, FGameplayTagContainer& tagContainer);

   // Get a list of gameplay tags from a gameplay tag query; useful to listening for all changes within a query
   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   static FGameplayTagContainer GetGameplayTagsFromQuery(const FGameplayTagQuery& tagQuery);

   // Make the handle to use our gameplay tag filter w/ a targeting ability
   UFUNCTION(BlueprintPure, Category = "Filter")
   static FGameplayTargetDataFilterHandle MakeOSETagGameplayTargetDataFilter(const FOSETagGameplayTargetDataFilter& filter, AActor* filterActor);

   // Make the handle to use our team attitude filter w/ a targeting ability
   UFUNCTION(BlueprintPure, Category = "Filter")
   static FGameplayTargetDataFilterHandle MakeOSETeamAttitudeTargetDataFilter(const FOSETeamAttitudeTargetDataFilter& filter, AActor* filterActor);

   UFUNCTION(BlueprintPure, Category = "Filter")
   static bool GameplayTargetDataFilterPassesForActor(const FGameplayTargetDataFilterHandle& handle, AActor* actorToFilter);

   UFUNCTION(BlueprintPure, Category = "Filter")
   static bool IsFriendlyToActor(const AActor* sourceActor, const AActor* targetActor);

   UFUNCTION(BlueprintPure, Category = "Filter")
   static bool IsHostileToActor(const AActor* sourceActor, const AActor* targetActor);

   UFUNCTION(BlueprintPure, Category = "Filter")
   static bool IsNeutralToActor(const AActor* sourceActor, const AActor* targetActor);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta = (WorldContext="worldContext"))
   static bool FindSphereTeleportSpot(UObject* worldContext, const FVector& location, float radius, FName collisionProfile, FVector& outAdjustedLocation);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   static void ApplyEffectWithSetByCallerTag(AActor* actor, const FOSEEffectWithSetByCallerTag& effectInfo, float magnitude);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   static void ApplyEffectWithSetByCallerTagAndMagnitude(AActor* actor, const FOSEEffectWithSetByCallerTagAndMagnitude& effectInfo);

   /// apply a gameplay cue to an arbitrary actor
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta = (GameplayTagFilter = "GameplayCue"))
   static void AddGameplayCueToActor(AActor* actor, FGameplayTag gameplayCueTag);

   /// apply a gameplay cue to an arbitrary actor
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta = (GameplayTagFilter = "GameplayCue"))
   static void AddGameplayCueToActorWithParams(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameter);

   /// apply a local-only gameplay cue to an actor w/o replicating it
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta = (GameplayTagFilter = "GameplayCue"))
   static void AddNonReplicatedGameplayCueToActorWithParams(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters);

   /// remove a local-only gameplay cue to an actor w/o replicating it
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta = (GameplayTagFilter = "GameplayCue"))
   static void RemoveNonReplicatedGameplayCueFromActor(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters);

   /// execute a local-only gameplay cue on an actor w/o replicating it
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta = (GameplayTagFilter = "GameplayCue"))
   static void ExecuteNonReplicatedGameplayCueOnActorWithParams(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters);

   /// execute a gameplay cue on an arbitrary actor
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta = (GameplayTagFilter = "GameplayCue"))
   static void ExecuteGameplayCueOnActor(AActor* actor, FGameplayTag gameplayCueTag);

   /// execute a gameplay cue on an arbitrary actor with params
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta = (GameplayTagFilter = "GameplayCue"))
   static void ExecuteGameplayCueOnActorWithParams(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters);

   /// execute a gameplay cue on an asc with params
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE", meta = (GameplayTagFilter = "GameplayCue"))
   static void ExecuteGameplayCueOnAbilitySystem(UAbilitySystemComponent* abilitySystem, const FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters);

   /// Adds an instigator + effect causer to the effect context
   UFUNCTION(BlueprintCallable, Category = "Ability|EffectContext|OSE", Meta = (DisplayName = "AddInstigator"))
   static void EffectContextAddInstigator(FGameplayEffectContextHandle effectContext, AActor* instigator, AActor* effectCauser);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE|AppliedEffectSet")
   static void AddToAppliedEffectSet(UPARAM(Ref) FOSEActorsWithAppliedEffectsSet& effectSet, AActor* actor, FActiveGameplayEffectHandle effectHandle);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE|AppliedEffectSet")
   static void AddMultipleToAppliedEffectSet(UPARAM(Ref) FOSEActorsWithAppliedEffectsSet& effectSet, AActor* actor, const TArray<FActiveGameplayEffectHandle>& effectHandles);

   // -1 stacks removes all of them
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE|AppliedEffectSet", meta=(DisplayName="Cancel Effect By Actor"))
   static bool CancelEffectInAppliedEffectSetByActor(UPARAM(Ref) FOSEActorsWithAppliedEffectsSet& effectSet, AActor* actor, int32 stacksToRemove = 1);

   // -1 stacks removes all of them
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE|AppliedEffectSet", meta = (DisplayName = "Clear Applied Effects"))
   static void CancelAllAppliedEffectSet(UPARAM(Ref) FOSEActorsWithAppliedEffectsSet& effectSet, int32 stacksToRemove = 1);

   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   static bool CanActivateAbility(UAbilitySystemComponent* abilitySystem, UGameplayAbility* ability);

   UFUNCTION(BlueprintPure, Category = "Ability|OSE", meta = (DisplayName = "GetValueAtLevel", CompactNodeTitle = "GetValueAtLevel"))
   static float ScalableFloat_GetValueAtLevel(const FScalableFloat& scalableFloat, float level);
};
