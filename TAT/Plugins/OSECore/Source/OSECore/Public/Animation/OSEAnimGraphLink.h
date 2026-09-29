// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

// OSE
#include "OSEAnimGraphLink.generated.h"


//--------------------------------------------------------------------------------------------------
/// Animation configuration for a specific anim graph linkage.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, Category = Animation, meta = (DisplayName = "Anim Graph Link Struct [OSE]"))
struct OSECORE_API FOSEAnimGraphLink
{
   GENERATED_BODY()

public:

   /// The tag to search for when replacing anim graph nodes.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   FName Tag;

   /// The class to replace the node with. An empty class with a valid tag will effectively no-op the anim graph.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   TSubclassOf< class UAnimInstance > InstanceClass;
};


//--------------------------------------------------------------------------------------------------
/// Data asset to specify which anim graphs to link.
//--------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Category = Animation, meta = (DisplayName = "Anim Graph Links Asset [OSE]"))
class OSECORE_API UOSEAnimGraphLinkAsset : public UDataAsset
{
   GENERATED_BODY()

public:

   /// The animation graphs to link. An empty class with a valid tag will effectively no-op the anim graph.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   TArray< FOSEAnimGraphLink > Links;
};
