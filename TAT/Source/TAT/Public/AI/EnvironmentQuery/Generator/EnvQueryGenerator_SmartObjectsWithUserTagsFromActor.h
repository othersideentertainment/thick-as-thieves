// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "EnvQueryGenerator_SmartObjects.h"

#include "EnvQueryGenerator_SmartObjectsWithUserTagsFromActor.generated.h"


UCLASS(meta = (DisplayName = "[TAT] Smart Object with User Tags from owned Pawn"))
class TAT_API UEnvQueryGenerator_SmartObjectsWithUserTagsFromActor : public UEnvQueryGenerator_SmartObjects
{
   GENERATED_BODY()

public:
   virtual FText GetDescriptionTitle() const override;
protected:
   virtual void InjectIntoSmartObjectRequest(FEnvQueryInstance& queryInstance, FSmartObjectRequest& request) const override;
};
