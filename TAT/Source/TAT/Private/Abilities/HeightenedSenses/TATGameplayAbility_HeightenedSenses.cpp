// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/HeightenedSenses/TATGameplayAbility_HeightenedSenses.h"

// wwise
#include "AkGameplayStatics.h"
#include "AkStateValue.h"

// ue
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_HeightenedSenses)
DEFINE_LOG_CATEGORY_STATIC(LogTATHeightenedSenses, Log, All);

UTATGameplayAbility_HeightenedSenses::UTATGameplayAbility_HeightenedSenses()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

   ActivationRequiresLocalControl = true;
   ActivationRequiresPlayerController = true;
   ActivateAbilityWhenGranted = true;
}

#if WITH_EDITOR
EDataValidationResult UTATGameplayAbility_HeightenedSenses::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   // Validate sense settings
   if (!_defaultAkState)
   {
      context.AddError(FText::FromString(TEXT("Unassigned DefaultAkState!")));
   }

   TSet<FGameplayTag> usedTags;
   TSet<FGameplayAttribute> usedAttributes;
   for (const FTATHeightenedSensesEntry& senseEntry : _heightenedSenseEntries)
   {
      senseEntry.IsDataValid(context);

      // Check for any overlapping usage of tag / attribute
      switch (senseEntry.EventType)
      {
      case ETATHeightenedSenseEventType::OnAttributeChanged:
         if (senseEntry.Attribute.IsValid())
         {
            // Check for multiple usage of same attribute
            bool alreadyPresent = false;
            usedAttributes.Add(senseEntry.Attribute, &alreadyPresent);
            if (alreadyPresent)
            {
               context.AddError(FText::FromString(FString::Printf(TEXT("Multiple FTATHeightenedSensesEntries using Attribute %s!"), *senseEntry.Attribute.AttributeName)));
            }
         }
         break;

      case ETATHeightenedSenseEventType::OnGameplayEvent:
         for (FGameplayTag tag : senseEntry.TagContainer)
         {
            if (tag.IsValid())
            {
               // Check for multiple usage of same tag
               bool alreadyPresent = false;
               usedTags.Add(tag, &alreadyPresent);
               if (alreadyPresent)
               {
                  context.AddError(FText::FromString(FString::Printf(TEXT("Multiple FTATHeightenedSensesEntries with EventType OnGameplayEvent using Tag %s!"), *tag.ToString())));
               }
            }
         }
         
         break;
      }
   }

   return context.GetNumErrors() + context.GetNumWarnings() <= 0 ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
void UTATGameplayAbility_HeightenedSenses::PostEditChangeChainProperty(FPropertyChangedChainEvent& propertyChangedEvent)
{
   Super::PostEditChangeChainProperty(propertyChangedEvent);
   if (propertyChangedEvent.Property)
   {
      // Listen for any changes to sense entries
      const FName propName = propertyChangedEvent.Property->GetFName();
      if (propName == GET_MEMBER_NAME_CHECKED(UTATGameplayAbility_HeightenedSenses, _heightenedSenseEntries)
         || propName == GET_MEMBER_NAME_CHECKED(FTATHeightenedSensesEntry, EventType)
         || propName == GET_MEMBER_NAME_CHECKED(FTATHeightenedSensesEntry, DurationSeconds))
      {
         // Zero out the DurationSeconds for OnTagAdded sense entries, as these senses should only be cancelled by tag-removal
         for (FTATHeightenedSensesEntry& senseEntry : _heightenedSenseEntries)
         {
            if (senseEntry.EventType == ETATHeightenedSenseEventType::OnTagAdded)
            {
               senseEntry.DurationSeconds = 0.f;
            }
         }
      }
   }
}
#endif // WITH_EDITOR

void UTATGameplayAbility_HeightenedSenses::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);

   CommitAbility(handle, actorInfo, activationInfo);

   // Bind to ASC hooks for each sense entry
   UAbilitySystemComponent* asc = actorInfo->AbilitySystemComponent.Get();
   check(IsValid(asc));
   _BindHeightenedSenseEvents(asc);

   // Refresh in case a sense-activated tag was added in a starting effect set
   _RefreshHeightenedSenseAkState();
}

