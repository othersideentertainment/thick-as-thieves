// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

#include "TATHeightenedSenseTypes.generated.h"

class UAkStateValue;

UENUM()
enum class ETATHeightenedSenseEventType : uint8
{
   OnTagAdded,
   OnAttributeChanged,
   OnGameplayEvent
};

USTRUCT(BlueprintType)
struct TAT_API FTATHeightenedSensesEntry
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   ETATHeightenedSenseEventType EventType = ETATHeightenedSenseEventType::OnTagAdded;

   // Ak State will be updated when any of these tags are added to the player (or any of these gameplay events fire)
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType == ETATHeightenedSenseEventType::OnTagAdded || EventType == ETATHeightenedSenseEventType::OnGameplayEvent", EditConditionHides))
   FGameplayTagContainer TagContainer;

   // If true, all tags in TagContainer must be present on the owner for sense to activate
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType == ETATHeightenedSenseEventType::OnTagAdded", EditConditionHides))
   bool AllTagsRequired = false;

   // Presence of any of these tags on the player prevents sense from activating (disables sense if already pactiveresent)
   UPROPERTY(EditAnywhere)
   FGameplayTagContainer BlockedTags;

   // Ak State will be updated when this attribute changes
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType == ETATHeightenedSenseEventType::OnAttributeChanged", EditConditionHides))
   FGameplayAttribute Attribute;

   UPROPERTY(EditAnywhere)
   UAkStateValue* AkState = nullptr;

   // How long to wait before applying the Ak State
   UPROPERTY(EditAnywhere, meta = (UIMin = 0.f, ClampMin = 0.f))
   float ApplyAfterSeconds = 0.f;

   // How long the Ak State should last once applied. 0 = infinite duration
   // NOTE: ignored for EventType == OnTagAdded (instead cancelled when tag removed)
   UPROPERTY(EditAnywhere, meta = (UIMin = 0.f, ClampMin = 0.f, EditCondition = "EventType != ETATHeightenedSenseEventType::OnTagAdded"))
   float DurationSeconds = 0.f;

   FDelegateHandle GameplayEventDelegateHandle;

   FORCEINLINE bool operator==(const FTATHeightenedSensesEntry& other) const 
   {
      if (other.EventType != EventType)
      {
         return false;
      }

      switch (EventType)
      {
      case ETATHeightenedSenseEventType::OnAttributeChanged:
         return other.Attribute == Attribute;
      case ETATHeightenedSenseEventType::OnTagAdded: // OnTagAdded ONLY
         if (other.AllTagsRequired != AllTagsRequired)
         {
            return false;
         }
      case ETATHeightenedSenseEventType::OnGameplayEvent: // OnTagAdded + OnGameplayEvent
         return other.TagContainer == TagContainer && other.BlockedTags == BlockedTags;
      default:
         checkNoEntry();
         return false;
      }
   }

public:
   FString ToString() const;
#if WITH_EDITOR
   bool IsDataValid(FDataValidationContext& context) const;
#endif // WITH_EDITOR

   FORCEINLINE bool HasTag(FGameplayTag tag) const
   {
      switch (EventType)
      {
      case ETATHeightenedSenseEventType::OnAttributeChanged:
         return false;
      case ETATHeightenedSenseEventType::OnGameplayEvent:
      case ETATHeightenedSenseEventType::OnTagAdded:
         return TagContainer.HasTag(tag) || BlockedTags.HasTag(tag);
      default:
         checkNoEntry();
         return false;
      }
   }
   FORCEINLINE bool HasAttribute(FGameplayAttribute attribute) const
   {
      switch (EventType)
      {
      case ETATHeightenedSenseEventType::OnAttributeChanged:
         return Attribute == attribute;
      case ETATHeightenedSenseEventType::OnGameplayEvent:
      case ETATHeightenedSenseEventType::OnTagAdded:
         return false;
      default:
         checkNoEntry();
         return false;
      }
   }
};

USTRUCT()
struct TAT_API FTATHeightenedSenseEventHistory
{
   GENERATED_BODY()

   FTATHeightenedSenseEventHistory() {}
   FTATHeightenedSenseEventHistory(const FTATHeightenedSensesEntry& senseEntry) : HeightenedSensesEntry(senseEntry){}
   
   UPROPERTY(Transient)
   FTATHeightenedSensesEntry HeightenedSensesEntry;

   float EntryAddedTimestamp = 0.f;

   // Bound to timer responsible for calling _OnInitialDelayElapsed() when ApplyAfterSeconds has elapsed
   FTimerHandle ApplyAfterSecondsHandle;
   // Bound to timer responsible for calling _OnDurationElapsed() when DurationSeconds has elapsed
   FTimerHandle DurationHandle;

   // Returns true if the entry's DurationSeconds has elapsed since the time of application
   bool DurationElapsed(const UObject* contextObj) const;
   // Returns true if the entry's ApplyAfterSeconds has elapsed since EntryAddedTimestamp
   bool InitialDelayElapsed(const UObject* contextObj) const;

   FString ToString() const;
};
