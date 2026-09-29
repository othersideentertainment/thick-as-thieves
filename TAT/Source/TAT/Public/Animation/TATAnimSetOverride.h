// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"

#include "TATAnimSetOverride.generated.h"

// A request to use an anim-set
struct FTATAnimSetRequest
{
   FGameplayTag AnimSetTag;
   FObjectKey Source; //< source object that can be used as a key to remove by later
   int Priority = 0; //< Higher is more important, can be negative

   bool operator==(const FTATAnimSetRequest&) const = default;
};

// A helper struct to collect anim set requests
struct FTATAnimSetOverrides
{
   void AddRequest(const FTATAnimSetRequest& request);
   void RemoveRequest(const FTATAnimSetRequest& request);
   void RemoveBySource(FObjectKey source);

   FGameplayTag GetCurrentOverride() const;

private:
   TArray<FTATAnimSetRequest, TInlineAllocator<4>> _requests;
};

/// Interface for actors who can have overrides on their anim-sets
UINTERFACE(BlueprintType, MinimalAPI, Category = "Animation", meta = (CannotImplementInterfaceInBlueprint))
class UTATAnimSetOverrideInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATAnimSetOverrideInterface
{
   GENERATED_BODY()

public:

   // Largely exists so that it does not fight with disguise, which currently
   // assumes that it has control of the ABP at that point. This is true as long as
   // the disguised character can use a single anim set while disguised
   virtual void SuppressAnimSets(bool isSuppressed) = 0;

   virtual void AddAnimSetRequest(const FTATAnimSetRequest& request) = 0;
   virtual void RemoveAnimSetRequest(const FTATAnimSetRequest& request) = 0;
   virtual void RemoveAnimSetBySource(FObjectKey source) = 0;
};