void UTATGameplayAbility_HeightenedSenses::EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled)
{
   if (UAbilitySystemComponent* asc = actorInfo->AbilitySystemComponent.Get())
   {
      _UnbindHeightenedSenseEvents(asc);
   }

   // Clear all history entries
   for(FTATHeightenedSenseEventHistory& eventHistory : _heightenedSenseEventHistoryEntries)
   {
      _ClearHeightenedSenseTimers(eventHistory);
   }
   _heightenedSenseEventHistoryEntries.Reset();

   if (_defaultAkState)
   {
      UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("EndAbility() | restoring default state..."));
      _SetAkState(_defaultAkState);
   }

   Super::EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);
}

void UTATGameplayAbility_HeightenedSenses::_RefreshHeightenedSenseAkState()
{
   for (const FTATHeightenedSensesEntry& heightenedSenseEntry : _heightenedSenseEntries)
   {
      // Check for tag-presence if necessary
      if (!_MeetsTagRequirements(heightenedSenseEntry))
      {
         continue;
      }

      // Skip entries according to ApplyAfterSeconds / DurationSeconds
      if (!_MeetsTimeConstraints(heightenedSenseEntry))
      {
         UE_LOG(LogTATHeightenedSenses, VeryVerbose, TEXT("_RefreshHeightenedSenseAkState() | FTATHeightenedSensesEntry (%s) doesn't meet time constraints - skipping..."), *heightenedSenseEntry.ToString());
         continue;
      }

      check(heightenedSenseEntry.AkState);
      UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("Applying Ak State %s for heightened sense: %s"), *heightenedSenseEntry.AkState->GetName(), *heightenedSenseEntry.ToString());
      _SetAkState(heightenedSenseEntry.AkState);
      return;
   }

   if(_defaultAkState)
   {
      UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("Applying DefaultAkState: %s"), *_defaultAkState->GetName());
      _SetAkState(_defaultAkState);
   }
   else
   {
      UE_LOG(LogTATHeightenedSenses, Error, TEXT("DefaultAkState unassigned!"));
   }
}

void UTATGameplayAbility_HeightenedSenses::_BindHeightenedSenseEvents(UAbilitySystemComponent* asc)
{
   check(IsValid(asc));
   for (auto it = _heightenedSenseEntries.CreateIterator(); it; it++)
   {
      FTATHeightenedSensesEntry& heightenedSenseEntry = *it;

      // Remove instances that don't pass validation (to prevent failed checks in PIE)
#if WITH_EDITOR
      FDataValidationContext context;
      if (!heightenedSenseEntry.IsDataValid(context))
      {
         UE_LOG(LogTATHeightenedSenses, Error, TEXT("Skipping invalid entry: %s"), *heightenedSenseEntry.ToString());
         it.RemoveCurrent();
         continue;
      }
#endif // WITH_EDITOR
      
      // All events listen for blocked tags that suppress senses
      for (const FGameplayTag tag : heightenedSenseEntry.BlockedTags)
      {
         asc->RegisterGameplayTagEvent(tag).AddUObject(this, &UTATGameplayAbility_HeightenedSenses::_OnHeightenedSenseTagChanged);
      }

      switch (heightenedSenseEntry.EventType)
      {
      case ETATHeightenedSenseEventType::OnTagAdded:
         for (const FGameplayTag tag : heightenedSenseEntry.TagContainer)
         {
            asc->RegisterGameplayTagEvent(tag).AddUObject(this, &UTATGameplayAbility_HeightenedSenses::_OnHeightenedSenseTagChanged);
         }
         break;
      case ETATHeightenedSenseEventType::OnGameplayEvent:
         heightenedSenseEntry.GameplayEventDelegateHandle = asc->AddGameplayEventTagContainerDelegate(
            heightenedSenseEntry.TagContainer
            , FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UTATGameplayAbility_HeightenedSenses::_OnHeightenedSenseGameplayEvent));
         break;
      case ETATHeightenedSenseEventType::OnAttributeChanged:
         asc->GetGameplayAttributeValueChangeDelegate(heightenedSenseEntry.Attribute).AddUObject(this, &UTATGameplayAbility_HeightenedSenses::_OnHeightenedSenseAttributeChanged);
         break;

      default:
         checkNoEntry();
      }
   }
}

