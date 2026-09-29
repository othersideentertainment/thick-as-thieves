// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TATVersion.generated.h"

/**
 * Static accessors for the build version information
 */
UCLASS(NotBlueprintable, NotPlaceable)
class TAT_API UTATVersionV1 : public UObject
{
   GENERATED_BODY()

public:
   /// Version formatted for parsing. All spaces and forward slashes are replaced by + signs
   static FString ToString(EVersionComponent lastComponent = EVersionComponent::Branch);

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development")
   /// Build version formatted for display on the UI. May contain spaces and other problematic characters for parsing
   static FString GetBuildVersionString();

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development")
   static FString GetBuildDate();

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development")
   static int32 GetBuildVersionMajor();

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development")
   static int32 GetBuildVersionMinor();

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development")
   static int32 GetBuildNumber();

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development")
   static int32 GetBuildChangelistNumber();

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development")
   static FString GetBuildBranch();

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development")
   static FString GetBuildBranchDescriptor();
   
   static const TArray<FString>& GetBuildArtifactDescriptors();
   static const FString& GetBuildArtifactDescriptor();
   static int GetEdition();
};
