// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ose
#include "VoiceOver/OSEVoiceOverLine.h"

// ue4
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "OSEVoiceOverControllerComponent.generated.h"

struct FOSEVoiceOverLineRequestParams;
class UAkAudioEvent;
class AOSEPlayerState;
class AOSEGameState;
class UOSEVoiceOverConversation;
class UOSEVoiceOverConversationNode;


DECLARE_LOG_CATEGORY_EXTERN(LogOSEVoiceController, Log, All);


class UOSEVoiceOverControllerComponent;


//---------------------------------------------------------------------------------------
/// FOSEVoiceOverRequest
///
/// Represents a single VO request in the queue of the VoiceController.
//---------------------------------------------------------------------------------------

USTRUCT()
struct FOSEVoiceOverRequest
{
   GENERATED_USTRUCT_BODY()

   UPROPERTY()
   UOSEVoiceOverLine* VoiceLine = nullptr;

   UPROPERTY()
   UOSEVoiceOverConversation* Conversation = nullptr;

   UPROPERTY()
   UOSEVoiceOverBucket* Bucket = nullptr;
   
   UPROPERTY()
   float AudibilityRadius = 0;

   // Actor making the VO request, must have a VO AkPlayer Component.
   UPROPERTY()
   TWeakObjectPtr<AActor> VOActor;

   // Priority level used to sort the queue.
   UPROPERTY()
   int32 Priority = 0;

   // Whether this request should interrupt other requests, forcing the audio to stop.
   UPROPERTY()
   bool Interrupting = false;

   // A bit mask representing which players can hear this VO line.
   UPROPERTY()
   int AudibilityMask = 0;

   // Server Authority Time at which to remove from the request queue.
   UPROPERTY()
   float ExpirationTime = TNumericLimits<float>::Max();

   UPROPERTY()
   TArray<TObjectPtr<AActor>> ForcedParticipants;
};


//---------------------------------------------------------------------------------------
/// FOSEVoiceOverPlayingRequest
///
/// Represents currently playing voice over lines or conversations.
//---------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FOSEVoiceOverPlayingRequest : public FFastArraySerializerItem
{
   GENERATED_USTRUCT_BODY()
   
   // Start time on the server of this voice request
   UPROPERTY(BlueprintReadOnly)
   float AuthorityStartTime = 0;

   // How long this playing voice over should last.
   UPROPERTY(BlueprintReadOnly)
   float Duration = 0;

   UPROPERTY(BlueprintReadOnly)
   UAkAudioEvent* AudioEvent = nullptr;

   UPROPERTY(BlueprintReadOnly)
   UOSEVoiceOverConversationNode* CurrentNode = nullptr;

   UPROPERTY(BlueprintReadOnly)
   TArray<AActor*> Speakers;

   UPROPERTY(BlueprintReadOnly)
   TArray<AActor*> ForcedParticipants;

   // Whether this request should interrupt other requests, forcing the audio to stop.
   UPROPERTY(BlueprintReadOnly)
   bool Interrupting = false;

   // The AkPlayingID of the audio, valid once playing.
   // Only valid on clients. On the server AkAudio doesn't play events, so the PlayingID remains 0.
   UPROPERTY(BlueprintReadOnly, NotReplicated)
   int AkPlayingID = 0;

   // A bit mask representing which players can hear this VO line.
   UPROPERTY(BlueprintReadOnly, NotReplicated)
   int AudibilityMask = 0;

   UPROPERTY(BlueprintReadOnly)
   int32 Priority = 0;

   int GetSpeakerIndex() const; 
   AActor* GetSpeakerActor() const;
   UAkComponent* GetSpeakerVOComponent() const;
   
   bool HasFinished(float serverTime) const;
   
   void NotifySpeakersInterrupted();
   void NotifySpeakersFinished();
   void NotifySpeakersStarted();
};

class UOSEVoiceOverControllerComponent;

//---------------------------------------------------------------------------------------
/// FOSEVoiceOverPlayingList
///
/// A Delta serialized list of playing requests. This handles proper replication events
/// in less than ideal network conditions.
//---------------------------------------------------------------------------------------
USTRUCT()
struct FOSEVoiceOverPlayingList : public FFastArraySerializer
{
   GENERATED_BODY()

   void SetController(UOSEVoiceOverControllerComponent* controller);
   bool NetDeltaSerialize(FNetDeltaSerializeInfo& deltaParms);

   // ---------------------------------------------------------
   //Give this a TArray like interface.

   FOSEVoiceOverPlayingRequest& operator[](int32 index) { return _requests[index]; }
   const FOSEVoiceOverPlayingRequest& operator[](int32 index) const { return _requests[index]; }

   class FIterator : public TArray<FOSEVoiceOverPlayingRequest>::TIterator
   {
   public:
      FIterator(FOSEVoiceOverPlayingList& list)
         : TArray<FOSEVoiceOverPlayingRequest>::TIterator(list._requests)
         , _list(list)
      {}

      void RemoveCurrent(); //Intentially Cover the TArray<>::TIterator version so we can mark the array as dirty.
   private:
      FOSEVoiceOverPlayingList& _list;
   };

   class FConstIterator : public TArray<FOSEVoiceOverPlayingRequest>::TConstIterator
   {
   public:
      FConstIterator(const FOSEVoiceOverPlayingList& list)
         : TArray<FOSEVoiceOverPlayingRequest>::TConstIterator(list._requests)
      {}
   };

   FIterator CreateIterator() { return FIterator(*this); }
   FConstIterator CreateConstIterator() const { return FConstIterator(*this); }

   auto begin() { return _requests.begin(); }
   auto end() { return _requests.end(); }
   auto begin() const { return _requests.begin(); }
   auto end() const { return _requests.end(); }

