// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OSEVersionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class OSEVERSION_API UOSEVersionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development|OSE")
	static FString GetBuildVersionString();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development")
	static FString GetBuildDate();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development|OSE")
	static int32 GetBuildVersionMajor();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development|OSE")
	static int32 GetBuildVersionMinor();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development|OSE")
	static int32 GetBuildNumber();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development|OSE")
	static int32 GetBuildChangelistNumber();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development|OSE")
	static FString GetBuildBranch();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Development|OSE")
	static FString GetBuildConfiguration();

};
