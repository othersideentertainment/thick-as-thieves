// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATContainerSpawnerComponent.h"

// tat
#include "Variation/TATSpawnData.h"

// ue
#include "Logging/MessageLog.h"
#include "Misc/DataValidation.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATContainerSpawnerComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATContainerSpawner, Log, All);

UTATContainerSpawnerComponent::UTATContainerSpawnerComponent()
{
   SpawnBucketBehavior = ETATSpawnBucketBehavior::Required;
}

void UTATContainerSpawnerComponent::InitializeComponent()
{
   Super::InitializeComponent();

   AuthorityOnSpawnActorDeferred.AddDynamic(this, &UTATContainerSpawnerComponent::_AuthorityOnSpawnActorDeferred);
   AuthorityOnFinishSpawnActor.AddDynamic(this, &UTATContainerSpawnerComponent::_AuthorityOnFinishSpawnActor);
}

#if WITH_EDITOR
EDataValidationResult UTATContainerSpawnerComponent::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   // Validate slot configuration
   TArray<FString> slotWarnings = _ValidateSpawnSlots();
   for (const FString& slotError : slotWarnings)
   {
      context.AddError(FText::FromString(slotError));
   }

   if (SpawnBucket)
   {
      // Validate empty spawn bucket
      if (SpawnBucket->Entries.IsEmpty())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] | SpawnBucket %s is empty!"), *GetName(), *SpawnBucket->GetName())));
      }
   }
   else
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] | not configured with a SpawnBucket from which to spawn actors!"), *GetName())));
   }

   return context.GetNumErrors() + context.GetNumWarnings() <= 0 ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}

void UTATContainerSpawnerComponent::CheckForErrors()
{
   Super::CheckForErrors();

   // Validate slot configuration
   TArray<FString> slotWarnings = _ValidateSpawnSlots();
   for (const FString& slotError : slotWarnings)
   {
      FMessageLog("MapCheck").Warning()
         ->AddToken(FUObjectToken::Create(this))
         ->AddToken(FTextToken::Create(FText::FromString(slotError)));
   }
}
#endif // WITH_EDITOR

int32 UTATContainerSpawnerComponent::GetMaxInstancesToSpawn() const
{
   if (SpawnType == ETATSpawnChanceType::Disabled)
   {
      return 0;
   }
   if (LimitSlotsUsedForSpawning)
   {
      return FMath::Clamp(NumEnabledSlots, 1, SpawnSlotEntries.Num());
   }

   return SpawnSlotEntries.Num();
}

FTransform UTATContainerSpawnerComponent::GetSpawnTransformWorldSpace(const FTATContainerSpawnSlotEntry& spawnSlotEntry) const
{
   const FTransform& actorToWorld = GetOwner()->GetTransform();
   const FTransform spawnToActor(spawnSlotEntry.SpawnRotation.Quaternion(), spawnSlotEntry.SpawnOffset, FVector::OneVector);
   return spawnToActor * actorToWorld;
}

void UTATContainerSpawnerComponent::_AuthorityOnSpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClassSoftPtr, const FRandomStream& randomStream, AActor*& spawnedActorInstance)
{
   if (SpawnSlotEntries.Num() <= 0)
   {
      UE_LOG(LogTATContainerSpawner, Error, TEXT("[%s] AuthoritySpawnActorDeferred() called with empty SpawnSlotEntries!"), *GetOwner()->GetName());
      return;
   }

   // Select an open slot to spawn into
   FTATContainerSpawnSlotEntry* spawnSlotEntry = _FindUnusedSpawnSlot(randomStream);
   if (!spawnSlotEntry)
   {
      UE_LOG(LogTATContainerSpawner, Error, TEXT("[%s] AuthoritySpawnActorDeferred() called after all %d SpawnSlotEntries have been used for spawning!")
         , *GetOwner()->GetName()
         , SpawnSlotEntries.Num());
      return;
   }

   if (actorClassSoftPtr.IsNull())
   {
      UE_LOG(LogTATContainerSpawner, Error, TEXT("[%s] AuthoritySpawnActorDeferred() called with invalid actorClass!")
         , *GetOwner()->GetName());
      return;
   }

   // Load the actor class
   TSubclassOf<AActor> actorClass = actorClassSoftPtr.LoadSynchronous();
   check(actorClass);

   // Spawn the actor
   const FTransform spawnTransform = GetSpawnTransformWorldSpace(*spawnSlotEntry);
   AActor* owner = nullptr;
   APawn* instigator = nullptr;
   ESpawnActorCollisionHandlingMethod collisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   if (AActor* actor = GetWorld()->SpawnActorDeferred<AActor>(actorClass, spawnTransform, owner, instigator, collisionHandlingOverride))
   {
      // Assign the out-param, so calling code can pass it to _AuthorityOnFinishSpawnActor()
      spawnedActorInstance = actor;

      // Cache the spawned actor instance, so we can retrieve this slot in AuthorityFinishedSpawnActor() (and ignore it when selecting unused slots for subsequently-spawned actors)
      spawnSlotEntry->SpawnedActorInstance = actor;
      UE_LOG(LogTATContainerSpawner, Verbose, TEXT("[%s] | AuthoritySpawnActorDeferred() spawned actor %s"), *GetOwner()->GetName(), *actor->GetName());
   }
   else
   {
      UE_LOG(LogTATContainerSpawner, Error, TEXT("[%s] failed to spawn actor of type %s!"), *GetOwner()->GetName(), *actorClassSoftPtr->GetName());
   }
}

