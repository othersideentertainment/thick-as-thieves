// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATInhibitorSubsystem.h"

// tat
#include "Environment/TATInhibitorActor.h"

// ue
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInhibitorSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATInhibitorSubsystem, Log, All)

UTATInhibitorSubsystem::UTATInhibitorSubsystem()
{
}

// static
bool UTATInhibitorSubsystem::CanActorBeInhibitedBy(AActor* targetActor, TSubclassOf<ATATInhibitorActor> inhibitorClass)
{
   if (!IsValid(targetActor) || !inhibitorClass || !targetActor->Implements<UTATInhibitableInterface>())
   {
      return false;
   }
   ATATInhibitorActor* cdo = inhibitorClass.GetDefaultObject();
   check(cdo != nullptr);
   return ITATInhibitableInterface::Execute_CanBeInhibitedBy(targetActor, cdo->InhibitorType);
}

// static
bool UTATInhibitorSubsystem::IsActorInhibitable(AActor* targetActor, TSubclassOf<ATATInhibitorActor> inhibitorClass, FGameplayTag& inhibitableType)
{
   if (CanActorBeInhibitedBy(targetActor, inhibitorClass))
   {
      inhibitableType = ITATInhibitableInterface::Execute_GetInhibitableType(targetActor);
      return true;
   }
   inhibitableType = FGameplayTag::EmptyTag;
   return false;
}

bool UTATInhibitorSubsystem::IsActorCurrentlyInhibited(const AActor* actor) const
{
   if (!IsValid(actor))
   {
      return false;
   }
   const int32_t* refCount = _inhibitorRefCounts.Find(actor);
   return refCount != nullptr && *refCount > 0;
}

static bool IsValidInhibitableTargetActor(AActor* targetActor, TSubclassOf<ATATInhibitorActor> inhibitorClass, const TCHAR* functionName)
{
   if (!inhibitorClass)
   {
      UE_LOG(LogTATInhibitorSubsystem, Error, TEXT("%s called with invalid inhibitor class (on target actor %s)"),
         functionName, *GetNameSafe(targetActor));
      return false;
   }

   if (targetActor == nullptr)
   {
      UE_LOG(LogTATInhibitorSubsystem, Error, TEXT("%s called on null target (with inhibitor class = %s)"),
         functionName, *GetNameSafe(inhibitorClass));
      return false;
   }

   if (!targetActor->Implements<UTATInhibitableInterface>())
   {
      UE_LOG(LogTATInhibitorSubsystem, Error, TEXT("%s called on actor %s that does not have the inhibitable interface (with inhibitor class = %s)"),
         functionName, *GetNameSafe(targetActor), *GetNameSafe(inhibitorClass));
      return false;
   }

   return true;
}

// static
ATATInhibitorActor* UTATInhibitorSubsystem::AuthoritySpawnInhibitorActor(AActor* targetActor, APawn* instigatorPawn, TSubclassOf<ATATInhibitorActor> inhibitorClass, FLinearColor color)
{
   if (!IsValidInhibitableTargetActor(targetActor, inhibitorClass, TEXT("AuthoritySpawnInhibitorActor")))
   {
      return nullptr;
   }

   UWorld* world = GEngine->GetWorldFromContextObject(targetActor, EGetWorldErrorMode::ReturnNull);
   check(world != nullptr);
   if (!ensure(world->GetNetMode() < NM_Client))
   {
      return nullptr;
   }

   FTATInhibitorPlacementInfo placementInfo = ITATInhibitableInterface::Execute_GetInhibitorPlacementInfo(targetActor);

   // Ideally the actor would supply its own placement info, but we can provide a simple fallback if it does not. Handy for prototyping.
   if (!placementInfo.IsValid())
   {
      placementInfo = FTATInhibitorPlacementInfo::Make(targetActor->GetActorLocation(), targetActor->GetActorRotation());
   }

   return AuthoritySpawnInhibitorActorWithPlacementInfo(targetActor, instigatorPawn, inhibitorClass, placementInfo, color);
}

// static
ATATInhibitorActor* UTATInhibitorSubsystem::AuthoritySpawnInhibitorActorWithPlacementInfo(AActor* targetActor, APawn* instigatorPawn,
   TSubclassOf<ATATInhibitorActor> inhibitorClass, const FTATInhibitorPlacementInfo& placementInfo, FLinearColor color)
{
   if (!IsValidInhibitableTargetActor(targetActor, inhibitorClass, TEXT("AuthoritySpawnInhibitorActorWithPlacementInfo")))
   {
      return nullptr;
   }

   if (!placementInfo.IsValid())
   {
      UE_LOG(LogTATInhibitorSubsystem, Error, TEXT("AuthoritySpawnInhibitorActorWithPlacementInfo called with invalid placement info (target = %s, inhibitor class = %s)"),
         *GetNameSafe(targetActor), *GetNameSafe(inhibitorClass))
      return nullptr;
   }

   UWorld* world = GEngine->GetWorldFromContextObject(targetActor, EGetWorldErrorMode::ReturnNull);
   check(world != nullptr);
   if (!ensure(world->GetNetMode() < NM_Client))
   {
      return nullptr;
   }

   // Make sure we're allowed to apply this inhibitor class to this actor
   if (!CanActorBeInhibitedBy(targetActor, inhibitorClass))
   {
      return nullptr;
   }

   const FTransform transform{ placementInfo.WorldRotation.Quaternion(), placementInfo.WorldLocation };
   AActor* owner = instigatorPawn;
   ATATInhibitorActor* inhibitorActor = world->SpawnActorDeferred<ATATInhibitorActor>(
      inhibitorClass,
      transform,
      owner,
      instigatorPawn,
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
      ESpawnActorScaleMethod::OverrideRootScale);

   ACharacter* owningCharacter = Cast<ACharacter>(instigatorPawn);
   APlayerState* owningPlayerState = (owningCharacter != nullptr) ? owningCharacter->GetPlayerState() : nullptr;
   inhibitorActor->AuthoritySetupBeforeFinishSpawning(targetActor, placementInfo, color, owningPlayerState);

   inhibitorActor->FinishSpawning(transform);

   return inhibitorActor;
}

bool UTATInhibitorSubsystem::NotifyActorApplyInhibitor(AActor* actor, ATATInhibitorActor* inhibitorActor, int32& outNewInhibitorCount)
{
   if (!ensure(actor != nullptr))
   {
      return false;
   }
   int32& refCount = _inhibitorRefCounts.FindOrAdd(actor);
   refCount++;
   outNewInhibitorCount = refCount;
   return true;
}

bool UTATInhibitorSubsystem::NotifyActorRemoveInhibitor(AActor* actor, ATATInhibitorActor* inhibitorActor, int32& outNewInhibitorCount)
{
   outNewInhibitorCount = 0;

   if (!ensure(actor != nullptr))
   {
      return false;
   }

   if (int32* refCount = _inhibitorRefCounts.Find(actor))
   {
      (*refCount)--;
      outNewInhibitorCount = *refCount;

      // Remove this actor from the map if there are no remaining references
      if (*refCount <= 0)
      {
         _inhibitorRefCounts.Remove(actor);
      }

      return true;
   }

   UE_LOG(LogTATInhibitorSubsystem, Warning, TEXT("NotifyActorRemoveInhibitor called on actor %s that has no inhibitors applied"), *GetNameSafe(actor));
   return false;
}
