// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue4
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

// ose
#include "Conditions/OSECondition.h"

#include "OSEVoiceOverConversationNode.generated.h"

class UOSEVoiceOverLine;
class UOSEVoiceOverConversation;

UENUM(BlueprintType)
enum class EOSEVoiceOverConversationNodeDurationType : uint8
{
   AudioDuration,
   FixedDuration,
   Instant,
};

UCLASS()
class OSECORE_API UOSEVoiceOverConversationNode : public UObject
{
   GENERATED_BODY()
public:

   UPROPERTY()
   TArray<UOSEVoiceOverConversationNode*> NextNodes;

   UPROPERTY()
   UOSEVoiceOverConversation* Conversation = nullptr;

   UPROPERTY(EditAnywhere)
   UOSEVoiceOverLine* Line = nullptr;

   UPROPERTY(EditAnywhere)
   EOSEVoiceOverConversationNodeDurationType DurationType = EOSEVoiceOverConversationNodeDurationType::AudioDuration;

   UPROPERTY(EditAnywhere, meta = (UIMin = "0.0", EditCondition = "DurationType == EOSEVoiceOverConversationNodeDurationType::FixedDuration", EditConditionHides))
   float FixedDuration = 1.0f;

   UPROPERTY(EditAnywhere)
   int32 SpeakerIndex = 0;

   UPROPERTY(EditAnywhere)
   FOSEConditionSet Conditions;

   UOSEVoiceOverConversationNode* FindNextNode(const TArray<AActor*>& speakers, FRandomStream& randomStream);

   static UOSEVoiceOverConversationNode* SelectRandomValidNode(const TArray<UOSEVoiceOverConversationNode*>& nodes, const TArray<AActor*>& speakers, FRandomStream& randomStream);
};
