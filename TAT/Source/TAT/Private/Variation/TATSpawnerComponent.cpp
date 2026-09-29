// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/TATSpawnerComponent.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATExternalSpawnerDependencyInterface.h"
#include "Variation/TATSpawnData.h"
#include "WorldMap/TATSpawnerActivatedMapActor.h"
#include "Variation/TATSpawnerOwnerInterface.h"
#include "Variation/TATSpawnerRegistrySubsystem.h"

// ue5
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSpawnerComponent)

UTATSpawnerComponent::UTATSpawnerComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   SetIsReplicatedByDefault(false);
   bWantsInitializeComponent = true;
   _primarySpawnerForActor = true;
}

int32 UTATSpawnerComponent::GetMaxInstancesToSpawn() const
{
   return SpawnType == ETATSpawnChanceType::Disabled ? 0 : 1;
}

void UTATSpawnerComponent::AuthoritySpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   check(GetOwner()->HasAuthority());

   _AuthorityTryEnableSpawnerActivatedMapActor();
   
   // broadcast that we have been chosen to spawn.
   // TODO: Send along a payload w/ some mission context here so things like traps could change themselves up
   AuthorityOnSpawn.Broadcast(spawnContext, randomStream);
}

AActor* UTATSpawnerComponent::AuthoritySpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClass, const FRandomStream& randomStream)
{
   check(GetOwner()->HasAuthority());
   check(!actorClass.IsNull());

   // broadcast that we have been chosen to spawn.
   AActor* spawnActorInstance = nullptr;
   AuthorityOnSpawnActorDeferred.Broadcast(spawnContext, actorClass, randomStream, spawnActorInstance);
   return spawnActorInstance;
}

void UTATSpawnerComponent::AuthorityFinishSpawnActor(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClass, const FRandomStream& randomStream, AActor* actorInstance)
{
   check(GetOwner()->HasAuthority());
   check(!actorClass.IsNull());
   check(IsValid(actorInstance));

   if (actorInstance)
   {
      _AuthorityTryEnableSpawnerActivatedMapActor();
   }

   // broadcast that spawn should complete
   AuthorityOnFinishSpawnActor.Broadcast(spawnContext, actorClass, randomStream, actorInstance);
}

void UTATSpawnerComponent::AuthorityNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   check(GetOwner()->HasAuthority());

   // broadcast that we have been chosen to NOT spawn
   AuthorityOnNotSpawn.Broadcast(spawnContext, randomStream);
}

FString UTATSpawnerComponent::GetSpawnGroupDebugString() const
{
   FString debugString;
   switch (SpawnType)
   {
   case ETATSpawnChanceType::UseSpawnGroup:
      {
         // remove "MapVariation.SpawnGroup"
         FString groupStr = SpawnGroup.ToString();
         groupStr.RemoveFromStart(TEXT("MapVariation.SpawnGroup."));
         debugString = groupStr;
      }
      break;
   case ETATSpawnChanceType::Always:
      {
         debugString = TEXT("Spawn Always");
      }
      break;
   case ETATSpawnChanceType::PercentChance:
      {
         debugString = FString::Printf(TEXT("%.02f%% Spawn Chance"), SpawnChancePercent);
      }
      break;
   case ETATSpawnChanceType::Disabled:
      {
         debugString = TEXT("Spawn Disabled");
      }
      break;
   default:
      unimplemented();
   }
   return debugString;
}

FString UTATSpawnerComponent::GetBucketDebugString() const
{
   FString bucketStr;
   if(SpawnBucket)
   {
      bucketStr = SpawnBucket->DebugName;
   }
   return bucketStr;
}

const FText& UTATSpawnerComponent::GetClueLocationName() const
{
   return _clueLocationText;
}

void UTATSpawnerComponent::InitializeComponent()
{
   Super::InitializeComponent();

   if (GetOwner()->HasAuthority())
   {
      if (UTATSpawnerRegistrySubsystem* spawnerRegistry = GetWorld()->GetSubsystem<UTATSpawnerRegistrySubsystem>())
      {
         spawnerRegistry->AuthorityRegisterSpawner(this);
      }
   }
}

