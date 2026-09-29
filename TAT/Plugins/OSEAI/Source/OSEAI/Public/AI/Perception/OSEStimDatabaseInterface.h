// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

// ose
#include "AI/Perception/OSEStimDatabase.h"

#include "OSEStimDatabaseInterface.generated.h"

class UOSEStimDatabase;

// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(BlueprintType, MinimalAPI, Category = "AI|OSE", meta = (CannotImplementInterfaceInBlueprint))
class UOSEStimDatabaseInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEAI_API IOSEStimDatabaseInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI|OSE|Utility")
   virtual UOSEStimDatabase* AuthorityGetStimDatabase() const = 0;
};

// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(BlueprintType, MinimalAPI, Category = "AI|OSE", meta = (CannotImplementInterfaceInBlueprint))
class UOSEStimDatabaseOwnerInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEAI_API IOSEStimDatabaseOwnerInterface
{
   GENERATED_BODY()

public:
   virtual void AuthorityOnStimAddedToDatabase(FStimInfo& stimInfo) = 0;
   virtual void AuthorityOnStimAboutToBeRemovedFromDatabase(const FStimInfo& stimInfo) = 0;
   virtual void AuthorityOnStimPerceivedByActorsChanged(FStimInfo& stimInfo, AActor* perceivedBy) = 0;
};
