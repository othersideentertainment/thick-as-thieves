// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OSEGenericGraphNodeHandle.generated.h"

class UOSEGenericGraph;
class UOSEGenericGraphNode;

/// Similar to a data table row handle, this makes it easy to reference graph nodes as a UPROPERTY.
/// Has a custom editor widget that lists all nodes in the graph for easy selection.
USTRUCT(BlueprintType)
struct OSEGENERICGRAPHRUNTIME_API FOSEGenericGraphNodeHandle
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Generic Graph Node Handle")
   TObjectPtr<UOSEGenericGraph> Graph;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Generic Graph Node Handle")
   FGuid NodeId;

   FOSEGenericGraphNodeHandle() = default;
   explicit FOSEGenericGraphNodeHandle(const UOSEGenericGraphNode* InNode);
   FOSEGenericGraphNodeHandle(UOSEGenericGraph* InGraph, const FGuid& InNodeId);

   UOSEGenericGraphNode* GetNode() const;

   template<typename T>
   T* GetNode() const
   {
      return Cast<T>(GetNode());
   }

   FString ToDebugString() const;

   FORCEINLINE void Reset()
   {
      Graph = nullptr;
      NodeId = FGuid();
   }

   /// Returns true if this handle is specifically pointing to nothing
   bool IsNull() const
   {
      return Graph == nullptr && !NodeId.IsValid();
   }

   bool IsValid() const
   {
      return Graph != nullptr && NodeId.IsValid();
   }

   explicit operator bool() const { return IsValid(); }

   bool operator==(const FOSEGenericGraphNodeHandle& Other) const;
   bool operator!=(const FOSEGenericGraphNodeHandle& Other) const { return !operator==(Other); }

   friend uint32 GetTypeHash(const FOSEGenericGraphNodeHandle& self)
   {
      return HashCombineFast(GetTypeHash(self.Graph), GetTypeHash(self.NodeId));
   }
};


UCLASS()
class OSEGENERICGRAPHRUNTIME_API UOSEGenericGraphNodeHandleFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "Generic Graph", Meta = (BlueprintAutocast, CompactNodeTitle = "To Handle"))
   static FOSEGenericGraphNodeHandle MakeGenericGraphNodeHandle(UOSEGenericGraphNode* Node);

   UFUNCTION(BlueprintPure, Category = "Generic Graph", Meta = (CompactNodeTitle = "Is Valid"))
   static bool IsGenericGraphNodeHandleValid(const FOSEGenericGraphNodeHandle& Handle) { return Handle.IsValid(); }

   UFUNCTION(BlueprintPure, Category = "Generic Graph", Meta = (CompactNodeTitle = "Get Node"))
   static UOSEGenericGraphNode* GetGenericGraphNodeHandleNode(const FOSEGenericGraphNodeHandle& Handle) { return Handle.GetNode(); }
};