#if WITH_EDITOR
void UTATSpawnerComponent::ValidateSpawnerProperties(FMessageLog& msgLog) const
{
   auto createObjectToken = [](const UTATSpawnerComponent* spawner) {
      return FUObjectToken::Create(spawner->GetOwner(), FText::FromString(spawner->GetReadableName()));
   };

   switch (SpawnType)
   {
   case ETATSpawnChanceType::UseSpawnGroup:
      {
         if (!SpawnGroup.IsValid())
         {
            msgLog.Error()
               ->AddToken(createObjectToken(this))
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Invalid SpawnGroup tag \"%s\"."), *SpawnGroup.ToString()))));
         }
      }
      break;
   case ETATSpawnChanceType::Always:
      {
      }
      break;
   case ETATSpawnChanceType::PercentChance:
      {
         if (SpawnChancePercent < 0.0f || SpawnChancePercent > 100.0f)
         {
            msgLog.Error()
               ->AddToken(createObjectToken(this))
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("SpawnChancePercent must between 0.0f and 100.0f.")))));
         }
      }
      break;
   case ETATSpawnChanceType::Disabled:
      {
      }
      break;
   default:
      unimplemented();
   }

   if (SpawnType != ETATSpawnChanceType::Disabled)
   {
      _sceneRequirement.ValidateRequirement(msgLog, GetOwner(), [this, createObjectToken]() { return createObjectToken(this); });

      if(UTATProjectSettings::Get().RequireClueLocationTags && _clueMode != ETATSpawnerClueMode::Never)
      {
         if(!_clueLocationTag.IsValid())
         {
            msgLog.Error()
                  ->AddToken(createObjectToken(this))
                  ->AddToken(FTextToken::Create(INVTEXT("Clue-relevant spawner does not have clue location tag")));
         }

         if(_clueLocationText.IsEmpty())
         {
            msgLog.Error()
                  ->AddToken(createObjectToken(this))
                  ->AddToken(FTextToken::Create(INVTEXT("Clue-relevant spawner does not have clue location text")));
         }
      }
      if(SpawnBucketBehavior == ETATSpawnBucketBehavior::Required && SpawnBucket == nullptr)
      {
         msgLog.Error()
                  ->AddToken(createObjectToken(this))
                  ->AddToken(FTextToken::Create(INVTEXT("Spawner is set to require bucket, but has none assigned")));
      }
      else if(SpawnBucketBehavior == ETATSpawnBucketBehavior::Never && SpawnBucket != nullptr)
      {
         msgLog.Error()
                  ->AddToken(createObjectToken(this))
                  ->AddToken(FTextToken::Create(INVTEXT("Spawner is set to never use a bucket, but has one assigned")));
      }
   }

   if (_primarySpawnerForActor && GetOwner() && GetOwner()->Implements<UTATSpawnerOwnerInterface>())
   {
      GetOwner()->ForEachComponent<UTATSpawnerComponent>(false, [this, &msgLog, createObjectToken](const UTATSpawnerComponent* spawner)
         {
            if (spawner != this && spawner->IsPrimarySpawnerForActor())
            {
               msgLog.Error()
                  ->AddToken(createObjectToken(this))
                  ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Actor %s has multiple primary spawners, set only one to primary %s + %s"), *GetOwner()->GetActorNameOrLabel(), *GetName(), *spawner->GetName()))));
            }
         });
   }

   if (_parentRequirement != ETATSpawnerDependencyType::None)
   {
      if (_parentSpawner == nullptr)
      {
         msgLog.Warning()
            ->AddToken(createObjectToken(this))
            ->AddToken(FTextToken::Create(FText::FromString(TEXT("Spawner has dependency, but does not specify actor"))));
         return;
      }

      const FTATSpawnerParent parent = GetParent();
      if (!parent.IsValid())
      {
         msgLog.Warning()
            ->AddToken(createObjectToken(this))
            ->AddToken(FTextToken::Create(FText::FromString(TEXT("Could not find spawner in parent spawner actor "))))
            ->AddToken(FUObjectToken::Create(_parentSpawner, FText::FromString(_parentSpawner->GetActorNameOrLabel())));
      }
      else if(const UTATSpawnerComponent* parentSpawnerComponent = parent.Spawner)
      {
         if (GetSpawnType() == ETATSpawnChanceType::UseSpawnGroup)
         {
            if (parentSpawnerComponent->GetSpawnType() != ETATSpawnChanceType::UseSpawnGroup && parentSpawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled)
            {
               msgLog.Warning()
                  ->AddToken(createObjectToken(this))
                  ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("cannot depend on non-spawn-group spawner %s from a spawn-group spawner"), *_parentSpawner->GetActorNameOrLabel()))))
                  ->AddToken(createObjectToken(parentSpawnerComponent));
            }
            else if (parentSpawnerComponent->GetSpawnType() == ETATSpawnChanceType::UseSpawnGroup)
            {
               const ATATWorldSettings& worldSettings = ATATWorldSettings::Get(this);
               if (const UTATSpawnDataAsset* spawnData = worldSettings.SpawnData)
               {
                  const int ourGroupIndex = spawnData->FindGroupIndex(GetSpawnGroupTag());
                  const int suppressingGroupIndex = spawnData->FindGroupIndex(parentSpawnerComponent->GetSpawnGroupTag());

                  if (ourGroupIndex <= suppressingGroupIndex)
                  {
                     msgLog.Warning()
                        ->AddToken(createObjectToken(this))
                        ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("parent spawner %s's spawn group %s does not spawn before %s"), *_parentSpawner->GetActorNameOrLabel(), *parentSpawnerComponent->GetSpawnGroupDebugString(), *GetSpawnGroupDebugString()))))
                        ->AddToken(createObjectToken(parentSpawnerComponent));
                  }
               }
            }
         }
      }
   }
}

