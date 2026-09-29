// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/TATActorSpawner.h"

// tat
#include "Variation/TATSpawnData.h"
#include "Variation/TATSpawnerComponent.h"

// ose

// ue4
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/ShapeComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActorSpawner)

DEFINE_LOG_CATEGORY(LogTATSpawner);

ATATActorSpawner::ATATActorSpawner()
   : Super()
{
   PrimaryActorTick.bCanEverTick = false;
   PrimaryActorTick.bStartWithTickEnabled = false;
   bReplicates = false;
   bNetLoadOnClient = false; // this is a server-only object that spawns in the actual thing we care about
   bCollideWhenPlacing = true;
   SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
   RootComponent->SetVisibleFlag(false);
   RootComponent->SetMobility(EComponentMobility::Static);

   _spawnerComponent = CreateDefaultSubobject<UTATSpawnerComponent>(TEXT("VariationSpawner"));
   _spawnerComponent->SetSpawnBucketBehavior(ETATSpawnBucketBehavior::Required);

   _goodSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("GoodSprite"));
   _badSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("BadSprite"));

#if WITH_EDITORONLY_DATA
   _arrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));

   if (!IsRunningCommandlet())
   {
      // Structure to hold one-time initialization
      struct FConstructorStatics
      {
         ConstructorHelpers::FObjectFinderOptional<UTexture2D> ActorSpawnerTextureObject;
         ConstructorHelpers::FObjectFinderOptional<UTexture2D> BadPlacementTextureObject;
         FName ID_ActorSpawner;
         FText NAME_ActorSpawner;
         FName ID_ActorSpawner_BadPlacement;
         FText NAME_ActorSpawner_BadPlacement;
         FConstructorStatics()
            : ActorSpawnerTextureObject(TEXT("/Engine/EditorResources/S_Actor")) // default, can override with EditorSettings struct
            , BadPlacementTextureObject(TEXT("/Game/Editor/Icons/BadPlacement")) // default, can override with EditorSettings struct
            , ID_ActorSpawner(TEXT("ActorSpawner"))
            , NAME_ActorSpawner(NSLOCTEXT("SpriteCategory", "ActorSpawner", "Actor Spawner"))
            , ID_ActorSpawner_BadPlacement(TEXT("ActorSpawner_BadPlacement"))
            , NAME_ActorSpawner_BadPlacement(NSLOCTEXT("SpriteCategory", "ActorSpawner_BadPlacement", "Actor Spawner BadPlacement"))
         {
         }
      };
      static FConstructorStatics sConstructorStatics;

      // default icons
      EditorSettings.GoodIcon = sConstructorStatics.ActorSpawnerTextureObject.Get();
      EditorSettings.BadIcon = sConstructorStatics.BadPlacementTextureObject.Get();

      if (_goodSprite)
      {
         _goodSprite->Sprite = sConstructorStatics.ActorSpawnerTextureObject.Get();
         _goodSprite->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
         _goodSprite->bHiddenInGame = true;
         _goodSprite->SpriteInfo.Category = sConstructorStatics.ID_ActorSpawner;
         _goodSprite->SpriteInfo.DisplayName = sConstructorStatics.NAME_ActorSpawner;
         _goodSprite->SetupAttachment(RootComponent);
         _goodSprite->SetUsingAbsoluteScale(true);
         _goodSprite->bIsScreenSizeScaled = true;
      }

      if (_badSprite)
      {
         _badSprite->Sprite = sConstructorStatics.BadPlacementTextureObject.Get();
         _goodSprite->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
         _badSprite->bHiddenInGame = true;
         _badSprite->SpriteInfo.Category = sConstructorStatics.ID_ActorSpawner_BadPlacement;
         _badSprite->SpriteInfo.DisplayName = sConstructorStatics.NAME_ActorSpawner_BadPlacement;
         _badSprite->SetUsingAbsoluteScale(true);
         _badSprite->SetupAttachment(RootComponent);
         _badSprite->bIsScreenSizeScaled = true;
      }

      if (_badSprite)
      {
         _badSprite->SetVisibility(false);
      }

      if (_arrowComponent)
      {
         _arrowComponent->ArrowColor = FColor(235, 64, 52);
         _arrowComponent->ArrowSize = 1.0f;
         _arrowComponent->bTreatAsASprite = true;
         _arrowComponent->SpriteInfo.Category = sConstructorStatics.ID_ActorSpawner;
         _arrowComponent->SpriteInfo.DisplayName = sConstructorStatics.NAME_ActorSpawner;
         _arrowComponent->SetupAttachment(RootComponent);
         _arrowComponent->bIsScreenSizeScaled = true;
      }
   }

   _validSpawnChanceTypes = { ETATSpawnChanceType::UseSpawnGroup,
                              ETATSpawnChanceType::Always,
                              ETATSpawnChanceType::PercentChance
   };
