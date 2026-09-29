// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "WorldMap/TATMapActorComponent.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "WorldMap/TATWorldMapSubsystem.h"

// ue
#include "Engine/AssetManager.h"
#include "GameFramework/Actor.h"
#include "Misc/DataValidation.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMapActorComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATMapActor, Log, All);


UTATMapActorComponent::UTATMapActorComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;

   // Default to auto-activation so component instances self-register w/ map subsystem by default
   bAutoActivate = true;
}

#if WITH_EDITOR
EDataValidationResult UTATMapActorComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   const bool hasSprite = _mapRepresentationData.DefaultSpriteTag.IsValid();
   const bool hasLabel = !MapLabel.IsEmptyOrWhitespace();

   // Require a sprite or a label (or both, I suppose)
   if (!hasSprite && !hasLabel)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] does not have a valid DefaultSpriteTag or a MapLabel!"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   // DefaultSpriteTag doesn't correspond to map sprite entry
   if (hasSprite && _mapSpriteDataAsset && !_mapSpriteDataAsset->SpriteTable.Contains(_mapRepresentationData.DefaultSpriteTag))
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has DefaultSpriteTag %s which doesn't match any entries in MapSpriteTable! Please add an entry with corresponding tag")
         , *GetName()
         , *_mapRepresentationData.DefaultSpriteTag.ToString())));

      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif // WITH_EDITOR

void UTATMapActorComponent::Activate(bool bAutoReset)
{
   Super::Activate(bAutoReset);

   // Early out if we've already registered with the world subsystem - otherwise we could stomp pre-existing state 
   // (eg. overridden _mapRepresentedActor / _currentSpriteTag)
   if (const UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
   {
      if (worldMapSubsystem->IsMapActorRegistered(this))
      {
         return;
      }
   }

   _mapRepresentedActor = GetOwner();
   _currentSpriteTag = _mapRepresentationData.DefaultSpriteTag;
   if (const APawn* mapPawn = Cast<const APawn>(_mapRepresentedActor))
   {
      if (!mapPawn->IsLocallyControlled())
      {
         // If this is a pawn that is not locally controlled, use the alternative tag instead.
         if (_mapRepresentationData.AlternativeRemoteSpriteTag.IsValid())
         {
            _currentSpriteTag = _mapRepresentationData.AlternativeRemoteSpriteTag;
            _mapRepresentationData.GeneralAreaColor = _mapRepresentationData.AlternativeGeneralAreaColor;
         }
      }
      else
      {
         // If this is a pawn that is locally controlled, set the general area to on and set the distance to the max visible range
         _mapRepresentationData.IsGeneralArea = true;
         _mapRepresentationData.ShowGeneralAreaOnQuestLootObtained = false;
         const UTATProjectSettings& tatSettings = UTATProjectSettings::Get();
         _mapRepresentationData.GeneralAreaRadius = tatSettings.MaxDistanceToVisibleMapAndCompassActors;
      }
   }
   _currentMapLabel = MapLabel;

   TSoftObjectPtr<UTATMapSpriteDataAsset> mapSpriteData = UTATProjectSettings::Get().MapSpriteData;
   TWeakObjectPtr<UTATMapActorComponent> weakThis(this);
   UAssetManager::GetStreamableManager().RequestAsyncLoad(mapSpriteData.ToSoftObjectPath(), [weakThis, mapSpriteData]
   {
      if (UTATMapActorComponent* self = weakThis.Get())
      {
         self->_mapSpriteDataAsset = mapSpriteData.Get();
            
         if (self->IsActive())
         {
            // Register self with world subsystem
            self->_SetMapTrackingEnabled(true);
         }
      }
   });

   GetWorld()->GetTimerManager().SetTimer(_updateGeneralAreaTimerHandle, this, &UTATMapActorComponent::_UpdateMapLocationForGeneralArea, _mapRepresentationData.UpdateGeneralAreaFrequency, true, 0.0f);
}

void UTATMapActorComponent::Deactivate()
{
   _SetMapTrackingEnabled(false);

   GetWorld()->GetTimerManager().ClearTimer(_updateGeneralAreaTimerHandle);
   _updateGeneralAreaTimerHandle.Invalidate();

   Super::Deactivate();
}

void UTATMapActorComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   // Unregister from world subsystem before teardown
   _SetMapTrackingEnabled(false);

   Super::EndPlay(endPlayReason);
}

