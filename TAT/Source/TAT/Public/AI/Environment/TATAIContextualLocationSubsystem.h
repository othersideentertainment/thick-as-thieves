// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATAIContextualLocation.h"

// ue
#include "GameplayTagContainer.h"
#include "Containers/Set.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATAIContextualLocationSubsystem.generated.h"

USTRUCT()
struct TAT_API FTATAIContextualLocationSet
{
   GENERATED_BODY()

public:
   UPROPERTY(Transient)
   TSet<TObjectPtr<ATATAIContextualLocation>> Locations;
};

// Subsystem used for cached retrieval of all of a given type of contextual location
// that AI behaviors may want to refer to (such as civilian flee points).
UCLASS()
class TAT_API UTATAIContextualLocationSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()
	
public:
   void RegisterLocation(ATATAIContextualLocation* location);
   void UnregisterLocation(ATATAIContextualLocation* location);

   const FTATAIContextualLocationSet* GetLocationSetForType(const FGameplayTag& locationType) const;

private:
   FTATAIContextualLocationSet* _FindLocationSet(const FGameplayTag& locationType);
   const FTATAIContextualLocationSet* _FindLocationSet(const FGameplayTag& locationType) const;

   FTATAIContextualLocationSet& _FindOrAddLocationSet(const FGameplayTag& locationType);

   UPROPERTY(Transient)
   TMap<FGameplayTag, FTATAIContextualLocationSet> _locationSets;

};