void UTATSpawnerComponent::OnRegister()
{
   Super::OnRegister();

   // Set spawner for editor-vis purposes
   if (_parentRequirement != ETATSpawnerDependencyType::None)
   {
      _SetEditorVisSpawnerDependency(_GetParentActor());
   }
}

void UTATSpawnerComponent::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   const FName propertyName = (propertyChangedEvent.Property != nullptr) ? propertyChangedEvent.Property->GetFName() : NAME_None;

   if (propertyName == GET_MEMBER_NAME_CHECKED(UTATSpawnerComponent, _parentRequirement) ||
       propertyName == GET_MEMBER_NAME_CHECKED(UTATSpawnerComponent, _parentSpawner))
   {
      _SetEditorVisSpawnerDependency(_GetParentActor());
   }
}

void UTATSpawnerComponent::CheckForErrors()
{
   Super::CheckForErrors();

   FMessageLog msgLog("MapCheck");
   ValidateSpawnerProperties(msgLog);
}

void UTATSpawnerComponent::_SetEditorVisSpawnerDependency(AActor* dependency)
{
   if (UTATSpawnerRegistrySubsystem* spawnerRegistry = GetWorld()->GetSubsystem<UTATSpawnerRegistrySubsystem>())
   {
      spawnerRegistry->SetEditorVisSpawnerDependency(this, dependency);
   }
}
#endif // WITH_EDITOR

AActor* UTATSpawnerComponent::_GetParentActor() const
{
   if (_parentRequirement == ETATSpawnerDependencyType::None)
   {
      return nullptr;
   }

   return _parentSpawner;
}

UTATSpawnerComponent* UTATSpawnerComponent::GetParentSpawner() const
{
   return GetParent().Spawner;
}

FTATSpawnerParent UTATSpawnerComponent::GetParent() const
{
   if (_parentRequirement == ETATSpawnerDependencyType::None)
   {
      return FTATSpawnerParent();
   }

   if (_parentSpawner == nullptr)
   {
      return FTATSpawnerParent();
   }

   if (_parentSpawner->Implements<UTATExternalSpawnerDependencyInterface>())
   {
      return FTATSpawnerParent(_parentSpawner);
   }

   // This was originally just called an interface method, but since it was
   // bp-implementable (since spawners can be added in BP), it ran into issues
   // being called in editor contexts (you can opt-in, but even that causes
   // issues in post-load)
   for (UActorComponent* component : _parentSpawner->GetComponents())
   {
      UTATSpawnerComponent* spawner = Cast<UTATSpawnerComponent>(component);
      if (spawner && spawner->IsPrimarySpawnerForActor())
      {
         return FTATSpawnerParent(spawner);
      }
   }

   return FTATSpawnerParent();
}

void UTATSpawnerComponent::_AuthorityTryEnableSpawnerActivatedMapActor()
{
   check(GetOwner()->HasAuthority());

   // Enable tracking for associated map actor if necessary
   if (_isSpawnedActorMapTracked)
   {
      if (IsValid(_spawnerActivatedMapActor))
      {
         _spawnerActivatedMapActor->AuthoritySetMapTrackingEnabled(true);
      }
      else
      {
         UE_LOG(LogTATMapVariation, Error, TEXT("%s | AuthorityFinishSpawnActor() missing expected reference to _spawnerActivatedMapActor!"), *GetReadableName());
      }
   }
}

FString FTATSpawnerParent::ToString() const
{
   if (Spawner)
   {
      return Spawner->GetReadableName();
   }
   else if (ExternalSpawner)
   {
      return ExternalSpawner->GetActorNameOrLabel();
   }
   else
   {
      return TEXT("None");
   }
}
