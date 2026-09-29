// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATExternalSpawnerDependencyInterface.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTATExternalSpawnerDependencyInterface : public UInterface
{
   GENERATED_BODY()
};

/// A marker interface for actors that another spawner would depend on, but
/// but are not spawners themselves
/// 
/// It is up the something to inform the spawn system which such actors
/// are "active" for the purposes of dependencies
class TAT_API ITATExternalSpawnerDependencyInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
