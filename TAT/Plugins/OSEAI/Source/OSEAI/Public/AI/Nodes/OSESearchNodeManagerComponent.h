// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"


#include "OSESearchNodeManagerComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSearchNodeManager, Log, All);

class AOSESearchNode;

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OSEAI_API UOSESearchNodeManagerComponent : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   void AddSearchNode(TWeakObjectPtr<AOSESearchNode> searchNode);

   const TArray<TWeakObjectPtr<AOSESearchNode>>& GetSearchNodes() const
   {
      return _searchNodes;
   }

private:
   TArray<TWeakObjectPtr<AOSESearchNode>> _searchNodes;
};
