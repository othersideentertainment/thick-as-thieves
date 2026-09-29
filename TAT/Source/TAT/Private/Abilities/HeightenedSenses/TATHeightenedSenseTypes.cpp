// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/HeightenedSenses/TATHeightenedSenseTypes.h"

#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHeightenedSenseTypes)

FString FTATHeightenedSensesEntry::ToString() const
{
   switch (EventType)
   {
   case ETATHeightenedSenseEventType::OnTagAdded:
   case ETATHeightenedSenseEventType::OnGameplayEvent:
      return FString::Printf(TEXT("(FTATHeightenedSensesEntry: Tags = [%s], BlockedTags = [%s], Type = [%s])"), *TagContainer.ToString(), *BlockedTags.ToString(), *UEnum::GetValueAsString(EventType));
   case ETATHeightenedSenseEventType::OnAttributeChanged:
      return FString::Printf(TEXT("(FTATHeightenedSensesEntry: Attribute = [%s], Type = OnAttributeChanged)"), *Attribute.AttributeName);
   
   default:
      checkNoEntry();
      return TEXT("");
   }
}

#if WITH_EDITOR
bool FTATHeightenedSensesEntry::IsDataValid(FDataValidationContext& context) const
{
   if (!AkState)
   {
      context.AddError(FText::FromString(TEXT("Found FTATHeightenedSensesEntry with unassigned AkState!")));
   }

   // Make sure tag / attribute is assigned according to event type
   switch (EventType)
   {
   case ETATHeightenedSenseEventType::OnAttributeChanged:
      if (!Attribute.IsValid())
      {
         context.AddError(FText::FromString(TEXT("FTATHeightenedSensesEntry with EventType OnAttributeChanged has unassigned Attribute!")));
      }
      break;

   
   case ETATHeightenedSenseEventType::OnTagAdded: // TagAdded ONLY: disallow use of DurationSeconds (sense should only be cancelled by tag-removal)
      if (DurationSeconds != 0 && EventType == ETATHeightenedSenseEventType::OnTagAdded)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("FTATHeightenedSensesEntry with EventType = OnTagAdded has a non-zero duration (%f)! Set this to 0"), DurationSeconds)));
      }
   case ETATHeightenedSenseEventType::OnGameplayEvent: // TagAdded + OnGameplayEvent
      if (TagContainer.IsEmpty())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("FTATHeightenedSensesEntries with type %s has empty TagContainer!"), *UEnum::GetValueAsString(EventType))));
      }
      for (FGameplayTag tag : TagContainer)
      {
         if (!tag.IsValid())
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("FTATHeightenedSensesEntry with EventType %s has invalid Tag in TagContainer!"), *UEnum::GetValueAsString(EventType))));
         }
      }
      for (FGameplayTag tag : BlockedTags)
      {
         if (!tag.IsValid())
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("FTATHeightenedSensesEntry with EventType %s has invalid Tag in BlockedTags!"), *UEnum::GetValueAsString(EventType))));
         }
      }
         
      break;
   }

   return context.GetNumErrors() + context.GetNumWarnings() <= 0;
}
#endif // WITH_EDITOR

bool FTATHeightenedSenseEventHistory::DurationElapsed(const UObject* contextObj) const
{
   // Default to false for infinite-duration entries
   if (HeightenedSensesEntry.DurationSeconds <= 0)
   {
      return false;
   }

   const float durationDelay = HeightenedSensesEntry.DurationSeconds + HeightenedSensesEntry.ApplyAfterSeconds;
   const float timeElapsed = contextObj->GetWorld()->GetTimeSeconds() - EntryAddedTimestamp;
   return timeElapsed > durationDelay;
}

bool FTATHeightenedSenseEventHistory::InitialDelayElapsed(const UObject* contextObj) const
{
   if (HeightenedSensesEntry.ApplyAfterSeconds <= 0)
   {
      return true;
   }

   const float timeElapsed = contextObj->GetWorld()->GetTimeSeconds() - EntryAddedTimestamp;
   return timeElapsed > HeightenedSensesEntry.ApplyAfterSeconds;
}

FString FTATHeightenedSenseEventHistory::ToString() const
{
   return FString::Printf(TEXT("(FTATHeightenedSenseEventHistory: %s | EntryAddedTimestamp = [%f])"), *HeightenedSensesEntry.ToString(), EntryAddedTimestamp);
}
