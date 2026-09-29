// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEAbilityFunctionLibrary.h"

// ose
#include "OSECommon.h"
#include "OSEProjectSettings.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/OSEAbilitySystemGlobals.h"
#include "Abilities/OSEActorsWithAppliedEffectsSet.h"
#include "Character/OSETeamInterface.h"

// ue4
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameplayCueManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilityFunctionLibrary)

DEFINE_LOG_CATEGORY(LogOSEAbilityFunctionLibrary);

//////////////////////////////////////////////////////////////////////////
///            FOSETagGameplayTargetDataFilter
//////////////////////////////////////////////////////////////////////////

bool FOSETagGameplayTargetDataFilter::FilterPassesForActor(const AActor* actorToBeFiltered) const
{
   FGameplayTagContainer tagContainer;
   UOSEAbilityFunctionLibrary::GetOwnedGameplayTagsFromActor(actorToBeFiltered, tagContainer);
   
   // passes if we match any of our required matches (or we don't have any tags to match against)
   bool passesMatchedTags = true;
   if (PawnTagsToMatch.IsValid())
   {
      passesMatchedTags = tagContainer.HasAny(PawnTagsToMatch);
   }
   
   // passes if we match any of our required exclusions matches (or we don't have any tags to match against)
   bool passesExcludedTags = true;
   if (PawnTagsToExclude.IsValid())
   {
      passesExcludedTags = !tagContainer.HasAny(PawnTagsToExclude);
   }

   return Super::FilterPassesForActor(actorToBeFiltered) && passesMatchedTags && passesExcludedTags;
}

//////////////////////////////////////////////////////////////////////////
///            FOSETeamAttitudeTargetDataFilter
//////////////////////////////////////////////////////////////////////////

bool FOSETeamAttitudeTargetDataFilter::FilterPassesForActor(const AActor* actorToBeFiltered) const
{
   // passes if we meet our target attitude requirements (or we dont have any)
   bool targetAttitudeRequirementMet = true;
   
   if (RequireTargetAttitude)
   {
      if (SelfActor && SelfActor->Implements<UOSETeamInterface>())
      {
         EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(SelfActor, actorToBeFiltered);
         targetAttitudeRequirementMet = (attitude == TargetAttitude);
      }
      else
      {
         targetAttitudeRequirementMet = false;
      }
   }

   return Super::FilterPassesForActor(actorToBeFiltered) && targetAttitudeRequirementMet;
}

//////////////////////////////////////////////////////////////////////////
///            FOSEEffectWithSetByCallerTag
//////////////////////////////////////////////////////////////////////////