void UTATGameplayAbility_HeightenedSenses::_UnbindHeightenedSenseEvents(UAbilitySystemComponent* asc)
{
   if (!IsValid(asc))
   {
      return;
   }

   for (const FTATHeightenedSensesEntry& heightenedSenseEntry : _heightenedSenseEntries)
   {
      for (const FGameplayTag tag : heightenedSenseEntry.BlockedTags)
      {
         asc->RegisterGameplayTagEvent(tag).RemoveAll(this);
      }

      switch (heightenedSenseEntry.EventType)
      {
      case ETATHeightenedSenseEventType::OnTagAdded:
         for (const FGameplayTag tag : heightenedSenseEntry.TagContainer)
         {
            asc->RegisterGameplayTagEvent(tag).RemoveAll(this);
         }
         break;
      case ETATHeightenedSenseEventType::OnGameplayEvent:
         for (const FGameplayTag tag : heightenedSenseEntry.TagContainer)
         {
            asc->RemoveGameplayEventTagContainerDelegate(heightenedSenseEntry.TagContainer, heightenedSenseEntry.GameplayEventDelegateHandle);
         }
         break;
      case ETATHeightenedSenseEventType::OnAttributeChanged:
         if (heightenedSenseEntry.Attribute.IsValid())
         {
            asc->GetGameplayAttributeValueChangeDelegate(heightenedSenseEntry.Attribute).RemoveAll(this);
         }
         break;
         
      default:
         checkNoEntry();
      }
   }
}

void UTATGameplayAbility_HeightenedSenses::_OnHeightenedSenseTagChanged(const FGameplayTag tag, int32 newCount)
{
   check(tag.IsValid());
   const bool tagAdded = newCount > 0;
   UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("_OnHeightenedSenseTagChanged() | Tag %s %s")
      , *tag.ToString()
      , tagAdded ? TEXT("added") : TEXT("removed"));

   const FTATHeightenedSensesEntry* heightenedSensesEntry = _GetHeightenedSensesEntry(tag);
   if (!ensure(heightenedSensesEntry))
   {
      return;
   }
   _HandleChangeForHeightenedSense(*heightenedSensesEntry);
}

void UTATGameplayAbility_HeightenedSenses::_OnHeightenedSenseAttributeChanged(const FOnAttributeChangeData& data)
{
   check(data.Attribute.IsValid());
   UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("_OnHeightenedSenseAttributeChanged() | Attribute %s changed"), *data.Attribute.AttributeName);

   const FTATHeightenedSensesEntry* heightenedSensesEntry = _GetHeightenedSensesEntry(data.Attribute);
   if (!ensure(heightenedSensesEntry))
   {
      return;
   }
   _HandleChangeForHeightenedSense(*heightenedSensesEntry);
}

void UTATGameplayAbility_HeightenedSenses::_OnHeightenedSenseGameplayEvent(FGameplayTag tag, const FGameplayEventData* payload)
{
   check(tag.IsValid());
   UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("_OnHeightenedSenseGameplayEvent() | Gameplay Event %s fired"), *tag.ToString());
   
   const FTATHeightenedSensesEntry* heightenedSensesEntry = _GetHeightenedSensesEntry(tag);
   if (!ensure(heightenedSensesEntry))
   {
      return;
   }
   _HandleChangeForHeightenedSense(*heightenedSensesEntry);
}