#endif // WITH_EDITORONLY_DATA
}

void ATATActorSpawner::AuthoritySpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   // subclasses can handle a spawn w/o an actor class here
   AuthorityOnSpawn(spawnContext, randomStream);
}

AActor* ATATActorSpawner::AuthoritySpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, TSubclassOf<AActor> spawnClass, const FRandomStream& randomStream)
{
   check(HasAuthority());
   check(spawnClass);

   // spawn the actor in C++, but deferred
   const FTransform& spawnXfm = GetActorTransform();
   AActor* owner = nullptr;
   APawn* instigator = nullptr;
   ESpawnActorCollisionHandlingMethod collisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   AActor* actor = GetWorld()->SpawnActorDeferred<AActor>(spawnClass, spawnXfm, owner, instigator, collisionHandlingOverride);
   if (actor)
   {
      _spawnedActor = actor;

      // let blueprints setup instance properties that may need to be replicated before we finish spawning
      AuthorityOnActorSpawnedDeferred(spawnContext, actor, randomStream);

      // NOTE: We'll finish spawning when we get the callback, we may need to apply modifiers and other state to the actor before completing this spawn
   }
   else
   {
      UE_LOG(LogTATSpawner, Error, TEXT("Spawner %s failed to spawn an actor with subclass %s!"), *GetName(), *spawnClass->GetName());
   }

   // TODO: Destroy ourselves once we're done?  We'd cause extra garbage collection, and
   // this object shouldn't take up all that much memory, but we could...?
   return actor;
}

void ATATActorSpawner::AuthorityFinishSpawnActor(const FTATVariationSpawnContext& spawnContext, TSubclassOf<AActor> spawnClass, const FRandomStream& randomStream, AActor* spawnedActorInstance)
{
   if (spawnedActorInstance)
   {
      check(_spawnedActor == spawnedActorInstance);
      _spawnedActor->FinishSpawning(_GetSpawnTransform());
      AuthorityOnActorSpawnFinished(spawnContext, _spawnedActor, randomStream);
   }
}

void ATATActorSpawner::AuthorityNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   // subclasses can handle a decision not to spawn this thing here
   AuthorityOnNotSpawn(spawnContext, randomStream);
}

#if WITH_EDITOR
void ATATActorSpawner::CheckForErrors()
{
   Super::CheckForErrors();

#if WITH_EDITORONLY_DATA
   ETATSpawnChanceType spawnChanceType = _spawnerComponent->GetSpawnType();
   if (!_validSpawnChanceTypes.Contains(spawnChanceType))
   {
      FMessageLog("MapCheck").Warning()
         ->AddToken(FUObjectToken::Create(this))
         ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("%s spawn type is set to %s but that is invalid for this spawner type!"), *GetName(), *UEnum::GetValueAsString(spawnChanceType)))));
   }
#endif // WITH_EDITORONLY_DATA
}
#endif // WITH_EDITOR

void ATATActorSpawner::PostInitializeComponents()
{
   Super::PostInitializeComponents();
   _ApplyEditorSettings();
   _spawnerComponent->AuthorityOnSpawnActorDeferred.AddUniqueDynamic(this, &ATATActorSpawner::_AuthorityOnSpawnerSpawnActorDeferred);
   _spawnerComponent->AuthorityOnFinishSpawnActor.AddUniqueDynamic(this, &ATATActorSpawner::_AuthorityOnSpawnerFinishSpawn);
   _spawnerComponent->AuthorityOnSpawn.AddUniqueDynamic(this, &ATATActorSpawner::_AuthorityOnSpawnerSpawn);
   _spawnerComponent->AuthorityOnNotSpawn.AddUniqueDynamic(this, &ATATActorSpawner::_AuthorityOnSpawnerNotSpawn);
}

