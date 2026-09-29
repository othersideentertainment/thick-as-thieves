// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATSceneVariantUtils.generated.h"

class UTATSceneAsset;
enum class ETATPrivateSpaceType : uint8;

UENUM()
enum class ETATSceneResolveResult : uint8
{
   Resolved,
   NoRequirement
};

UCLASS()
class TAT_API UTATSceneVariantUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()
   
public:
   UFUNCTION(BlueprintPure, Category="TAT|SceneVariant", meta=(ShortName=IsSet))
   static bool IsSceneRequirementSet(const FTATSceneRequirement& requirement);

   UFUNCTION(BlueprintPure, Category = "TAT|SceneVariant", meta = (ShortName = IsNone))
   static bool IsSceneRequirementNone(const FTATSceneRequirement& requirement);

   // Likely more useful for c++ callers
   static bool ResolveBoolRequirement(const UWorld* world, const FTATSceneRequirement& requirement);

   // Resolves a SceneRequirement to a boolean if it is set
   // Possible results:
   //  - Resolved with value: The requirement is set, so use the value
   //  - NoRequirement: No actual requirement, so just ignore
   //
   // Safe to call in BeginPlay or later
   UFUNCTION(BlueprintCallable, Category = "TAT|SceneVariant", meta = (DisplayName = "Resolve Scene Requirement", ExpandEnumAsExecs = "ReturnValue", WorldContext = worldContext, UnsafeDuringActorConstruction))
   static ETATSceneResolveResult TryResolveBoolRequirement(const UObject* worldContext, const FTATSceneRequirement& requirement, bool& outValue);

   static ETATPrivateSpaceType ResolvePrivacyForScene(const UWorld* world, const UTATSceneAsset* scene, ETATPrivateSpaceType fallback);
};
