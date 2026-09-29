// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Interface.h"

#include "TATAlternateLockInterface.generated.h"

class ILockpickableInterface;
struct FInteractPrompt;
struct FInteractStartResult;
struct FLockInteractContext;

// This class does not need to be modified.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UTATAlternateLockInterface : public UInterface
{
   GENERATED_BODY()
};

// An interface that can be implemented by components to bolt-on non-standard locks to existing lockable actors
// (e.g. combination locks)
//
// It does not own the locked-ness state, but rather the behavior around it
//
// Currently, it intercepts interaction before the LockConfig gets a crack
// at the default behavior. That makes it convenient to inherit default
// behavior, but may imply more knowledge than is desired. So that may be something
// to revisit, but the surface area is small for now.
//
// Alternate: Could set _in_ LockConfig structure, although that wouldn't
//            change the basic behavior.
// TODO: maybe do that?
class TAT_API ITATAlternateLockInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   // Returns true if handled
   virtual bool TryAddToPrompt(FInteractPrompt& prompt, FLockInteractContext lockContext) const = 0;

   // Returns true if handled
   virtual bool TryHandleInteractStart(
      TScriptInterface<ILockpickableInterface> lockable,
      ACharacter* interactingCharacter,
      FLockInteractContext lockContext,
      FInteractStartResult& outResult) = 0;
};
