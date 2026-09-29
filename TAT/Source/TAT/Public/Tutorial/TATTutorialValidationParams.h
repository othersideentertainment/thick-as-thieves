// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATTutorialValidationParams.generated.h"

struct FGameplayTag;

// Struct for basic validation of tutorial script
//
// BP-exposed, since some are implemented.
//
// NOTE(zkamsler):
//   This didn't end up being easy to compose safely without eager allocations,
//   so I don't recommend this pattern in the future. However, it works okay,
//   so I have left it just for the leaf callers.
USTRUCT(BlueprintType)
struct TAT_API FTATTutorialValidationParams
{
   GENERATED_BODY()

   // Turns out to mostly be a sentinel for whether it should validate soft actors
   // against the world.
   UPROPERTY()
   TObjectPtr<const UWorld> World = nullptr;

   using FReportErrorRef = TFunctionRef<void(const FText&)>;
   TOptional<FReportErrorRef> ErrorReporter;

   void ReportError(const FText& message) const;
   void RequireSoftActor(const TSoftObjectPtr<AActor>& softActor, const FStringView name) const;
   void RequireSoftClass(const TSoftClassPtr<>& softClass, const FStringView name) const;
   void RequireSoftAsset(const TSoftObjectPtr<>& softObject, const FStringView name) const;
   void RequireTag(const FGameplayTag& tag, const FStringView name) const;
   void RequireClass(const UClass*, const FStringView name) const;
};


UCLASS()
class TAT_API UTATTutorialValidationUtils : public UObject
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable, Category="Tutorial|Validation|TAT")
   static void TutorialRequireSoftActor(const FTATTutorialValidationParams& params, const TSoftObjectPtr<AActor>& softActor, FName name);
};
