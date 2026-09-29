// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATSpawnerOwnerInterface.generated.h"

class UTATSpawnerComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTATSpawnerOwnerInterface : public UInterface
{
   GENERATED_BODY()
};

/// A marker interface for actors that contain a spawner that another spawner would depend on
///
/// Largely because component references are un-ergonomic
/// 
/// The expectation is that implementors of this interface will have a single spawner component that has
/// "PrimarySpawnerForActor"=true. (backed by validation)
/// 
/// NOTE: It is important that only actor's implement this interface or the UI integration will be broken
class TAT_API ITATSpawnerOwnerInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
