// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSEGenericGraphNodeHandle.h"
#include "Templates/SubclassOf.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OSEGenericGraphSoftNodeHandle.generated.h"

class UOSEGenericGraph;
class UOSEGenericGraphNode;

/// Similar to a data table row handle, this makes it easy to reference graph nodes as a UPROPERTY.
/// Has a custom editor widget that lists all nodes in the graph for easy selection.
/// Identical to FOSEGenericGraphNodeHandle, except that it uses a TSoftObjectPtr for the graph reference.
USTRUCT(BlueprintType)
struct OSEGENERICGRAPHRUNTIME_API FOSEGenericGraphSoftNodeHandle
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Generic Graph Soft Node Handle")
   TSoftObjectPtr<UOSEGenericGraph> Graph;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Generic Graph Soft Node Handle")
   FGuid NodeId;

   FOSEGenericGraphSoftNodeHandle() = default;
   explicit FOSEGenericGraphSoftNodeHandle(const UOSEGenericGraphNode* InNode);
   explicit FOSEGenericGraphSoftNodeHandle(const FOSEGenericGraphNodeHandle& InHandle);
   FOSEGenericGraphSoftNodeHandle(const TSoftObjectPtr<UOSEGenericGraph>& InGraph, const FGuid& InNodeId);

   FOSEGenericGraphNodeHandle LoadSynchronous() const;

   bool RequestAsyncLoad(const TFunction<void(FOSEGenericGraphNodeHandle)>& OnLoadedCallback) const;

   FString ToDebugString() const;

   FORCEINLINE void Reset()
   {
      Graph.Reset();
      NodeId = FGuid();
   }

   /// Returns true if this handle is specifically pointing to nothing
   bool IsNull() const
   {
      return Graph.IsNull() && !NodeId.IsValid();
   }

   bool IsValid() const
   {
      return !IsNull();
   }

   explicit operator bool() const { return IsValid(); }

   bool operator==(const FOSEGenericGraphSoftNodeHandle& Other) const;
   bool operator!=(const FOSEGenericGraphSoftNodeHandle& Other) const { return !operator==(Other); }

   friend uint32 GetTypeHash(const FOSEGenericGraphSoftNodeHandle& self)
   {
      return HashCombineFast(GetTypeHash(self.Graph), GetTypeHash(self.NodeId));
   }
};


UCLASS()
class OSEGENERICGRAPHRUNTIME_API UOSEGenericGraphSoftNodeHandleFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "Generic Graph", Meta = (BlueprintAutocast, CompactNodeTitle = "To Soft Node Handle"))
   static FOSEGenericGraphSoftNodeHandle MakeGenericGraphSoftNodeHandleFromNode(UOSEGenericGraphNode* Node);

   UFUNCTION(BlueprintPure, Category = "Generic Graph", Meta = (BlueprintAutocast, CompactNodeTitle = "To Soft Node Handle"))
   static FOSEGenericGraphSoftNodeHandle MakeGenericGraphSoftNodeHandleFromHandle(const FOSEGenericGraphNodeHandle& Handle);

   UFUNCTION(BlueprintPure, Category = "Generic Graph", Meta = (CompactNodeTitle = "Is Valid"))
   static bool IsGenericGraphSoftNodeHandleValid(const FOSEGenericGraphSoftNodeHandle& Handle) { return Handle.IsValid(); }

   UFUNCTION(BlueprintPure, Category = "Generic Graph", DisplayName = "Load Synchronous (Generic Graph Soft Node Handle)")
   static FOSEGenericGraphNodeHandle GenericGraphSoftNodeHandleLoadSynchronous(const FOSEGenericGraphSoftNodeHandle& Handle) { return Handle.LoadSynchronous(); }
};
