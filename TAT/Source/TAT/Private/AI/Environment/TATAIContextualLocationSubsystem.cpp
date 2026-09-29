// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Environment/TATAIContextualLocationSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIContextualLocationSubsystem)

void UTATAIContextualLocationSubsystem::RegisterLocation(ATATAIContextualLocation* location)
{
   check(location != nullptr);
   FTATAIContextualLocationSet& locationSet = _FindOrAddLocationSet(location->GetLocationType());
   locationSet.Locations.Add(location);
}

void UTATAIContextualLocationSubsystem::UnregisterLocation(ATATAIContextualLocation* location)
{
   check(location != nullptr);
   if (FTATAIContextualLocationSet* locationSet = _FindLocationSet(location->GetLocationType()))
   {
      locationSet->Locations.Remove(location);
   }
}

const FTATAIContextualLocationSet* UTATAIContextualLocationSubsystem::GetLocationSetForType(const FGameplayTag& locationType) const
{
   return _FindLocationSet(locationType);
}

FTATAIContextualLocationSet* UTATAIContextualLocationSubsystem::_FindLocationSet(const FGameplayTag& locationType)
{
   return _locationSets.Find(locationType);
}

const FTATAIContextualLocationSet* UTATAIContextualLocationSubsystem::_FindLocationSet(const FGameplayTag& locationType) const
{
   return _locationSets.Find(locationType);
}

FTATAIContextualLocationSet& UTATAIContextualLocationSubsystem::_FindOrAddLocationSet(const FGameplayTag& locationType)
{
   FTATAIContextualLocationSet* locationSet = _FindLocationSet(locationType);
   if (locationSet == nullptr)
   {
      locationSet = &_locationSets.Emplace(locationType);
   }
   check(locationSet != nullptr);
   return *locationSet;
}
