// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#include "AI/Nodes/OSESearchNodeManagerComponent.h"

// ose
#include "AI/Nodes/OSESearchNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESearchNodeManagerComponent)

DEFINE_LOG_CATEGORY(LogSearchNodeManager);

void UOSESearchNodeManagerComponent::AddSearchNode(TWeakObjectPtr<AOSESearchNode> searchNode)
{
   _searchNodes.Add(searchNode);
   UE_LOG(LogSearchNodeManager, Verbose, TEXT("Added search node: %s"), *searchNode->GetName());
}