void UTATGameplayAbility_HeightenedSenses::_HandleChangeForHeightenedSense(const FTATHeightenedSensesEntry& heightenedSensesEntry)
{
   int32 eventIndex = INDEX_NONE;
   FTATHeightenedSenseEventHistory& eventHistory = _FindOrCreateEventHistory(heightenedSensesEntry, eventIndex);

   const bool waitForDelay = heightenedSensesEntry.ApplyAfterSeconds > 0;
   const bool meetsTagRequirements = _MeetsTagRequirements(heightenedSensesEntry);
   if (meetsTagRequirements)
   {
      _UpdateHeightenedSenseTimers(heightenedSensesEntry, eventHistory);
   }
   else
   {
      _RemoveEventHistoryAt(eventIndex);
   }

   // If there's no delay to wait for (or we no longer meet tag requirements), refresh now
   if (!meetsTagRequirements || !waitForDelay)
   {
      _RefreshHeightenedSenseAkState();
   }
}

void UTATGameplayAbility_HeightenedSenses::_OnInitialDelayElapsed()
{
   _RefreshHeightenedSenseAkState();
}

void UTATGameplayAbility_HeightenedSenses::_OnDurationElapsed()
{
   // Remove history entry with elapsed duration
   for (auto it = _heightenedSenseEventHistoryEntries.CreateIterator(); it; it++)
   {
      FTATHeightenedSenseEventHistory& eventHistory = *it;
      if (eventHistory.DurationElapsed(this))
      {
         // Clear timers (just in case) and remove from collection
         _RemoveEventHistoryAt(it.GetIndex());
         break;
      }
   }

   _RefreshHeightenedSenseAkState();
}

void UTATGameplayAbility_HeightenedSenses::_UpdateHeightenedSenseTimers(const FTATHeightenedSensesEntry& senseEntry, FTATHeightenedSenseEventHistory& eventHistory)
{
   const bool looping = false;
   if (senseEntry.ApplyAfterSeconds > 0)
   {
      UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("_UpdateHeightenedSenseTimers() | Setting %f second delay for FTATHeightenedSensesEntry (%s)...")
         , senseEntry.ApplyAfterSeconds
         , *senseEntry.ToString());

      GetWorld()->GetTimerManager().SetTimer(eventHistory.ApplyAfterSecondsHandle, this, &UTATGameplayAbility_HeightenedSenses::_OnInitialDelayElapsed, senseEntry.ApplyAfterSeconds, looping);
   }
   if (senseEntry.DurationSeconds > 0 && senseEntry.EventType != ETATHeightenedSenseEventType::OnTagAdded)
   {
      UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("_UpdateHeightenedSenseTimers() | Setting %f second duration for FTATHeightenedSensesEntry (%s)...")
         , senseEntry.DurationSeconds
         , *senseEntry.ToString());

      // Make sure to incorporate preceding ApplyAfterSeconds into delay
      const float durationDelay = senseEntry.ApplyAfterSeconds + senseEntry.DurationSeconds;
      GetWorld()->GetTimerManager().SetTimer(eventHistory.DurationHandle, this, &UTATGameplayAbility_HeightenedSenses::_OnDurationElapsed, durationDelay, looping);
   }
}

void UTATGameplayAbility_HeightenedSenses::_ClearHeightenedSenseTimers(FTATHeightenedSenseEventHistory& eventHistory)
{
   UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("_ClearHeightenedSenseTimers() | Clearing timers for FTATHeightenedSensesEntry (%s)..."), *eventHistory.ToString());
   GetWorld()->GetTimerManager().ClearTimer(eventHistory.ApplyAfterSecondsHandle);
   GetWorld()->GetTimerManager().ClearTimer(eventHistory.DurationHandle);
}

FTATHeightenedSenseEventHistory* UTATGameplayAbility_HeightenedSenses::_FindEventHistory(const FTATHeightenedSensesEntry& senseEntry, int32& outIndex)
{
   outIndex = INDEX_NONE;
   for (auto it = _heightenedSenseEventHistoryEntries.CreateIterator(); it; it++)
   {
      FTATHeightenedSenseEventHistory& eventHistory = *it;
      if (eventHistory.HeightenedSensesEntry == senseEntry)
      {
         outIndex = it.GetIndex();
         return &eventHistory;
      }
   }

   return nullptr;
}