   int32 Add(const FOSEVoiceOverPlayingRequest& request);
   int32 Num() const { return _requests.Num(); }
   //--------------------------------------------------------------

   /**
    * Replication functions, Non-Virtual FFastArraySerializer uses templates to call these.
    */
   void PreReplicatedRemove(const TArrayView<int32>& removedIndices, int32 finalSize);
   void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
   void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
private:
   UPROPERTY(Transient)
   TArray<FOSEVoiceOverPlayingRequest> _requests;

   UPROPERTY(Transient, NotReplicated)
   UOSEVoiceOverControllerComponent* _controller = nullptr;
};

template<>
struct TStructOpsTypeTraits<FOSEVoiceOverPlayingList> : public TStructOpsTypeTraitsBase2<FOSEVoiceOverPlayingList>
{
   enum
   {
      WithNetDeltaSerializer = true,
   };
};

//---------------------------------------------------------------------------------------
/// UOSEVoiceOverControllerComponent
///
/// The central point for both NPC and Player VO. The VO Controller acts as a traffic
/// control system so players don't hear overlapping VO.
//---------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API UOSEVoiceOverControllerComponent : public UActorComponent
{
   GENERATED_BODY()
public:
   UOSEVoiceOverControllerComponent();

   // Adds a new voice request using a OSEVoiceOverLine to the queue.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (GameplayTagFilter = "VoiceOver.Priority"))
   void AuthorityRequestVOLine(AActor* actor, UOSEVoiceOverLine* line, FGameplayTag priorityTag, bool interrupting, float timeInQueue = -1.f);

   // Adds a new voice request using a OSEVoiceOverLine to the queue.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (GameplayTagFilter = "VoiceOver.Priority"))
   void AuthorityRequestVOBucket(AActor* actor, UOSEVoiceOverBucket* bucket, FGameplayTag priorityTag, bool interrupting, float timeInQueue = -1.f);

   // Plays a VO Line skipping the queue. Only works if the speaker isn't speaking or can be interrupted by this priority level.
   UFUNCTION(BlueprintCallable, meta = (GameplayTagFilter = "VoiceOver.Priority"))
   void RequestGrunt(AActor* actor, UOSEVoiceOverLine* line, FGameplayTag priorityTag, bool interrupting);

   // Adds a new voice request using a OSEVoiceOverConversation to the queue.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (GameplayTagFilter = "VoiceOver.Priority"))
   void AuthorityRequestVOConversation(AActor* actor, UOSEVoiceOverConversation* conversation, FGameplayTag priorityTag, bool interrupting, TArray<AActor*> forcedParticipants, float timeInQueue = -1.f);

   // Clear any voice requests made for the supplied actor from the queue.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityClearVORequests(AActor* actor);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityHaltPlayingVO(AActor* actor);

   /// Replication
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   virtual void BeginPlay() override;

   friend struct FOSEVoiceOverPlayingList;

protected:
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
private:
   void _UpdateAudibility();
   
   void _UpdateGrunts();

   float _GetExpirationTime(float timeInQueue) const;

   float _GetAudibilityRadius(UAkAudioEvent* audioEvent);
   bool _IsAudible(APawn* playerPawn, AActor* voActor, float radius);
   void _UpdateRequestAudibility(FOSEVoiceOverRequest& request, const TArray<AOSEPlayerState*>& playerStates);
   void _UpdatePlayingAudibility(FOSEVoiceOverPlayingRequest& playingRequest, const TArray<AOSEPlayerState*>& playerStates);

   bool _SetPlayingVoiceLine(FOSEVoiceOverPlayingRequest& playingRequest, UOSEVoiceOverLine* line, const UAkComponent* voComponent, const AActor* voActor);
   bool _SetPlayingConversation(FOSEVoiceOverPlayingRequest& playingRequest, const UOSEVoiceOverConversation* conversation, const UAkComponent* voComponent);
   bool _FindConversationParticipants(FOSEVoiceOverPlayingRequest& playingRequest, const UOSEVoiceOverConversation* conversation, const UAkComponent* voComponent, const UWorld* world);

   bool _AdvanceConversation(FOSEVoiceOverPlayingRequest& finishedRequest);
   
   void _PlayRequest(FOSEVoiceOverPlayingRequest& playingRequest, AOSEGameState* gameState) const;
   void _PlayGrunt(FOSEVoiceOverPlayingRequest& playingRequest) const;
   void _HaltPlayingRequest(FOSEVoiceOverPlayingRequest& playingRequest);

   bool _ShouldPlayRequest(const FOSEVoiceOverRequest &request, int playersAvailable) const;

   void _CleanUpFinishedVO();

   void _HandlePendingRequests();
   void _CleanupExpiredRequests();

   bool _GetPriorityForTag(FGameplayTag priorityTag, int32* outPriorityValue) const;

public:
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   float DefaultAudibilityDistance = 5000.0f;
private:

   //The voice request queue.
   UPROPERTY(Transient)
   TArray<FOSEVoiceOverRequest> _pendingRequests;

   //List of voice requests currently playing
   UPROPERTY(Transient, Replicated)
   FOSEVoiceOverPlayingList _playingVO;

   //List of currently playing grunts, not replicated only client simulated.
   UPROPERTY(Transient)
   TArray<FOSEVoiceOverPlayingRequest> _playingGrunts;

   //Bit mask for which players are currently hearing VO.
   UPROPERTY(Transient)
   int _playingMask;

   // Data table consisting of `FOSEVoiceOverPriorityRow` rows
   UPROPERTY(Transient)
   UDataTable* _voiceOverPriority;

   TMap<FGameplayTag, int32> _priorityTagToPriorityValue;

   FRandomStream _randomStream;
};
