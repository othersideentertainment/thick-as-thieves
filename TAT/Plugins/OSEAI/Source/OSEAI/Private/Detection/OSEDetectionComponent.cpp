// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Detection/OSEDetectionComponent.h"

// UE
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDetectionComponent)

UOSEDetectionComponent::UOSEDetectionComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   SetIsReplicatedByDefault(true);
}

float UOSEDetectionComponent::GetDetectionValueForPlayer(const AActor* actor) const
{
   if (const FOSECharacterDetectionData* entry = _GetPlayerDetectionEntry(actor))
   {
      return entry->DetectionValue;
   }
   return static_cast<float>(INDEX_NONE);
}

EActorDetectionState UOSEDetectionComponent::GetDetectionStateForPlayer(const AActor* actor) const
{
   if (const FOSECharacterDetectionData* entry = _GetPlayerDetectionEntry(actor))
   {
      return entry->DetectionState;
   }
   return EActorDetectionState::Observing;
}

bool UOSEDetectionComponent::GetHighestDetectionValue(float& highestDetectionValue, AActor*& highestDetectionActor) const
{
   highestDetectionValue = static_cast<float>(INDEX_NONE);
   highestDetectionActor = nullptr;
   for(const FOSECharacterDetectionData& detectionEntry : _actorDetectionEntries)
   {
      if (detectionEntry.DetectionValue > highestDetectionValue)
      {
         // blech, out param can't be const due to ue4 header parser...
         highestDetectionActor = const_cast<AActor*>(detectionEntry.Actor.Get());
         highestDetectionValue = detectionEntry.DetectionValue;
      }
   }
   return highestDetectionActor != nullptr;
}

void UOSEDetectionComponent::GetVisibilityStateForPlayer(const AActor* actor, bool& isVisible, float& timeSinceLastVisible) const
{
   isVisible = false;
   timeSinceLastVisible = static_cast<float>(INDEX_NONE);

   if (const FOSECharacterDetectionData* entry = _GetPlayerDetectionEntry(actor))
   {
      isVisible = entry->IsVisible;
      
      if (!isVisible && FMath::IsNearlyEqual(entry->LocalVisibilityChangedTimestamp,static_cast<float>(INDEX_NONE)))
      {
         const float now = GetWorld()->GetTimeSeconds();
         timeSinceLastVisible = (now - entry->LocalVisibilityChangedTimestamp);
      }
   }
}

FOSECharacterDetectionData& UOSEDetectionComponent::_FindOrCreatePlayerDetectionEntry(AActor* actor)
{
   FOSECharacterDetectionData* actorDetectionEntry = _actorDetectionEntries.FindByKey(actor);
   if (!actorDetectionEntry)
   {
      actorDetectionEntry = &_actorDetectionEntries.Emplace_GetRef(FOSECharacterDetectionData(actor));
      actor->OnEndPlay.AddUniqueDynamic(this, &ThisClass::_OnDetectedPlayerActorEndPlay);

      // Virtual 'local player detection value changed' on authority (listen server)
      if (GetOwner()->HasAuthority())
      {
         if (_IsActorLocalCharacter(actor))
         {
            // Local character detection: 'not seen' -> 'not detected'
            _OnLocalPlayerDetectionValueChanged(-1.0f, 0.0f);
         }
         _OnPlayerDetectionValueChanged(-1.0f, 0.0f);
      }
   }

   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _actorDetectionEntries, this);

   check(actorDetectionEntry);
   return *actorDetectionEntry;
}

void UOSEDetectionComponent::_ClearPlayerDetectionEntry(const AActor* actor)
{
   const int idx = _actorDetectionEntries.Find(actor);
   if (idx != INDEX_NONE)
   {
      const_cast<AActor*>(actor)->OnEndPlay.RemoveAll(this);
      const float oldValue = _actorDetectionEntries[idx].DetectionValue;
      _actorDetectionEntries.RemoveAt(idx);

      // Virtual 'local player detection value changed' on authority (listen server)
      if (GetOwner()->HasAuthority())
      {
         if (_IsActorLocalCharacter(actor))
         {
            // Local character detection: 'previous' -> 'not seen'
            _OnLocalPlayerDetectionValueChanged(oldValue, -1.0f);
         }
         _OnPlayerDetectionValueChanged(oldValue, -1.0f);
      }

      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _actorDetectionEntries, this);
   }
}

FOSECharacterDetectionData* UOSEDetectionComponent::_GetPlayerDetectionEntry(const AActor* actor)
{
   if (!actor)
   {
      return nullptr;
   }

   FOSECharacterDetectionData* actorDetectionEntry = _actorDetectionEntries.FindByKey(actor);
   if (!actorDetectionEntry)
   {
      return nullptr;
   }

   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _actorDetectionEntries, this);

   return actorDetectionEntry;
}

const FOSECharacterDetectionData* UOSEDetectionComponent::_GetPlayerDetectionEntry(const AActor* actor) const
{
  return _actorDetectionEntries.FindByKey(actor);
}

bool UOSEDetectionComponent::_IsActorLocalCharacter(const AActor* actor)
{
   // Returns true if the actor is valid and is a local player
   if (const APawn* pawn = Cast<const APawn>(actor))
   {
      return pawn->IsLocallyControlled();
   }
   return false;
}

