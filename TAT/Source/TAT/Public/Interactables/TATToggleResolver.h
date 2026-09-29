// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATToggleResolver.generated.h"


class IOSEToggleInterface;

// Base class for dynamically resolving a toggle for something use
//
// TODO: replace AOSESyncedToggle with an interface (or actor that happens to)
USTRUCT()
struct FTATToggleResolver
{
   GENERATED_BODY()

   virtual ~FTATToggleResolver() = default;

   // Assumed to be callable on clients unless specified otherwise
   virtual TScriptInterface<IOSEToggleInterface> ResolveToggle(AActor* context) const { unimplemented();  return nullptr; }

#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const {}
#endif
};