void UTATContainerSpawnerComponent::_AuthorityOnFinishSpawnActor(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClassSoftPtr, const FRandomStream& randomStream, AActor* actorInstance)
{
   check(IsValid(actorInstance));

   // Should be guaranteed to find a spawn slot for this actor, as it would have been reserved by _AuthorityOnSpawnActorDeferred()
   const FTATContainerSpawnSlotEntry* spawnSlotEntry = _FindDeferredSpawnSlot(actorInstance);
   check(spawnSlotEntry);

   // Finish spawning the actor
   actorInstance->FinishSpawning(GetSpawnTransformWorldSpace(*spawnSlotEntry));
   UE_LOG(LogTATContainerSpawner, Verbose, TEXT("[%s] | AuthorityFinishSpawnActor() finished spawning actor %s"), *GetOwner()->GetName(), *actorInstance->GetName());
}

FTATContainerSpawnSlotEntry* UTATContainerSpawnerComponent::_FindUnusedSpawnSlot(const FRandomStream& randomStream)
{
   const TArray<int32>& enabledSpawnSlotEntries = _GetOrPopulateEnabledSpawnSlotIndices(randomStream);
   for (int32 spawnSlotIdx : _enabledSpawnSlotIndices)
   {
      check(SpawnSlotEntries.IsValidIndex(spawnSlotIdx));
      FTATContainerSpawnSlotEntry* spawnSlotEntry = &SpawnSlotEntries[spawnSlotIdx];

      // Skip slots that have been selected for spawning
      if(!spawnSlotEntry->SpawnedActorInstance.IsExplicitlyNull())
      {
         continue;
      }
      return spawnSlotEntry;
   }

   return nullptr;
}

const FTATContainerSpawnSlotEntry* UTATContainerSpawnerComponent::_FindDeferredSpawnSlot(AActor* actor) const
{
   check(IsValid(actor));

   // _enabledSpawnSlotEntries should be populated by the time we're looking up a slot reserved for an actor
   for (int32 spawnSlotIdx : _enabledSpawnSlotIndices)
   {
      check(SpawnSlotEntries.IsValidIndex(spawnSlotIdx));
      const FTATContainerSpawnSlotEntry* spawnSlotEntry = &SpawnSlotEntries[spawnSlotIdx];

      // Skip slots that have not been selected for spawning
      if (spawnSlotEntry->SpawnedActorInstance.IsExplicitlyNull())
      {
         continue;
      }

      if (spawnSlotEntry->SpawnedActorInstance == actor)
      {
         return spawnSlotEntry;
      }
   }

   return nullptr;
}

const TArray<int32>& UTATContainerSpawnerComponent::_GetOrPopulateEnabledSpawnSlotIndices(const FRandomStream& randomStream)
{
   // Populate with slots if we haven't already
   if (_enabledSpawnSlotIndices.IsEmpty() && SpawnSlotEntries.Num() > 0)
   {
      // TODO: randomize
      const int32 slotsToSelect = LimitSlotsUsedForSpawning ? FMath::Clamp(NumEnabledSlots, 1, SpawnSlotEntries.Num()) : SpawnSlotEntries.Num();
      for (int32 slotIdx = 0; slotIdx < slotsToSelect; ++slotIdx)
      {
         _enabledSpawnSlotIndices.Add(slotIdx);
      }
   }

   return _enabledSpawnSlotIndices;
}

#if WITH_EDITOR
TArray<FString> UTATContainerSpawnerComponent::_ValidateSpawnSlots() const
{
   TArray<FString> slotErrors;

   // Make sure we have slot to spawn into
   if (SpawnSlotEntries.IsEmpty())
   {
      slotErrors.Add(FString::Printf(TEXT("[%s] | SpawnSlotEntries is empty!"), *GetName()));
   }

   // Make sure NumEnabledSlots is valid
   else if (LimitSlotsUsedForSpawning)
   {
      if (NumEnabledSlots > SpawnSlotEntries.Num() || NumEnabledSlots < 1)
      {
         slotErrors.Add(FString::Printf(TEXT("[%s] | Invalid NumEnabledSlots = %d! \
Set NumEnabledSlots a value between 1 and size of SpawnSlotEntries (%d), or set SpawnType = Disabled")
            , *GetName()
            , NumEnabledSlots
            , SpawnSlotEntries.Num()));
      }
   }

   return slotErrors;
}
#endif //WITH_EDITOR