void UTATMapActorComponent::SetMapRepresentationData(const FTATMapRepresentationData& mapRepresentationData)
{
#if DO_CHECK
   if (const UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
   {
      check(!worldMapSubsystem->IsMapActorRegistered(this));
   }
#endif // DO_CHECK
   _mapRepresentationData = mapRepresentationData;
}

void UTATMapActorComponent::SetGeneralAreaVisible(bool bGeneralAreaVisual, bool bSetActive)
{
   if (bGeneralAreaVisual != _mapRepresentationData.IsGeneralArea)
   {
      const bool bWasActive = IsRegistered();
      if (bWasActive)
      {
         SetActive(false);
      }
      _mapRepresentationData.IsGeneralArea = bGeneralAreaVisual;
      if (bWasActive)
      {
         SetActive(true);
         return;
      }
   }

   if (bSetActive)
   {
      SetActive(true);
   }
}

void UTATMapActorComponent::SetRepresentedActor(const AActor* mapRepresentedActor)
{
   check(IsValid(mapRepresentedActor));

   // Broadcast change so UI can pull new actor's location/facing direction
   if (UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
   {
      _mapRepresentedActor = mapRepresentedActor;
      if (worldMapSubsystem->IsMapActorRegistered(this))
      {
         worldMapSubsystem->BroadcastMapActorChange(this);
      }
   }
}

void UTATMapActorComponent::UpdateMapSprite(const FGameplayTag spriteTag)
{
   if (!spriteTag.IsValid())
   {
      return;
   }

   if (!GetMapSpriteEntry(spriteTag))
   {
      UE_LOG(LogTATMapActor, Error, TEXT("%s | UpdateMapSprite() called with spriteTag not present in FTATMapRepresentationData::MapSpriteTable!"), *GetOwner()->GetName());
      return;
   }

   if (UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
   {
      // Cache tag
      _currentSpriteTag = spriteTag;

      // Notify subsystem if registered (otherwise log error)
      if (!worldMapSubsystem->IsMapActorRegistered(this))
      {
         UE_LOG(LogTATMapActor, Error, TEXT("%s | UpdateMapSprite() called on unregistered UTATMapActorComponent! Did you forget to call Activate()?"), *GetOwner()->GetName());
         return;
      }
      worldMapSubsystem->BroadcastMapActorChange(this);
   }
}

void UTATMapActorComponent::UpdateMapLabel(const FText& newLabel)
{
   if (UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
   {
      // Cache tag
      _currentMapLabel = newLabel;

      // Notify subsystem if registered (otherwise log error)
      if (!worldMapSubsystem->IsMapActorRegistered(this))
      {
         UE_LOG(LogTATMapActor, Error, TEXT("%s | UpdateMapLabel() called on unregistered UTATMapActorComponent! Did you forget to call Activate()?"), *GetOwner()->GetName());
         return;
      }
      worldMapSubsystem->BroadcastMapActorChange(this);
   }
}

FVector UTATMapActorComponent::GetActorLocation() const
{
   if (!_mapRepresentedActor.IsValid())
   {
      UE_LOG(LogTATMapActor, Warning, TEXT("%s GetActorLocation() | _mapRepresentedActor is null!"), *GetOwner()->GetName());
      return FVector::Zero();
   }
   const AActor* mapRepresentedActor = _mapRepresentedActor.Get();
   return mapRepresentedActor->GetActorLocation();
}

FVector UTATMapActorComponent::GetMapLocation() const
{
   FVector mapLocation;
   if (_mapRepresentationData.ShowGeneralAreaOnQuestLootObtained && _mapRepresentationData.IsGeneralArea)
   {
      mapLocation = _savedMapLocationForGeneralArea;
   }
   else
   {
      mapLocation = GetActorLocation();
   }

   mapLocation += _mapLocationOffset;
   return mapLocation;
}

FVector2D UTATMapActorComponent::GetActorFacingDirection() const
{
   if (!_mapRepresentedActor.IsValid())
   {
      UE_LOG(LogTATMapActor, Warning, TEXT("%s GetActorFacingDirection() | _mapRepresentedActor is null!"), *GetOwner()->GetName());
      return FVector2D::Zero();
   }
   const AActor* mapRepresentedActor = _mapRepresentedActor.Get();

   FVector actorLocation;
   FRotator actorRotation;
   mapRepresentedActor->GetActorEyesViewPoint(actorLocation, actorRotation);
   return FVector2D(actorRotation.Vector());
}

const FTATMapSpriteEntry* UTATMapActorComponent::GetCurrentMapSpriteEntry() const
{
   return GetMapSpriteEntry(_currentSpriteTag);
}

const FTATMapSpriteEntry* UTATMapActorComponent::GetMapSpriteEntry(FGameplayTag spriteTag) const
{
   if (!spriteTag.IsValid())
   {
      return nullptr;
   }
   if (!_mapSpriteDataAsset)
   {
      UE_LOG(LogTATMapActor, Error, TEXT("GetMapSprite() | _mapSpriteDataAsset not assigned! Could not perform sprite lookup"));
      return nullptr;
   }

   if (const FTATMapSpriteEntry* spriteEntry = _mapSpriteDataAsset->SpriteTable.FindByKey(spriteTag))
   {
      return spriteEntry;
   }

   UE_LOG(LogTATMapActor, Error, TEXT("GetMapSprite() | No entry with tag %s present in SpriteTable!"), *spriteTag.ToString());
   return nullptr;
}

const FText& UTATMapActorComponent::GetCurrentMapLabel() const
{
   return _currentMapLabel;
}

void UTATMapActorComponent::_SetMapTrackingEnabled(bool enabled)
{
   if (UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
   {
      if (enabled)
      {
         worldMapSubsystem->RegisterMapActor(this);
      }
      else
      {
         worldMapSubsystem->UnregisterMapActor(this);
      }
   }
}

void UTATMapActorComponent::_UpdateMapLocationForGeneralArea()
{
   _savedMapLocationForGeneralArea = GetActorLocation();
   _mapRepresentationData.GeneralAreaOffset = FMath::RandPointInCircle(_mapRepresentationData.GeneralAreaRadius);
}