void FOSEEffectWithSetByCallerTag::ApplyEffectWithMagnitude(UAbilitySystemComponent* asc, float magnitude) const
{
   if (asc && asc->IsOwnerActorAuthoritative())
   {
      FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
      if (effectContext.IsValid())
      {
         // ASSUMPTION: I'm going to assume this LoadSynchronous() is a no-op and these ability classes are already loaded up by time we use this.
         FGameplayEffectSpecHandle specHandle = asc->MakeOutgoingSpec(GameplayEffectClass.LoadSynchronous(), UGameplayEffect::INVALID_LEVEL, effectContext);
         if (specHandle.IsValid())
         {
            FGameplayEffectSpec* spec = specHandle.Data.Get();
            check(spec); // already checked that the handle is valid...
            if (SetByCallerMagnitudeTag.IsValid())
            {
               spec->SetSetByCallerMagnitude(SetByCallerMagnitudeTag, magnitude);
            }
            asc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////
///            FOSEEffectWithSetByCallerTagAndMagnitude
//////////////////////////////////////////////////////////////////////////

void FOSEEffectWithSetByCallerTagAndMagnitude::ApplyEffect(UAbilitySystemComponent* asc) const
{
   if (asc && asc->IsOwnerActorAuthoritative())
   {
      FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
      if (effectContext.IsValid())
      {
         // ASSUMPTION: I'm going to assume this LoadSynchronous() is a no-op and these ability classes are already loaded up by time we use this.
         FGameplayEffectSpecHandle specHandle = asc->MakeOutgoingSpec(GameplayEffectClass.LoadSynchronous(), UGameplayEffect::INVALID_LEVEL, effectContext);
         if (specHandle.IsValid())
         {
            FGameplayEffectSpec* spec = specHandle.Data.Get();
            check(spec); // already checked that the handle is valid...
            if (SetByCallerMagnitudeTag.IsValid())
            {
               spec->SetSetByCallerMagnitude(SetByCallerMagnitudeTag, Magnitude);
            }
            asc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////
///            UOSEAbilityFunctionLibrary
//////////////////////////////////////////////////////////////////////////

void UOSEAbilityFunctionLibrary::CancelAbilitiesWithTags(UAbilitySystemComponent* abilitySystem, const FGameplayTagContainer& withTags)
{
   // Tags may be invalid, but we still want to pass them through to "CancelAbilities".
   if (!withTags.IsValid())
   {
      UE_LOG(LogOSEAbilityFunctionLibrary, Warning, TEXT("Attempting to cancel an ability with invalid tags."));
   }

   if (abilitySystem)
   {
      abilitySystem->CancelAbilities(&withTags);
   }
   else
   {
      UE_LOG(LogOSEAbilityFunctionLibrary, Warning, TEXT("Attempting to call cancel on a null Ability System Component."));
   }
}

FGameplayTagContainer UOSEAbilityFunctionLibrary::SupplementalTagsFromEffectContext(FGameplayEffectContextHandle context)
{
   FOSEGameplayEffectContext* contextAsOSE = FOSEGameplayEffectContext::GetFromHandle(context);
   if (!contextAsOSE)
   {
      return FGameplayTagContainer();
   }
   return contextAsOSE->GetEventTags();
}

TArray<FActiveGameplayEffectHandle> UOSEAbilityFunctionLibrary::GetActiveEffectsByClass(UAbilitySystemComponent* abilitySystem, TSubclassOf<UGameplayEffect> effectClass)
{
   TArray<FActiveGameplayEffectHandle> effectHandles;
   if (abilitySystem)
   {
      FGameplayEffectQuery query;
      query.EffectDefinition = effectClass;
      effectHandles = abilitySystem->GetActiveEffects(query);
   }
   return effectHandles;
}

FGameplayEffectContextHandle UOSEAbilityFunctionLibrary::GetEffectContextForActiveEffect(FActiveGameplayEffectHandle effectHandle)
{
   if (UAbilitySystemComponent* asc = effectHandle.GetOwningAbilitySystemComponent())
   {
      if (const FActiveGameplayEffect* effect = asc->GetActiveGameplayEffect(effectHandle))
      {
         return effect->Spec.GetEffectContext();
      }
   }
   return FGameplayEffectContextHandle();
}

float UOSEAbilityFunctionLibrary::GetLevelForActiveEffect(FActiveGameplayEffectHandle effectHandle)
{
   if (UAbilitySystemComponent* asc = effectHandle.GetOwningAbilitySystemComponent())
   {
      if (const FActiveGameplayEffect* effect = asc->GetActiveGameplayEffect(effectHandle))
      {
         return effect->Spec.GetLevel();
      }
   }
   return 0;
}

int UOSEAbilityFunctionLibrary::GetStackCountByGameplayEffectAssetTag(UAbilitySystemComponent* abilitySystem, FGameplayTag assetTag)
{
   int stackCount = 0;
   if (abilitySystem)
   {
      FGameplayEffectQuery query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(assetTag.GetSingleTagContainer());
      TArray<FActiveGameplayEffectHandle> activeHandles = abilitySystem->GetActiveEffects(query);
      if (activeHandles.Num() > 0)
      {
         return abilitySystem->GetCurrentStackCount(activeHandles[0]);
      }
   }
   return stackCount;
}

UGameplayAbility* UOSEAbilityFunctionLibrary::FindFirstMatchingAbilityInstance(UAbilitySystemComponent* abilitySystem, const FGameplayTagContainer& tagsToMatch)
{
   check(abilitySystem);

   TArray<struct FGameplayAbilitySpec*> found;
   abilitySystem->GetActivatableGameplayAbilitySpecsByAllMatchingTags(tagsToMatch, found, false);

   for (const FGameplayAbilitySpec* spec : found)
   {
      UGameplayAbility* instance = spec->GetPrimaryInstance();
      if (instance)
      {
         return instance;
      }
   }

   return nullptr;
}

FGameplayTag UOSEAbilityFunctionLibrary::GetFirstTagInContainer(const FGameplayTagContainer& tagContainer)
{
   return tagContainer.First();
}

void UOSEAbilityFunctionLibrary::AddLooseGameplayTag(UAbilitySystemComponent* abilitySystem, FGameplayTag gameplayTag)
{
   if (abilitySystem)
   {
      abilitySystem->AddLooseGameplayTag(gameplayTag);
   }
}

void UOSEAbilityFunctionLibrary::AddLooseGameplayTags(UAbilitySystemComponent* abilitySystem, FGameplayTagContainer gameplayTags)
{
   if (abilitySystem)
   {
      abilitySystem->AddLooseGameplayTags(gameplayTags);
   }
}

void UOSEAbilityFunctionLibrary::RemoveLooseGameplayTag(UAbilitySystemComponent* abilitySystem, FGameplayTag gameplayTag)
{
   if (abilitySystem)
   {
      abilitySystem->RemoveLooseGameplayTag(gameplayTag);
   }
}

void UOSEAbilityFunctionLibrary::RemoveLooseGameplayTags(UAbilitySystemComponent* abilitySystem, FGameplayTagContainer gameplayTags)
{
   if (abilitySystem)
   {
      abilitySystem->RemoveLooseGameplayTags(gameplayTags);
   }
}

void UOSEAbilityFunctionLibrary::SetLooseGameplayTagCount(UAbilitySystemComponent* abilitySystem, FGameplayTag gameplayTag, int count)
{
   if (abilitySystem)
   {
      abilitySystem->SetLooseGameplayTagCount(gameplayTag, count);
   }
}

FGameplayTag UOSEAbilityFunctionLibrary::RequestGameplayTag(const FName& gameplayTagName)
{
    return FGameplayTag::RequestGameplayTag(gameplayTagName);
}

FGameplayTag UOSEAbilityFunctionLibrary::FindSingleChildTag(const FGameplayTagContainer& container, FGameplayTag parent)
{
   check(parent != FGameplayTag::EmptyTag);

   FGameplayTag childTag = FGameplayTag::EmptyTag;
   for (const FGameplayTag& tag : container)
   {
      if (tag.MatchesTag(parent) && tag != parent)
      {
#if DO_CHECK
         // If childTag is already set, then 2+ tags under the specified parent have been found
         if (childTag.IsValid())
         {
            UE_LOG(LogOSEAbilityFunctionLibrary, Error, TEXT("Found more than one %s child tag inside %s"), *parent.ToString(), *container.ToString());
            continue;
         }
#endif

         childTag = tag;

#if !DO_CHECK
         break;
#endif
      }
   }
   return childTag;
}

bool UOSEAbilityFunctionLibrary::IsAbilityActive(UAbilitySystemComponent* abilitySystem, TSubclassOf<UGameplayAbility> inAbilityClass)
{
   if (abilitySystem)
   {
      const FGameplayAbilitySpec* abilitySpec = abilitySystem->FindAbilitySpecFromClass(inAbilityClass);
      return abilitySpec && abilitySpec->IsActive();
   }

   UE_LOG(LogOSEAbilityFunctionLibrary, Warning, TEXT("Attempting to call IsAbilityActive() on a null Ability System Component!"));
   return false;
}

bool UOSEAbilityFunctionLibrary::PerformTargetTrace(FHitResult& outHitResult, const AActor* sourceActor, FGameplayAbilityTargetingLocationInfo startLocation, float maxRange, FCollisionProfileName traceProfile, FGameplayTargetDataFilterHandle targetFilter, bool debug)
{
   FVector traceStart;
   FVector traceEnd;
   if (OffsetCameraAimToPhysicalAim(sourceActor, startLocation, maxRange, traceStart, traceEnd))
   {
      return PerformTargetTraceToPoint(outHitResult, sourceActor, traceStart, traceEnd, traceProfile, targetFilter, debug);
   }
   return false;
}

bool UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(const AActor* sourceActor, FGameplayAbilityTargetingLocationInfo startLocation, float maxRange, FVector& outStartPos, FVector& outEndPos)
{
   // We cast to a pawn since we could be running targeting abilities on non-character
   // actors, such as drones or other possessable pawns
   auto sourcePawn = UOSECommon::GetPawn<const APawn>(sourceActor);
   if (sourcePawn == nullptr)
   {
      // failed
      return false;
   }

   FVector viewStart;
   FRotator viewRot;

   // If we have a controller, use AController::GetPlayerViewPoint
   // For the AI this means the Pawn's 'Eyes' ViewPoint
   // For a Human player, this means the Camera's ViewPoint
   if (AController* sourceController = sourcePawn->Controller)
   {
      // @TODO: Do we need to check that we're the authority for the controller
      // before performing the trace? The results could be different on clients.
      sourceController->GetPlayerViewPoint(viewStart, viewRot);
   }
   else
   {
      // @TODO: Make sure this is close to valid on AI/remote players
      sourcePawn->GetActorEyesViewPoint(viewStart, viewRot);
   }

   FVector aimVec = viewRot.Vector().GetSafeNormal();
   FVector viewEnd = viewStart + (aimVec * maxRange);

   bool useSourceLocationAsTraceStart = false;
   FVector pawnLocation = sourcePawn->GetPawnViewLocation();

   // If we need to use a different source location, do so. If it's 0,0,0 or set to source actor, then don't use
   FVector sourceLocationStart = startLocation.GetTargetingTransform().GetLocation();
   if (!sourceLocationStart.IsZero() && !(startLocation.LocationType == EGameplayAbilityTargetingLocationType::ActorTransform && startLocation.SourceActor == sourcePawn))
   {
      useSourceLocationAsTraceStart = true;
      pawnLocation = sourceLocationStart;
   }

   // Move start/end point forward if pawn is in front of view
   outStartPos = viewEnd + (((pawnLocation - viewEnd) | aimVec) * aimVec);
   outEndPos = outStartPos + aimVec * maxRange;

   if (useSourceLocationAsTraceStart)
   {
      outStartPos = sourceLocationStart;
   }

   // success
   return true;
}

bool UOSEAbilityFunctionLibrary::GetPhysicalAim(const AActor* sourceActor, FVector& outStartPos, FVector& outAimVec)
{
   // We cast to a pawn since we could be running targeting abilities on non-character
   // actors, such as drones or other possessable pawns
   auto sourcePawn = UOSECommon::GetPawn<const APawn>(sourceActor);
   if (sourcePawn == nullptr)
   {
      // failed
      return false;
   }

   FVector viewStart;
   FRotator viewRot;

   // If we have a controller, use AController::GetPlayerViewPoint
   // For the AI this means the Pawn's 'Eyes' ViewPoint
   // For a Human player, this means the Camera's ViewPoint
   if (AController* sourceController = sourcePawn->Controller)
   {
      // @TODO: Do we need to check that we're the authority for the controller
      // before performing the trace? The results could be different on clients.
      sourceController->GetPlayerViewPoint(viewStart, viewRot);
   }
   else
   {
      // @TODO: Make sure this is close to valid on AI/remote players
      sourcePawn->GetActorEyesViewPoint(viewStart, viewRot);
   }

   FVector pawnLocation = sourcePawn->GetPawnViewLocation();

   outAimVec = viewRot.Vector().GetSafeNormal();

   // Move start/end point forward if pawn is in front of view
   outStartPos = viewStart + (((pawnLocation - viewStart) | outAimVec) * outAimVec);
   
   return true;
}

bool UOSEAbilityFunctionLibrary::IsAbilityActorInfoLocallyControlled(const FGameplayAbilityActorInfo& actorInfo)
{
    return actorInfo.IsLocallyControlled();
}

bool UOSEAbilityFunctionLibrary::OffsetCameraAimToAvatarAim(const AActor* sourceActor, FGameplayAbilityTargetingLocationInfo startLocation, FVector& outPos, FRotator& outRot)
{
   FVector startPos;
   FVector endPos;
   static const float kMaxInteractionRange = 1.0f;  // arbitrary because we use the endpos for a look-at but aren't targeting it.
   if (!UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(sourceActor, startLocation, kMaxInteractionRange, startPos, endPos))
   {
      // failed
      return false;
   }

   outRot = UKismetMathLibrary::FindLookAtRotation(startPos, endPos);
   outPos = startPos;
   return true;
}

bool UOSEAbilityFunctionLibrary::PerformTargetTraceToPoint(FHitResult& outHitResult, const AActor* sourceActor, FVector sourcePoint, FVector targetPoint, FCollisionProfileName traceProfile, FGameplayTargetDataFilterHandle targetFilter, bool debug)
{
   if (!sourceActor)
   {
      return false;
   }

   // This is based on AGameplayAbilityTargetActor_SingleLineTrace
   bool traceComplex = false;

   FCollisionQueryParams params(SCENE_QUERY_STAT(UOSEAbilityFunctionLibrary), traceComplex);
   params.bReturnPhysicalMaterial = true;
   params.AddIgnoredActor(sourceActor);

   // Call our version as we don't want overlaps to count as blocking hits
   LineTraceWithFilter(outHitResult, sourceActor->GetWorld(), targetFilter, sourcePoint, targetPoint, traceProfile.Name, params, false);
   
   // Default to end of trace line if we don't hit anything.
   if (!outHitResult.bBlockingHit)
   {
      outHitResult.Location = targetPoint;
   }

#if ENABLE_DRAW_DEBUG
   if (debug)
   {
      const float kDrawDuration = 5.0f;
      DrawDebugLine(sourceActor->GetWorld(), sourcePoint, targetPoint, FColor::Yellow, false, kDrawDuration);
      DrawDebugSphere(sourceActor->GetWorld(), targetPoint, 50.0f, 16, FColor::Yellow, false, kDrawDuration);
      if (outHitResult.bBlockingHit)
      {
         DrawDebugSphere(sourceActor->GetWorld(), outHitResult.Location, 5.0f, 16, FColor::Green, false, kDrawDuration);
      }
   }
#endif // ENABLE_DRAW_DEBUG

   return outHitResult.bBlockingHit;
}

bool UOSEAbilityFunctionLibrary::PerformTargetSweepToPoint(FHitResult& outHitResult, const AActor* sourceActor, FVector sourcePoint, FVector targetPoint, FCollisionProfileName traceProfile, float radius, FGameplayTargetDataFilterHandle targetFilter, bool debug)
{
   if (!sourceActor)
   {
      return false;
   }

   // This is based on AGameplayAbilityTargetActor_SingleLineTrace
   bool traceComplex = false;

   FCollisionQueryParams params(SCENE_QUERY_STAT(UOSEAbilityFunctionLibrary), traceComplex);
   params.bReturnPhysicalMaterial = true;
   params.AddIgnoredActor(sourceActor);

   // Call our version as we don't want overlaps to count as blocking hits
  SweepWithFilter(outHitResult, sourceActor->GetWorld(), targetFilter, sourcePoint, targetPoint, FQuat::Identity, FCollisionShape::MakeSphere(radius), traceProfile.Name, params, false);

   // Default to end of trace line if we don't hit anything.
   if (!outHitResult.bBlockingHit)
   {
      outHitResult.Location = targetPoint;
   }

#if ENABLE_DRAW_DEBUG
   if (debug)
   {
      const float kDrawDuration = 5.0f;
      DrawDebugLine(sourceActor->GetWorld(), sourcePoint, targetPoint, FColor::Yellow, false, kDrawDuration);
      DrawDebugSphere(sourceActor->GetWorld(), targetPoint, 50.0f, 16, FColor::Yellow, false, kDrawDuration);
      if (outHitResult.bBlockingHit)
      {
         DrawDebugSphere(sourceActor->GetWorld(), outHitResult.Location, 5.0f, 16, FColor::Green, false, kDrawDuration);
      }
   }
#endif // ENABLE_DRAW_DEBUG

   return outHitResult.bBlockingHit;
}

// Based on AGameplayAbilityTargetActor_Trace with extra options
void UOSEAbilityFunctionLibrary::LineTraceWithFilter(FHitResult& outHitResult, const UWorld* world, const FGameplayTargetDataFilterHandle filterHandle, const FVector& start, const FVector& end, FName profileName, const FCollisionQueryParams params, bool treatOverlapAsBlocking)
{
   TArray<FHitResult> hitResults;
   world->LineTraceMultiByProfile(hitResults, start, end, profileName, params);

   for (int32 hitIdx = 0; hitIdx < hitResults.Num(); ++hitIdx)
   {
      const FHitResult& hit = hitResults[hitIdx];

      if (!hit.HitObjectHandle.IsValid() || filterHandle.FilterPassesForActor(hit.GetActor()))
      {
         if (hit.bBlockingHit || treatOverlapAsBlocking)
         {
            outHitResult = hit;
            outHitResult.bBlockingHit = true; // treat it as a blocking hit
            return;
         }
      }
   }

   outHitResult.Init(start, end);
}

void UOSEAbilityFunctionLibrary::SweepWithFilter(FHitResult& outHitResult, const UWorld* world, const FGameplayTargetDataFilterHandle filterHandle, const FVector& start, const FVector& end, const FQuat& rotation, const FCollisionShape collisionShape, FName profileName, const FCollisionQueryParams params, bool treatOverlapAsBlocking)
{
   check(world);

   TArray<FHitResult> hitResults;
   world->SweepMultiByProfile(hitResults, start, end, rotation, profileName, collisionShape, params);

   for (int32 hitIdx = 0; hitIdx < hitResults.Num(); ++hitIdx)
   {
      const FHitResult& hit = hitResults[hitIdx];

      if (!hit.HitObjectHandle.IsValid() || filterHandle.FilterPassesForActor(hit.GetActor()))
      {
         if (hit.bBlockingHit || treatOverlapAsBlocking)
         {
            outHitResult = hit;
            outHitResult.bBlockingHit = true; // treat it as a blocking hit
            return;
         }
      }
   }

   outHitResult.Init(start, end);
}

AOSECharacterBase* UOSEAbilityFunctionLibrary::GetInstigatorCharacterFromContext(FGameplayEffectContextHandle effectContext)
{
   const FOSEGameplayEffectContext* contextInstance = FOSEGameplayEffectContext::GetFromHandle(effectContext);

   if (contextInstance)
   {
      return contextInstance->GetInstigatorCharacter();
   }

   return nullptr;
}

float UOSEAbilityFunctionLibrary::GetGrantedByEffectSetByCallerMagnitude(const UGameplayAbility* ability, FGameplayTag tag, float defaultValue)
{
   if (IsValid(ability) && ability->IsInstantiated() && ability->GetCurrentActorInfo())
   {
      UAbilitySystemComponent* const abilitySystemComponent = ability->GetAbilitySystemComponentFromActorInfo_Ensured();
      FActiveGameplayEffectHandle activeHandle = abilitySystemComponent->FindActiveGameplayEffectHandle(ability->GetCurrentAbilitySpecHandle());
      if (activeHandle.IsValid())
      {
         const FActiveGameplayEffect* effect = abilitySystemComponent->GetActiveGameplayEffect(activeHandle);
         return effect->Spec.GetSetByCallerMagnitude(tag, true, defaultValue);
      }
   }
   return defaultValue;
}

float UOSEAbilityFunctionLibrary::GetGrantedByEffectLevel(const UGameplayAbility* ability, float defaultValue)
{
   if (IsValid(ability) && ability->IsInstantiated() && ability->GetCurrentActorInfo())
   {
      UAbilitySystemComponent* const abilitySystemComponent = ability->GetAbilitySystemComponentFromActorInfo_Ensured();
      FActiveGameplayEffectHandle activeHandle = abilitySystemComponent->FindActiveGameplayEffectHandle(ability->GetCurrentAbilitySpecHandle());
      if (activeHandle.IsValid())
      {
         const FActiveGameplayEffect* effect = abilitySystemComponent->GetActiveGameplayEffect(activeHandle);
         return effect->Spec.GetLevel();
      }
   }
   return defaultValue;
}

float UOSEAbilityFunctionLibrary::GetGameplayEffectDurationByClass(const AActor* actor, TSubclassOf<UGameplayEffect> gameplayEffect)
{
   if (const UOSEAbilitySystemComponent* asc = UOSEAbilitySystemGlobals::GetOSEAbilitySystemComponentFromActor(actor))
   {
      TArray<FGameplayEffectSpec> outSpecCopies;
      asc->GetAllActiveGameplayEffectSpecs(outSpecCopies);
      for (const FGameplayEffectSpec& spec : outSpecCopies)
      {
         if (spec.Def && spec.Def->GetClass() == gameplayEffect)
         {
            return spec.Duration;
         }
      }
   }
   return float(INDEX_NONE);
}

float UOSEAbilityFunctionLibrary::GetEffectAttributeModifierMagnitudeByClass(TSubclassOf<UGameplayEffect> gameplayEffectClass, FGameplayAttribute attribute, bool& found, float level /*= 1.f*/)
{
   if (const UGameplayEffect* gameplayEffect = gameplayEffectClass.GetDefaultObject())
   {
      for (const FGameplayModifierInfo& modifier : gameplayEffect->Modifiers)
      {
         if (modifier.Attribute == attribute)
         {
            float magnitude = -1.f;
            const FString contextString = FString::Printf(TEXT("GetEffectAttributeModifierMagnitudeByClass()"), *gameplayEffectClass->GetName());
            if (modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(level, magnitude, &contextString))
            {
               found = true;
               return magnitude;
            }
            else
            {
               UE_LOG(LogOSEAbilityFunctionLibrary, Error, TEXT("GetEffectAttributeModifierMagnitudeByClass() called for effect %s with modifier of unexpected EGameplayEffectMagnitudeCalculation type %s!")
                  , *gameplayEffectClass->GetName()
                  , *UEnum::GetValueAsString(modifier.ModifierMagnitude.GetMagnitudeCalculationType()));
               break;
            }
         }
      }
   }
   found = false;
   return 0.0f;
}

float UOSEAbilityFunctionLibrary::GetGameplayEffectDurationByEffectTag(const AActor* actor, FGameplayTag tag)
{
   // Would it be preferable to just expose the FGameplayEffectQuery directly as a parameter?
   if (const UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      // N.B. GetActiveEffectsEndTimeAndDuration() will read in the value of endTime, so it needs to be initialized
      float endTime = 0.0f;
      float duration = 0.0f;
      FGameplayEffectQuery query;
      query.EffectTagQuery.Build(
         FGameplayTagQueryExpression()
            .AllTagsMatch()
            .AddTag(tag)
      );
      if (asc->GetActiveEffectsEndTimeAndDuration(query, endTime, duration))
      {
         return duration;
      }
   }
   return float(INDEX_NONE);
}

float UOSEAbilityFunctionLibrary::GetGameplayEffectPeriodFromSpecHandle(const FGameplayEffectSpecHandle& specHandle)
{
   if (specHandle.IsValid())
   {
      FGameplayEffectSpec* spec = specHandle.Data.Get();
      check(spec); // already checked that the handle is valid...
      return spec->GetPeriod();
   }
   return 0.0f;
}

void UOSEAbilityFunctionLibrary::GetOwnedGameplayTagsFromActor(const AActor* actor, FGameplayTagContainer& tagContainer)
{
   tagContainer.Reset();
   if (const UOSEAbilitySystemComponent* asc = UOSEAbilitySystemGlobals::GetOSEAbilitySystemComponentFromActor(actor))
   {
      asc->GetOwnedGameplayTags(tagContainer);
   }
}

namespace
{
   void GatherTagsFromExpression(const FGameplayTagQueryExpression& expression, FGameplayTagContainer& container)
   {
      for (const FGameplayTag& tag : expression.TagSet)
      {
         if (tag.IsValid())
            container.AddTag(tag);
      }

      for (const FGameplayTagQueryExpression& containedExpression : expression.ExprSet)
      {
         // recurse
         GatherTagsFromExpression(containedExpression, container);
      }
   }
}

FGameplayTagContainer UOSEAbilityFunctionLibrary::GetGameplayTagsFromQuery(const FGameplayTagQuery& tagQuery)
{
   // some serious hoops here to get access to the list of tags, since tagQuery.TagDictionary is private
   // TODO: Should I just mod the engine to expose it?  This is kind of a stupid thing to go through just to get a list of tags

   FGameplayTagQueryExpression expression;
   tagQuery.GetQueryExpr(expression);

   FGameplayTagContainer container;
   GatherTagsFromExpression(expression, container);
   return container;
}

FGameplayTargetDataFilterHandle UOSEAbilityFunctionLibrary::MakeOSETagGameplayTargetDataFilter(const FOSETagGameplayTargetDataFilter& filter, AActor* filterActor)
{
   FGameplayTargetDataFilter* newFilter = new FOSETagGameplayTargetDataFilter(filter);
   newFilter->InitializeFilterContext(filterActor);

   FGameplayTargetDataFilterHandle filterHandle;
   filterHandle.Filter = TSharedPtr<FGameplayTargetDataFilter>(newFilter);
   return filterHandle;
}

FGameplayTargetDataFilterHandle UOSEAbilityFunctionLibrary::MakeOSETeamAttitudeTargetDataFilter(const FOSETeamAttitudeTargetDataFilter& filter, AActor* filterActor)
{
   FGameplayTargetDataFilter* newFilter = new FOSETeamAttitudeTargetDataFilter(filter);
   newFilter->InitializeFilterContext(filterActor);

   FGameplayTargetDataFilterHandle filterHandle;
   filterHandle.Filter = TSharedPtr<FGameplayTargetDataFilter>(newFilter);
   return filterHandle;
}

bool UOSEAbilityFunctionLibrary::GameplayTargetDataFilterPassesForActor(const FGameplayTargetDataFilterHandle& handle, AActor* actorToFilter)
{
   if (handle.Filter)
   {
      return handle.Filter->FilterPassesForActor(actorToFilter);
   }
   return false;
}

bool UOSEAbilityFunctionLibrary::IsFriendlyToActor(const AActor* sourceActor, const AActor* targetActor)
{
   if (sourceActor && sourceActor->Implements<UOSETeamInterface>())
   {
      EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(sourceActor, targetActor);
      return attitude == EOSETeamAttitude::Friendly;
   }
   return false;
}

bool UOSEAbilityFunctionLibrary::IsHostileToActor(const AActor* sourceActor, const AActor* targetActor)
{
   if (sourceActor && sourceActor->Implements<UOSETeamInterface>())
   {
      EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(sourceActor, targetActor);
      return attitude == EOSETeamAttitude::Hostile;
   }
   return false;
}

bool UOSEAbilityFunctionLibrary::IsNeutralToActor(const AActor* sourceActor, const AActor* targetActor)
{
   if (sourceActor && sourceActor->Implements<UOSETeamInterface>())
   {
      EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(sourceActor, targetActor);
      return attitude == EOSETeamAttitude::Neutral;
   }
   return false;
}

void UOSEAbilityFunctionLibrary::ResetSpawnParameters(AActor* actorToReset)
{
   // Adapted from AActor::ResetPropertiesForConstruction(), but only reset spawn params

   // Get class CDO
   AActor* cdoActor = actorToReset->GetClass()->GetDefaultObject<AActor>();

   // Iterate over properties
   for( TFieldIterator<FProperty> it(actorToReset->GetClass()) ; it ; ++it )
   {
      FProperty* prop = *it;
      auto structProp = CastField<FStructProperty>(prop);

      if (!prop->ContainsInstancedObjectProperty())
      {
         const bool exposedOnSpawn = prop->HasAnyPropertyFlags(CPF_ExposeOnSpawn);

         if (exposedOnSpawn)
         {
            prop->CopyCompleteValue_InContainer(actorToReset, cdoActor);
         }
      }
   }
}

const UOSEAbilityMetadata* UOSEAbilityFunctionLibrary::GetAbilityMetadataByClass(const FOSEAbilityInfo& abilityInfo, const TSubclassOf<UOSEAbilityMetadata> abilityMetadataClass)
{
	for (const UOSEAbilityMetadata* oseAbilityMetadata : abilityInfo.AbilityMetadata)
	{
		if (oseAbilityMetadata->IsA(abilityMetadataClass))
		{
			return oseAbilityMetadata;
		}
	}
	return nullptr;
}

const UGameplayAbility* UOSEAbilityFunctionLibrary::GetAbilityByTag(const UAbilitySystemComponent* target,
                                                                    const FGameplayTag& tag)
{
   for (const FGameplayAbilitySpec& spec : target->GetActivatableAbilities())
   {
      if (spec.GetDynamicSpecSourceTags().HasTag(tag))
      {
         return spec.Ability;
      }
   }
   return nullptr;
}

static bool ShapeEncroachesBlockingGeometry_WithAdjustment(UWorld const* World, FName profile, const FCollisionShape& shrunkShape, const FCollisionShape& collisionShape, const FVector& location, const FQuat& rotation, FVector* outProposedAdjustment)
{
   // Adapted from LevelActor.cpp ComponentEncroachesBlockingGeometry_WithAdjustment
   FCollisionQueryParams params(SCENE_QUERY_STAT(ShapeEncroachesBlockingGeometry_WithAdjustment), false);
   FCollisionResponseParams responseParams;
   ECollisionChannel traceChannel;
   UCollisionProfile::GetChannelAndResponseParams(profile, traceChannel, responseParams);

   TArray<FOverlapResult> overlaps;
   bool foundBlockingHit = World->OverlapMultiByChannel(overlaps, location, rotation, traceChannel, shrunkShape, params, responseParams);

   // compute adjustment
   if (foundBlockingHit && outProposedAdjustment)
   {
      // if encroaching, add up all the MTDs of overlapping shapes
      FMTDResult mtdResult;
      uint32 numBlockingHits = 0;
      for (int32 hitIdx = 0; hitIdx < overlaps.Num(); hitIdx++)
      {
         UPrimitiveComponent* const overlapComponent = overlaps[hitIdx].Component.Get();
         // first determine closest impact point along each axis
         if (overlapComponent && overlapComponent->GetCollisionResponseToChannel(traceChannel) == ECR_Block)
         {
            numBlockingHits++;
            bool success = overlapComponent->ComputePenetration(mtdResult, collisionShape, location, rotation);
            if (success)
            {
               *outProposedAdjustment += mtdResult.Direction * mtdResult.Distance;
            }
            else
            {
               UE_LOG(LogPhysics, Log, TEXT("OverlapTest says we are overlapping, yet MTD says we're not. Something is wrong"));
               // It's not safe to use a partial result, that could push us out to an invalid location (like the other side of a wall).
               *outProposedAdjustment = FVector::ZeroVector;
               return true;
            }
         }
      }

      // See if we chose to invalidate all of our supposed "blocking hits".
      if (numBlockingHits == 0)
      {
         *outProposedAdjustment = FVector::ZeroVector;
         foundBlockingHit = false;
      }
   }

   return foundBlockingHit;
}

static bool ShapeEncroachesBlockingGeometry_NoAdjustment(UWorld const* World, FName profile, const FCollisionShape& collisionShape, const FVector& location, const FQuat& rotation)
{
   // Adapted from LevelActor.cpp ComponentEncroachesBlockingGeometry_NoAdjustment
   FCollisionQueryParams params(SCENE_QUERY_STAT(ShapeEncroachesBlockingGeometry_NoAdjustment), false);
   return World->OverlapBlockingTestByProfile(location, rotation, profile, collisionShape, params);
}

static bool FindTeleportSpot(const UWorld* world, FName profile, const FCollisionShape& shrunkShape, const FCollisionShape& collisionShape, const FVector& location, const FQuat& rotation, FVector& outLocation)
{
   // Adapted from LevelActor.cpp FindTeleportSpot
   QUICK_SCOPE_CYCLE_COUNTER(STAT_UOSEAbilityFunctionLibrary_FindTeleportSpot);

   FVector adjust(0.f);

   const FVector originalTestLocation = location;
   outLocation = originalTestLocation;

   // check if fits at desired location
   if (!ShapeEncroachesBlockingGeometry_WithAdjustment(world, profile, shrunkShape, collisionShape, location, rotation, &adjust))
   {
      return true;
   }

   if (adjust.IsNearlyZero())
   {
      outLocation = originalTestLocation;
      return false;
   }

   // first do only Z
   const bool bZeroZ = FMath::IsNearlyZero(adjust.Z, KINDA_SMALL_NUMBER);
   if (!bZeroZ)
   {
      outLocation.Z += adjust.Z;
      if (!ShapeEncroachesBlockingGeometry_NoAdjustment(world, profile, shrunkShape, outLocation, rotation))
      {
         return true;
      }

      outLocation = originalTestLocation;
   }

   // now try just XY
   const bool bZeroX = FMath::IsNearlyZero(adjust.X, KINDA_SMALL_NUMBER);
   const bool bZeroY = FMath::IsNearlyZero(adjust.Y, KINDA_SMALL_NUMBER);
   if (!bZeroX || !bZeroY)
   {
      const float X = bZeroX ? 0.f : adjust.X;
      const float Y = bZeroY ? 0.f : adjust.Y;
      FVector adjustment = FVector(X, Y, 0);
      outLocation = originalTestLocation + adjustment;
      if (!ShapeEncroachesBlockingGeometry_NoAdjustment(world, profile, shrunkShape, outLocation, rotation))
      {
         return true;
      }

      // Try XY adjustment including Z. Note that even with only 1 iteration, this will still try the full proposed (X,Y,Z) adjustment.
      if (!bZeroZ)
      {
         outLocation = originalTestLocation + adjustment;
         outLocation.Z += adjust.Z;
         if (!ShapeEncroachesBlockingGeometry_NoAdjustment(world, profile, shrunkShape, outLocation, rotation))
         {
            return true;
         }
      }
   }

   // Don't write out the last failed test location, we promised to only if we find a good spot, in case the caller re-uses the original input.
   outLocation = originalTestLocation;
   return false;
}

bool UOSEAbilityFunctionLibrary::FindSphereTeleportSpot(UObject* worldContext, const FVector& location, float radius, FName collisionProfile, FVector& outAdjustedLocation)
{
   check(worldContext);
   const float radiusEpsilon = 0.15f;
   return FindTeleportSpot(worldContext->GetWorld(), collisionProfile, FCollisionShape::MakeSphere(radius - radiusEpsilon), FCollisionShape::MakeSphere(radius), location, FQuat::Identity, outAdjustedLocation);
}

void UOSEAbilityFunctionLibrary::ApplyEffectWithSetByCallerTag(AActor* actor, const FOSEEffectWithSetByCallerTag& effectInfo, float magnitude)
{
   if (UOSEAbilitySystemComponent* asc = UOSEAbilitySystemGlobals::GetOSEAbilitySystemComponentFromActor(actor))
   {
      effectInfo.ApplyEffectWithMagnitude(asc, magnitude);
   }
}

void UOSEAbilityFunctionLibrary::ApplyEffectWithSetByCallerTagAndMagnitude(AActor* actor, const FOSEEffectWithSetByCallerTagAndMagnitude& effectInfo)
{
   if (UOSEAbilitySystemComponent* asc = UOSEAbilitySystemGlobals::GetOSEAbilitySystemComponentFromActor(actor))
   {
      effectInfo.ApplyEffect(asc);
   }
}

void UOSEAbilityFunctionLibrary::AddGameplayCueToActor(AActor* actor, FGameplayTag gameplayCueTag)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      asc->AddGameplayCue(gameplayCueTag);
   }
}

void UOSEAbilityFunctionLibrary::AddGameplayCueToActorWithParams(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameter)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      asc->AddGameplayCue(gameplayCueTag, gameplayCueParameter);
   }
}

void UOSEAbilityFunctionLibrary::AddNonReplicatedGameplayCueToActorWithParams(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters)
{
   UGameplayCueManager::AddGameplayCue_NonReplicated(actor, gameplayCueTag, gameplayCueParameters);
}

void UOSEAbilityFunctionLibrary::ExecuteNonReplicatedGameplayCueOnActorWithParams(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters)
{
   UGameplayCueManager::ExecuteGameplayCue_NonReplicated(actor, gameplayCueTag, gameplayCueParameters);
}

void UOSEAbilityFunctionLibrary::RemoveNonReplicatedGameplayCueFromActor(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters)
{
   UGameplayCueManager::RemoveGameplayCue_NonReplicated(actor, gameplayCueTag, gameplayCueParameters);
}

void UOSEAbilityFunctionLibrary::ExecuteGameplayCueOnActor(AActor* actor, FGameplayTag gameplayCueTag)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      asc->ExecuteGameplayCue(gameplayCueTag);
   }
}

void UOSEAbilityFunctionLibrary::ExecuteGameplayCueOnActorWithParams(AActor* actor, FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      asc->ExecuteGameplayCue(gameplayCueTag, gameplayCueParameters);
   }
}

void UOSEAbilityFunctionLibrary::ExecuteGameplayCueOnAbilitySystem(UAbilitySystemComponent* abilitySystem, const FGameplayTag gameplayCueTag, const FGameplayCueParameters& gameplayCueParameters)
{
   if (abilitySystem)
   {
      abilitySystem->ExecuteGameplayCue(gameplayCueTag, gameplayCueParameters);
   }
}

void UOSEAbilityFunctionLibrary::EffectContextAddInstigator(FGameplayEffectContextHandle effectContext, AActor* instigator, AActor* effectCauser)
{
   effectContext.AddInstigator(instigator, effectCauser);
}

void UOSEAbilityFunctionLibrary::AddToAppliedEffectSet(FOSEActorsWithAppliedEffectsSet& effectSet, AActor* actor, FActiveGameplayEffectHandle effectHandle)
{
   effectSet.Add(actor, effectHandle);
}

void UOSEAbilityFunctionLibrary::AddMultipleToAppliedEffectSet(FOSEActorsWithAppliedEffectsSet& effectSet, AActor* actor, const TArray<FActiveGameplayEffectHandle>& effectHandles)
{
   effectSet.AddMultiple(actor, effectHandles);
}

bool UOSEAbilityFunctionLibrary::CancelEffectInAppliedEffectSetByActor(FOSEActorsWithAppliedEffectsSet& effectSet, AActor* actor, int32 stacksToRemove)
{
   return effectSet.CancelByActor(actor, stacksToRemove);
}

void UOSEAbilityFunctionLibrary::CancelAllAppliedEffectSet(FOSEActorsWithAppliedEffectsSet& effectSet, int32 stacksToRemove)
{
   effectSet.CancelAll(stacksToRemove);
}

bool UOSEAbilityFunctionLibrary::CanActivateAbility(UAbilitySystemComponent* abilitySystem, UGameplayAbility* ability)
{
   if (!abilitySystem || !ability)
      return false;

   // loosely copied from UAbilitySystemComponent::Debug_Internal
   for (const FGameplayAbilitySpec& abilitySpec : abilitySystem->GetActivatableAbilities())
   {
      if (abilitySpec.Ability != ability)
         continue;
      
      FGameplayTagContainer failureTags;
      return abilitySpec.Ability->CanActivateAbility(abilitySpec.Handle, abilitySystem->AbilityActorInfo.Get(), nullptr, nullptr, &failureTags);
   }

   // couldn't find the ability!
   UE_LOG(LogOSEAbilityFunctionLibrary, Warning, TEXT("Could not find ability %s on asc owned by %s"), *ability->GetName(), *abilitySystem->GetOwner()->GetName());
   return false;
}

float UOSEAbilityFunctionLibrary::ScalableFloat_GetValueAtLevel(const FScalableFloat& scalableFloat, float level)
{
   return scalableFloat.GetValueAtLevel(level);
}