FTATHeightenedSenseEventHistory& UTATGameplayAbility_HeightenedSenses::_FindOrCreateEventHistory(const FTATHeightenedSensesEntry& senseEntry, int32& outIndex)
{
   outIndex = INDEX_NONE;
   if (FTATHeightenedSenseEventHistory* eventHistory = _FindEventHistory(senseEntry, outIndex))
   {
      return *eventHistory;
   }
   
   UE_LOG(LogTATHeightenedSenses, Verbose, TEXT("Creating a FTATHeightenedSenseEventHistory for FTATHeightenedSensesEntry %s (ApplyAfterSeconds = %f, DurationSeconds = %f")
      , *senseEntry.ToString()
      , senseEntry.ApplyAfterSeconds
      , senseEntry.DurationSeconds);

   outIndex = _heightenedSenseEventHistoryEntries.Emplace(senseEntry);
   check(_heightenedSenseEventHistoryEntries.IsValidIndex(outIndex));
   FTATHeightenedSenseEventHistory& eventHistory = _heightenedSenseEventHistoryEntries[outIndex];

   const float worldTime = GetWorld()->GetTimeSeconds();
   eventHistory.EntryAddedTimestamp = worldTime;

   return eventHistory;
}

void UTATGameplayAbility_HeightenedSenses::_RemoveEventHistoryAt(int32 index)
{
   checkf(_heightenedSenseEventHistoryEntries.IsValidIndex(index), TEXT("Invalid index %d!"), index);

   // Clear timers and remove entry
   FTATHeightenedSenseEventHistory& eventHistory = _heightenedSenseEventHistoryEntries[index];
   _ClearHeightenedSenseTimers(eventHistory);
   _heightenedSenseEventHistoryEntries.RemoveAt(index);
}

void UTATGameplayAbility_HeightenedSenses::_SetAkState(const UAkStateValue* akState)
{
   if (!akState)
   {
      UE_LOG(LogTATHeightenedSenses, Error, TEXT("_SetAkState() called with invalid UAkStateValue!"));
      return;
   }

   UAkGameplayStatics::SetState(akState);
}

bool UTATGameplayAbility_HeightenedSenses::_MeetsTimeConstraints(const FTATHeightenedSensesEntry& heightenedSenseEntry)
{
   // Skip for entries with no time constraints
   if (heightenedSenseEntry.ApplyAfterSeconds <= 0 && heightenedSenseEntry.DurationSeconds <= 0)
   {
      return true;
   }

   const FTATHeightenedSenseEventHistory* heightenedSenseEventHistory = _FindEventHistory(heightenedSenseEntry);
   if (!heightenedSenseEventHistory)
   {
      return false;
   }

   // Return false if ApplyAfterSeconds delay has not elapsed
   if (!heightenedSenseEventHistory->InitialDelayElapsed(this))
   {
      return false;
   }
   // Return false if DurationSeconds (and preceding delay) has elapsed
   if (heightenedSenseEventHistory->DurationElapsed(this))
   {
      return false;
   }

   return true;
}

bool UTATGameplayAbility_HeightenedSenses::_MeetsTagRequirements(const FTATHeightenedSensesEntry& heightenedSenseEntry) const
{
   const UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo();
   check(asc);
   if (asc->HasAnyMatchingGameplayTags(heightenedSenseEntry.BlockedTags))
   {
      return false;
   }

   // Only check presence of tags in TagContainer for OnTagAdded (as they represent GameplayEvents for senses with ETATHeightenedSenseEventType::OnGameplayEvent, and aren't used for OnAttributeChanged)
   if (heightenedSenseEntry.EventType == ETATHeightenedSenseEventType::OnTagAdded)
   {
      return heightenedSenseEntry.AllTagsRequired ? asc->HasAllMatchingGameplayTags(heightenedSenseEntry.TagContainer) : asc->HasAnyMatchingGameplayTags(heightenedSenseEntry.TagContainer);
   }
   return true;
}
