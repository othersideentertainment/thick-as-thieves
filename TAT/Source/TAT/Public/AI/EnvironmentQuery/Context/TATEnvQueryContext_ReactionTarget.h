// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "EnvironmentQuery/EnvQueryContext.h"

#include "TATEnvQueryContext_ReactionTarget.generated.h"

// EQS context used to retrieve data within UTATAIReactionCoordinatorSubsystem that is associated
// with the query using this context. Returns the actual location of the associated reaction target.
UCLASS()
class TAT_API UTATEnvQueryContext_ReactionTarget : public UEnvQueryContext
{
   GENERATED_BODY()

public:
   // from UEnvQueryContext
   virtual void ProvideContext(FEnvQueryInstance& queryInstance, FEnvQueryContextData& contextData) const;

protected:
   FORCEINLINE virtual bool UseProjectedLocation() const { return false; }
};

// EQS context used to retrieve data within UTATAIReactionCoordinatorSubsystem that is associated
// with the query using this context. Returns the navmesh projected location of the associated reaction target.
UCLASS()
class TAT_API UTATEnvQueryContext_ReactionTarget_Projected : public UTATEnvQueryContext_ReactionTarget
{
   GENERATED_BODY()

protected:
   // from UEnvQueryContext_ReactionTarget
   FORCEINLINE virtual bool UseProjectedLocation() const override { return true; }

};
