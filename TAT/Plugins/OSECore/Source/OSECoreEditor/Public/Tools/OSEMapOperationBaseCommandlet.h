// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Tools/OSECommandletBase.h"

#include "OSEMapOperationBaseCommandlet.generated.h"

/// A commandlet for performing an operation across a collection of maps and (if desired) the maps they reference via level instance
UCLASS(Config=Editor)
class OSECOREEDITOR_API UOSEMapOperationBaseCommandlet : public UOSECommandletBase
{
   GENERATED_BODY()

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;


   /// Derived commandlets should implement this to perform whatever fixup/modification is needed. 
   /// Non-level instance packages modified should go in outPackagesToSave, and level instances requiring the same fixup should have their package-paths added to levelInstancePackagePathsWarrantingOperation
   virtual void PerformOperation(bool includeActorsInLevelInstances, TArray<UPackage*>& outPackagesToSave, TArray<FString>& levelInstancePackagePathsWarrantingOperation) { unimplemented(); }

private:
   bool _LoadMap(const FString& mapPath);
   void _PerformOperationAndSaveMap(const FString& mapPath, TArray<FString>& levelInstancePathsToPerformOperationOn, bool operateOnLevelInstances);

protected:
   UPROPERTY(Config)
   TArray<FFilePath> _mapsToRunOn;
};
