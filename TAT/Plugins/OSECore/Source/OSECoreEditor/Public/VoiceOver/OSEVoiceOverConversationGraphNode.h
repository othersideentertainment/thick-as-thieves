// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

//ose
#include "VoiceOver/OSEVoiceOverConversationNode.h"

// ue4
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNode.h"


#include "OSEVoiceOverConversationGraphNode.generated.h"

class UOSEVoiceOverConversation;

UCLASS()
class UOSEVoiceOverConversationGraphNode : public UEdGraphNode
{
   GENERATED_UCLASS_BODY()
public:
   UPROPERTY()
   UOSEVoiceOverConversationNode* ConversationNode;

   UPROPERTY()
   UOSEVoiceOverConversation* Conversation;

   virtual void AllocateDefaultPins() override;
   virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
   virtual FLinearColor GetNodeTitleColor() const;
   virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
   virtual bool CanUserDeleteNode() const override;
};
