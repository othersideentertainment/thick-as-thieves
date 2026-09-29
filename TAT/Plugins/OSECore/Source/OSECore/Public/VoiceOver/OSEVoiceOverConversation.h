// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once
// ose
#include "VoiceOver/OSEVoiceOverBase.h"
#include "VoiceOver/OSEVoiceOverConversationNode.h"

// ue4
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"

#include "OSEVoiceOverConversation.generated.h"

class UEdGraph;
struct FRandomStream;


//---------------------------------------------------------------------------------------
/// FOSEVoiceOverConversationParticipantInfo
///
/// Contains info for selecting conversation participants.
//---------------------------------------------------------------------------------------
USTRUCT()
struct FOSEVoiceOverConversationParticipantInfo
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   FGameplayTagQuery AcceptedTags;

   UPROPERTY(EditAnywhere)
   FOSEConditionSet Conditions;
};


//---------------------------------------------------------------------------------------
/// UOSEVoiceOverConversation
///
/// Contains a directed graph of conversation nodes. Each node representing a VO line to
/// play in a conversation. The VO Controller moves from node to node playing lines for
/// chosen participants.
//---------------------------------------------------------------------------------------
UCLASS(BlueprintType)
class OSECORE_API UOSEVoiceOverConversation 
   : public UOSEVoiceOverBucketItem
{
   GENERATED_BODY()
public:

#if WITH_EDITORONLY_DATA
   UPROPERTY()
   UEdGraph* EdGraph;
#endif

   // Do we search for participants spacially around the speaker?  Or use the passed in set of speakers?
   UPROPERTY(EditAnywhere)
   bool FindParticipants = true;

   UPROPERTY(EditAnywhere, meta = (EditCondition = "FindParticipants", EditConditionHides))
   TArray<FOSEVoiceOverConversationParticipantInfo> Participants;

   UPROPERTY()
   TArray<UOSEVoiceOverConversationNode*> ConversationRoots;

   UOSEVoiceOverConversationNode* ChooseStartingNode(const TArray<AActor*>& participants, FRandomStream &randomStream) const;
   float GetParticipantSearchRadius() const;
};

