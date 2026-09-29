// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

//ose
#include "VoiceOver/OSEVoiceOverConversationNode.h"

// ose editor
#include "OSEVoiceOverConversationGraphNode.h"

// ue4
#include "EdGraph/EdGraph.h"


#include "OSEVoiceOverConversationGraph.generated.h"

class UOSEVoiceOverConversationNode;

UCLASS()
class UOSEVoiceOverConversationGraph : public UEdGraph
{
   GENERATED_BODY()
public:

   void RecomputeRootNodes();
};