void ATATActorSpawner::BeginPlay()
{
   Super::BeginPlay();
}

void ATATActorSpawner::OnConstruction(const FTransform& transform)
{
   Super::OnConstruction(transform);
   _ApplyEditorSettings();
}

#if WITH_EDITOR
void ATATActorSpawner::PostEditMove(bool finished)
{
   if (finished)
   {
      if (GetWorld()->IsNavigationRebuilt())
      {
         UE_LOG(LogTATSpawner, Log, TEXT("PostEditMove Clear paths rebuilt"));
      }

      // Validate collision
      _TryValidateCollision();
   }

   _ApplyEditorSettings();
   MarkComponentsRenderStateDirty();
   Super::PostEditMove(finished);
}

void ATATActorSpawner::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _ApplyEditorSettings();
}

void ATATActorSpawner::PostEditUndo()
{
   Super::PostEditUndo();

   _ApplyEditorSettings();

   // undo can move the object without calling post edit move, we need to validate on any movement
   _TryValidateCollision();
}
#endif // WITH_EDITOR

void ATATActorSpawner::_TryValidateCollision()
{
   if (_ShouldBeBased() && (_goodSprite || _badSprite))
   {
      // Validate collision
      _ValidateCollision();

      // Force update of icon(s)
      MarkComponentsRenderStateDirty();
   }
}

void ATATActorSpawner::_ApplyEditorSettings()
{
#if WITH_EDITOR
   if (_goodSprite)
   {
      _goodSprite->SetRelativeScale3D(EditorSettings.GoodIconScale);
      if (EditorSettings.GoodIcon)
         _goodSprite->Sprite = EditorSettings.GoodIcon;
   }

   if (_badSprite)
   {
      _badSprite->SetRelativeScale3D(EditorSettings.BadIconScale);
      if (EditorSettings.BadIcon)
         _badSprite->Sprite = EditorSettings.BadIcon;
   }

   if (_arrowComponent)
   {
      _arrowComponent->ArrowColor = EditorSettings.ForwardDirectionArrowColor;
   }
#endif
}

bool ATATActorSpawner::_ShouldBeBased() const
{
   APhysicsVolume* physicsVolume = _GetNavPhysicsVolume();
   return ((physicsVolume == NULL || !physicsVolume->bWaterVolume) && _GetShapeComponent());
}

APhysicsVolume* ATATActorSpawner::_GetNavPhysicsVolume() const
{
   if (_GetShapeComponent())
   {
      return _GetShapeComponent()->GetPhysicsVolume();
   }
   return GetWorld()->GetDefaultPhysicsVolume();
}

const FTransform& ATATActorSpawner::_GetSpawnTransform() const
{
   return GetActorTransform();
}

void ATATActorSpawner::_AuthorityOnSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   AuthoritySpawn(spawnContext, randomStream);
}

void ATATActorSpawner::_AuthorityOnSpawnerSpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClass, const FRandomStream& randomStream, AActor*& outSpawnedActorInstance)
{
   // TODO: Get actors preloaded somehow so this doesn't actually cause a CDO load ever
   
   if (UClass* classToSpawn = actorClass.LoadSynchronous())
   {
      outSpawnedActorInstance = AuthoritySpawnActorDeferred(spawnContext, classToSpawn, randomStream);
   }
   else
   {
      UE_LOG(LogTATSpawner, Log, TEXT("Failed to load class %s"), *actorClass.ToString());
   }
}

void ATATActorSpawner::_AuthorityOnSpawnerFinishSpawn(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClass, const FRandomStream& randomStream, AActor* spawnedActorInstance)
{
   if (UClass* classToSpawn = actorClass.LoadSynchronous())
   {
      AuthorityFinishSpawnActor(spawnContext, classToSpawn, randomStream, spawnedActorInstance);
   }
   else
   {
      UE_LOG(LogTATSpawner, Log, TEXT("Failed to load class %s"), *actorClass.ToString());
   }
}

void ATATActorSpawner::_AuthorityOnSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   AuthorityNotSpawn(spawnContext, randomStream);
}