void UOSEDetectionComponent::ClearDetectionEntries()
{
   if (GetOwner()->HasAuthority())
   {
      for (const FOSECharacterDetectionData& entry : _actorDetectionEntries)
      {
         if (entry.DetectionValue != -1.0f)
         {
            if (_IsActorLocalCharacter(entry.Actor.Get()))
            {
               // Local character detection: 'previous' -> 'not seen'
               _OnLocalPlayerDetectionValueChanged(entry.DetectionValue, -1.0f);
            }
            _OnPlayerDetectionValueChanged(entry.DetectionValue, -1.0f);
         }         
      }
   }

   _actorDetectionEntries.Empty();
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _actorDetectionEntries, this);
}

void UOSEDetectionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   // Could benefit from fast array serialization based on OnRep logic
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _actorDetectionEntries, params);
}

void UOSEDetectionComponent::_OnLocalPlayerDetectionValueChanged(float oldDetectionValue, float newDetectionValue)
{
   if(OnLocalPlayerDetectionValueChanged.IsBound())
   {
      OnLocalPlayerDetectionValueChanged.Execute(oldDetectionValue, newDetectionValue);
   }
}


void UOSEDetectionComponent::AuthorityUpdatePlayerActorVisibility(AActor* actor, const bool isVisible)
{
   check(GetOwner());
   check(GetOwner()->HasAuthority());
   
   if (!IsValid(actor))
      return;

   FOSECharacterDetectionData& entry = _FindOrCreatePlayerDetectionEntry(actor);
   if (entry.IsVisible != isVisible)
   {
      entry.IsVisible = isVisible;
      entry.LocalVisibilityChangedTimestamp = GetWorld()->GetTimeSeconds();
   }
}

void UOSEDetectionComponent::_OnDetectedPlayerActorEndPlay(AActor* actor, EEndPlayReason::Type endPlayReason)
{
   _ClearPlayerDetectionEntry(actor);
}

void UOSEDetectionComponent::AuthorityUpdatePlayerActorDetectionValue(AActor* actor, const EActorDetectionState state, const float detectionValue)
{
   check(GetOwner());
   check(GetOwner()->HasAuthority());

   if (!IsValid(actor))
      return;

   FOSECharacterDetectionData& entry = _FindOrCreatePlayerDetectionEntry(actor);
   if (entry.DetectionState != state || entry.DetectionValue != detectionValue)
   {
      const float oldValue = entry.DetectionValue;
      const EActorDetectionState oldState = entry.DetectionState;
      entry.DetectionValue = detectionValue;
      entry.DetectionState = state;

      // Virtual 'local player detection value changed'
      if (_IsActorLocalCharacter(entry.Actor.Get()))
      {
         // Local character detection: 'previous' -> 'new'
         _OnLocalPlayerDetectionValueChanged(oldValue, entry.DetectionValue);
      }

      if (oldValue != entry.DetectionValue)
      {
         _OnPlayerDetectionValueChanged(oldValue, entry.DetectionValue);
      }
   }
}


void UOSEDetectionComponent::_OnRep_ActorDetectionEntries(const TArray<FOSECharacterDetectionData>& oldDetectionEntries)
{
   // Find adds/update
   for (FOSECharacterDetectionData& entry : _actorDetectionEntries)
   {
      if (entry.Actor.IsValid())
      {
         const AActor* actor = entry.Actor.Get();

         const FOSECharacterDetectionData* oldEntry = oldDetectionEntries.FindByPredicate(
            [&](const FOSECharacterDetectionData& e)
            {
               return e.Actor == entry.Actor;
            });

         const float oldDetectionValue = oldEntry ? oldEntry->DetectionValue : static_cast<float>(INDEX_NONE);
         const EActorDetectionState oldDetectionState = oldEntry ? oldEntry->DetectionState : EActorDetectionState::Observing;

         // add/update detection value
         if (oldDetectionValue != entry.DetectionValue)
         {
            if (_IsActorLocalCharacter(actor))
            {
               _OnLocalPlayerDetectionValueChanged(oldDetectionValue, entry.DetectionValue);
            }

            _OnPlayerDetectionValueChanged(oldDetectionValue, entry.DetectionValue);
         }

         // add/update vis
         const float now = GetWorld()->GetTimeSeconds();
         if (oldEntry)
         {
            // if we have an old entry we can diff-check and update the timestamp locally
            if (oldEntry->IsVisible != entry.IsVisible)
            {
               entry.LocalVisibilityChangedTimestamp = now;
            }
         }
         else
         {
            // there was no old entry, so this is an add, consider this our first changed timestamp
            entry.LocalVisibilityChangedTimestamp = now;
         }
      }
   }

   // remove (currently unused, only add/update code paths are needed)
   for (const FOSECharacterDetectionData& entry : oldDetectionEntries)
   {
      if (!_actorDetectionEntries.ContainsByPredicate([&](const FOSECharacterDetectionData& e) { return e.Actor == entry.Actor; }))
      {
         // Check if actor is still valid, may be null/gc'd before the OnRep call.
         // Note: Due to how the detection entries are replicated, IsValid will never be 
         // true, because the old pointer is either gc'd or pending kill.
         // If the behavior of detection entries changes, this may be required in the
         // future.

         if (entry.Actor.IsValid())
         {
            if (_IsActorLocalCharacter(entry.Actor.Get()))
            {
               _OnLocalPlayerDetectionValueChanged(entry.DetectionValue, -1.0f);
            }
            _OnPlayerDetectionValueChanged(entry.DetectionValue, -1.0f);
         }
      }
   }
}
