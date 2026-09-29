// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverControllerComponent.h"

//ose
#include "Audio/OSEAkAudioComponentSystemInterface.h"
#include "Online/OSEGameState.h"
#include "OSEProjectSettings.h"
#include "Player/OSEPlayerCharacter.h"
#include "Player/OSEPlayerState.h"
#include "VoiceOver/OSEVoiceOverBucket.h"
#include "VoiceOver/OSEVoiceOverConversation.h"
#include "VoiceOver/OSEVoiceOverConversationNode.h"
#include "VoiceOver/OSEVoiceOverEventHandlerInterface.h"
#include "VoiceOver/OSEVoiceOverLine.h"
#include "VoiceOver/OSEVoiceOverLineRequestParams.h"
#include "VoiceOver/OSEVoiceOverParticipantSubsystem.h"
#include "VoiceOver/OSEVoiceOverPriority.h"

//ue4
#include "Engine/OverlapResult.h"
#include "AkAudioEvent.h"
#include "AkComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverControllerComponent)

DEFINE_LOG_CATEGORY(LogOSEVoiceController);

static float AuthorityTimeOffsetTolerance = 0.5f;
FAutoConsoleVariableRef CVarAuthorityTimeOffsetTolerance(
   TEXT("OSE.VO.AuthorityTimeOffsetTolerance"),
   AuthorityTimeOffsetTolerance,
   TEXT("Maximum offset from servertime to play VO lines from the beginning."),
   ECVF_Default);



int FOSEVoiceOverPlayingRequest::GetSpeakerIndex() const
{
   int speakerIndex = IsValid(CurrentNode) ? CurrentNode->SpeakerIndex : 0; //Use the conversation node speaker index if available, if not the speaker must be 0.
   check(speakerIndex >= 0 && speakerIndex < Speakers.Num());
   return speakerIndex;
}

namespace VOHelpers {
   static UAkComponent* GetVOComponent(AActor* speaker)
   {
      if (!IsValid(speaker))
         return nullptr;

      checkf(speaker->Implements<UOSEAkAudioComponentSystemInterface>(), TEXT("Speaker %s does not implement OSEAkAudioComponentSystemInterface"), *speaker->GetName());
      UAkComponent* voComponent = IOSEAkAudioComponentSystemInterface::Execute_GetAkComponent(speaker, EAkComponentType::Voice);
      checkf(IsValid(voComponent), TEXT("Speaker %s does not have valild VO Component"), *speaker->GetName());
      checkf(voComponent->Implements<UOSEVoiceOverEventHandlerInterface>(), TEXT("Speaker %s's VO Component does not implement OSEVoiceOverEventHandlerInterface"), *speaker->GetName());
      return voComponent;
   }

   //Do all the appropriate checks and log error messages here. Use this to prevent actors without the appropriate interfaces and components
   //from entering the VO queue, this way all the checks inside of the system can be asserts.
   static bool IsValidRequestActor(AActor* speaker, bool logErrors = true)
   {
      if (!IsValid(speaker))
      {
         UE_CLOG(logErrors, LogOSEVoiceController, Error, TEXT("Attempted to request VO with no target actor. This request was ignored."));
         return false;
      }

      if (!speaker->Implements<UOSEAkAudioComponentSystemInterface>())
      {
         UE_CLOG(logErrors, LogOSEVoiceController, Error, TEXT("Attempted to request VO with a target actor (%s) that doesn't implement UOSEAkAudioComponentSystemInterface. This request was ignored."), *speaker->GetName());
         return false;
      }

      UAkComponent* voComponent = IOSEAkAudioComponentSystemInterface::Execute_GetAkComponent(speaker, EAkComponentType::Voice);
      if (!IsValid(voComponent))
      {
         UE_CLOG(logErrors, LogOSEVoiceController, Error, TEXT("Attempted to request VO with a target actor (%s) that doesn't supply a voice akcomponent. This request was ignored."), *speaker->GetName());
         return false;
      }

      if (!voComponent->Implements<UOSEVoiceOverEventHandlerInterface>())
      {
         UE_CLOG(logErrors, LogOSEVoiceController, Error, TEXT("Attempted to request VO with a target actor (%s) with a voice akcomponent that doesn't implement UOSEVoiceOverEventHandlerInterface. This request was ignored."), *speaker->GetName());
         return false;
      }

      return true;
   }

   static void FilterValidForcedParticipants(AActor* speaker, TArray<AActor*>& speakers)
   {
      // throw out actors that can't speak, and any cases where we accidentally pass the speaker that made the request into the list of forced participants
      speakers.RemoveAll([=](AActor* actor)
      {
         return actor == speaker || !VOHelpers::IsValidRequestActor(actor);
      });
   }

   static FVector GetLocationForComponent(const USceneComponent* component)
   {
      check(component);

      // prefer to use owner location, since the vo component may be detached
      if(const AActor* actor = component->GetOwner())
      {
         return actor->GetActorLocation();
      }

      return component->GetComponentLocation();
   }
}

AActor* FOSEVoiceOverPlayingRequest::GetSpeakerActor() const
{
   return Speakers[GetSpeakerIndex()];
}

UAkComponent* FOSEVoiceOverPlayingRequest::GetSpeakerVOComponent() const
{
   AActor* speaker = GetSpeakerActor();
   return VOHelpers::GetVOComponent(speaker);
}

bool FOSEVoiceOverPlayingRequest::HasFinished(float serverTime) const
{
   if (CurrentNode)
   {
      // A note / future TODO about this: if the FixedDuration (or Instant) is less than the total time it would take audio to play
      // it can result in PostAkEvent being called a second time for a single speaker for a future line.  For the time being, the audio team will 
      // handle that on the wwise side, only allowing an actor to speak one line at a time.  It would be better if we could advance a conversation along
      // but still prevent the speaker from saying any new lines till these are completed.  In the short term, this functionality allows squad callouts
      // to take just a couple seconds, rather than 10+ when a handful of guards are trying to take part in a conversation.
      
      // for conversations we can ask the node how long it wants to wait
      switch(CurrentNode->DurationType)
      {
      case EOSEVoiceOverConversationNodeDurationType::AudioDuration:
         return AuthorityStartTime + Duration < serverTime;
      case EOSEVoiceOverConversationNodeDurationType::FixedDuration:
         return AuthorityStartTime + CurrentNode->FixedDuration < serverTime;
      case EOSEVoiceOverConversationNodeDurationType::Instant:
            return true;
      default:
         unimplemented();
         return true; // don't block I guess?
      }
   }
   else
   {
      // else, use audio duration
      return AuthorityStartTime + Duration < serverTime;
   }
}

void FOSEVoiceOverPlayingRequest::NotifySpeakersInterrupted()
{
   for (AActor* speaker : Speakers)
   {
      UAkComponent* voComponent = VOHelpers::GetVOComponent(speaker);
      if (IsValid(voComponent))
      {
         IOSEVoiceOverEventHandlerInterface::Execute_OnVoiceOverRequestInterrupted(voComponent, *this);
      }
   }
}

void FOSEVoiceOverPlayingRequest::NotifySpeakersFinished()
{
   for (AActor* speaker : Speakers)
   {
      UAkComponent* voComponent = VOHelpers::GetVOComponent(speaker);
      if (IsValid(voComponent))
      {
         IOSEVoiceOverEventHandlerInterface::Execute_OnVoiceOverRequestFinished(voComponent, *this);
      }
   }
}

void FOSEVoiceOverPlayingRequest::NotifySpeakersStarted()
{
   for (AActor* speaker : Speakers)
   {
      UAkComponent* voComponent = VOHelpers::GetVOComponent(speaker);
      if (IsValid(voComponent))
      {
         IOSEVoiceOverEventHandlerInterface::Execute_OnVoiceOverRequestStarted(voComponent, *this);
      }
   }
}

UOSEVoiceOverControllerComponent::UOSEVoiceOverControllerComponent()
   : Super()
   , _playingMask(0)
{
   _playingVO.SetController(this);

   PrimaryComponentTick.bCanEverTick = true;
   SetIsReplicatedByDefault(true);

   //TODO: Use the level randomStream, but for now seed it using the time.
   int32 seed = ((int32)(FDateTime::Now().GetTicks() % (int64)MAX_int32));
   _randomStream.Initialize(seed);
}

void UOSEVoiceOverControllerComponent::BeginPlay()
{
   Super::BeginPlay();

   _voiceOverPriority = UOSEProjectSettings::Get().VoiceOverPriority.LoadSynchronous();

   _priorityTagToPriorityValue.Reset();
   if (_voiceOverPriority)
   {
      _voiceOverPriority->ForeachRow<FOSEVoiceOverPriorityRow>(TEXT("VOPriority"),
      [&](FName, const FOSEVoiceOverPriorityRow& dataRow)
      {
         if (_priorityTagToPriorityValue.Contains(dataRow.PriorityTag))
         {
            UE_LOG(LogOSEVoiceController, Warning, TEXT("VoiceOverPriority data table '%s' contains two rows with the same tag: '%s'"),
               *_voiceOverPriority->GetName(),
               *dataRow.PriorityTag.ToString());
         }
         _priorityTagToPriorityValue.Add(dataRow.PriorityTag, dataRow.Priority);
      });
   }
   else
   {
      UE_LOG(LogOSEVoiceController, Warning, TEXT("Could not load VO priority data table at '%s'"), *UOSEProjectSettings::Get().VoiceOverPriority.GetAssetName());
   }
}

void UOSEVoiceOverControllerComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   QUICK_SCOPE_CYCLE_COUNTER(OSEVoiceOverControllerComponent_Tick);

   AActor* owner = GetOwner();
   if (owner->HasAuthority())
   {
      _CleanUpFinishedVO();

      _UpdateAudibility();

      _HandlePendingRequests();

      _CleanupExpiredRequests();
   }

   _UpdateGrunts();
}

void UOSEVoiceOverControllerComponent::_UpdateGrunts()
{
   AActor* owner = GetOwner();

   AOSEGameState* gameState = CastChecked<AOSEGameState>(owner);

   float serverTime = gameState->GetServerWorldTimeSeconds();

   for (auto playingIt = _playingGrunts.CreateIterator(); playingIt; ++playingIt)
   {
      if (playingIt->HasFinished(serverTime))
      {
         FOSEVoiceOverPlayingRequest& finishedRequest = *playingIt;

         UAkComponent* voComponent = finishedRequest.GetSpeakerVOComponent();
         if (IsValid(voComponent))
         {
            //Only run events and attempt to continue a conversation if the VO Component is still valid, the actor might be marked pending kill.
            IOSEVoiceOverEventHandlerInterface::Execute_OnVoiceOverLineFinished(voComponent, finishedRequest);
         }

         UE_LOG(LogOSEVoiceController, Verbose, TEXT("Finished Grunt %s:"), *finishedRequest.AudioEvent->GetName());

         playingIt->NotifySpeakersFinished();

         //remove it from the playing list
         playingIt.RemoveCurrent();
      }
   }
}

bool UOSEVoiceOverControllerComponent::_AdvanceConversation(FOSEVoiceOverPlayingRequest& finishedRequest)
{
   if (!IsValid(finishedRequest.CurrentNode))
      return false; //Not a Conversation

   AActor* owner = GetOwner();
   check(owner->HasAuthority());

   AOSEGameState* gameState = CastChecked<AOSEGameState>(owner);

   //Choose a next node.
   UOSEVoiceOverConversationNode* newNode = finishedRequest.CurrentNode->FindNextNode(finishedRequest.Speakers, _randomStream);

   if (!IsValid(newNode))
   {
      return false;
   }

   UE_LOG(LogOSEVoiceController, Verbose, TEXT("Advancing Conversation: %s"), newNode->Conversation ? *newNode->Conversation->GetName() : TEXT("Unknown"));
   if (newNode->SpeakerIndex < 0 || newNode->SpeakerIndex >= finishedRequest.Speakers.Num())
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Conversation Node in %s has invalid speaker index %d."), newNode->Conversation ? *newNode->Conversation->GetName() : TEXT("Unknown"), newNode->SpeakerIndex);
      return false;
   }

   AActor* newSpeaker = finishedRequest.Speakers[newNode->SpeakerIndex];
   if (!IsValid(newSpeaker))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Conversation's (%s) next speaker following %s is invalid."), newNode->Conversation ? *newNode->Conversation->GetName() : TEXT("Unknown"), finishedRequest.GetSpeakerActor() ? *finishedRequest.GetSpeakerActor()->GetName() : TEXT("Unknown"));
      return false;
   }

   UAkComponent* newVOComponent = VOHelpers::GetVOComponent(newSpeaker);
   if (!IsValid(newVOComponent))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Conversation's (%s) next speaker's %s vo component is invalid."), newNode->Conversation ? *newNode->Conversation->GetName() : TEXT("Unknown"), *newSpeaker->GetName());
      return false;
   }

   if (_SetPlayingVoiceLine(finishedRequest, newNode->Line, newVOComponent, newSpeaker))
   {
      finishedRequest.CurrentNode = newNode;
      finishedRequest.AuthorityStartTime = gameState->GetServerWorldTimeSeconds();
      finishedRequest.AkPlayingID = 0;
      _PlayRequest(finishedRequest, gameState);
      _playingVO.MarkItemDirty(finishedRequest);
      return true;
   }

   return false;
}


void UOSEVoiceOverControllerComponent::_CleanUpFinishedVO()
{
   AActor* owner = GetOwner();
   check(owner->HasAuthority());

   AOSEGameState* gameState = CastChecked<AOSEGameState>(owner);

   float serverTime = gameState->GetServerWorldTimeSeconds();

   for (auto playingIt = _playingVO.CreateIterator(); playingIt; ++playingIt)
   {  
      if (playingIt->HasFinished(serverTime))
      {
         FOSEVoiceOverPlayingRequest& finishedRequest = *playingIt;

         UAkComponent* voComponent = finishedRequest.GetSpeakerVOComponent();
         if (IsValid(voComponent))
         {
            //Only run events and attempt to continue a conversation if the VO Component is still valid, the actor might be marked pending kill.
            IOSEVoiceOverEventHandlerInterface::Execute_OnVoiceOverLineFinished(voComponent, finishedRequest);

            if (_AdvanceConversation(finishedRequest))
            {
               //This request has a conversation that has advanced to the next node, so we keep it in the playing list.
               continue;
            }
         }

         //Remove audiblity from the playing mask.
         _playingMask &= ~finishedRequest.AudibilityMask;

         UE_LOG(LogOSEVoiceController, Verbose, TEXT("Finished %s for players %d"), *finishedRequest.AudioEvent->GetName(), finishedRequest.AudibilityMask);

         playingIt->NotifySpeakersFinished();

         //remove it from the playing list
         playingIt.RemoveCurrent();
      }
   }
}

void UOSEVoiceOverControllerComponent::_UpdateAudibility()
{
   AActor* owner = GetOwner();
   check(owner->HasAuthority());
   
   AOSEGameState* gameState = CastChecked<AOSEGameState>(owner);
   const TArray<AOSEPlayerState*>& playerStates = gameState->GetOSEPlayerStates();

   for (FOSEVoiceOverPlayingRequest& playingRequest : _playingVO)
   {
      _UpdatePlayingAudibility(playingRequest, playerStates);
   }

   for (FOSEVoiceOverRequest& request : _pendingRequests)
   {
      _UpdateRequestAudibility(request, playerStates);
   }
}

float UOSEVoiceOverControllerComponent::_GetAudibilityRadius(UAkAudioEvent* audioEvent)
{
   float audibilityDistance = DefaultAudibilityDistance;
   if (IsValid(audioEvent) && audioEvent->MaxAttenuationRadius != 0.0f)
   {
      audibilityDistance = audioEvent->MaxAttenuationRadius;
   }
   return audibilityDistance;
}

bool UOSEVoiceOverControllerComponent::_IsAudible(APawn* playerPawn, AActor* voActor, float audibilityDistance)
{
   UAkComponent* voComponent = VOHelpers::GetVOComponent(voActor);
   if (!IsValid(voComponent))
      return false;

   float distanceSqr = FVector::DistSquared(voActor->GetActorLocation(), playerPawn->GetActorLocation());

   //TODO: For now we're doing a basic distance check. Eventually we'll want to modify this audiblity check to interface with whatever sound prop solution is used.
   //TODO: Check if there's a component used for listening, the position for listening might be different than the actor's position.
   return distanceSqr <= FMath::Square(audibilityDistance);
}

void UOSEVoiceOverControllerComponent::_UpdateRequestAudibility(FOSEVoiceOverRequest& request, const TArray<AOSEPlayerState*>& playerStates)
{
   int newAudibility = 0;

   for (auto playerIt = playerStates.CreateConstIterator(); playerIt; ++playerIt)
   {
      const AOSEPlayerState* playerState = *playerIt;

      //It's possible to tick the VO Controller inbetween playerState creation and that player possessing a pawn which will result in a null pawn pointer.
      //So for this frame we ignore their audibility and we'll catch it the next one.
      if (!IsValid(playerState) || !IsValid(playerState->GetPawn()))
      {
         continue;
      }

      if (!request.VOActor.IsValid())
      {
         continue;
      }

      bool audible = _IsAudible(playerState->GetPawn(), request.VOActor.Get(), request.AudibilityRadius);
      
      if (audible)
      {
         //Use the index in playerStates since it's stable sorted and synced between server and client.
         //This VO is audibile to this player, mark it in the mask
         check(playerIt.GetIndex() < sizeof(request.AudibilityMask) * 8);
         newAudibility |= 1 << playerIt.GetIndex();
      }
   }

   request.AudibilityMask = newAudibility;
}

void UOSEVoiceOverControllerComponent::_UpdatePlayingAudibility(FOSEVoiceOverPlayingRequest& playingRequest, const TArray<AOSEPlayerState*>& playerStates)
{
   int newAudibility = 0;

   for (auto playerIt = playerStates.CreateConstIterator(); playerIt; ++playerIt)
   {
      const AOSEPlayerState* playerState = *playerIt;
      //It's possible to tick the VO Controller inbetween playerState creation and that player possessing a pawn which will result in a null pawn pointer.
      //So for this frame we ignore their audibility and we'll catch it the next one.
      if (!IsValid(playerState) || !IsValid(playerState->GetPawn()))
      {
         continue;
      }
     
      AActor* voActor = playingRequest.Speakers[0];
      if (!IsValid(voActor))
      {
         continue;
      }

      if (_IsAudible(playerState->GetPawn(), voActor, _GetAudibilityRadius(playingRequest.AudioEvent)))
      {
         //Use the index in playerStates since it's stable sorted and synced between server and client.
        //This VO is audibile to this player, mark it in the mask
         check(playerIt.GetIndex() < sizeof(playingRequest.AudibilityMask) * 8);
         newAudibility |= 1 << playerIt.GetIndex();
      }
   }

   playingRequest.AudibilityMask = newAudibility;
}

bool UOSEVoiceOverControllerComponent::_SetPlayingVoiceLine(FOSEVoiceOverPlayingRequest& playingRequest, UOSEVoiceOverLine* line, const UAkComponent* voComponent, const AActor* voActor)
{
   check(IsValid(line));
   check(IsValid(voComponent));
   check(IsValid(voActor));

   const FOSEVoiceOverLineIdentityData* identityData = line->GetIdentityData(voComponent);

   if (!identityData)
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("VoiceLine %s is missing identity data for actor %s"), *line->GetName(), *voActor->GetName());
      return false;
   }

   const FOSEVoiceOverLineData voLineData = line->ResolveVoiceLineData(_randomStream, voComponent);

   UAkAudioEvent* audioEvent = nullptr;
   float duration = -1.f;

   // Prefer audio event provided by VOL data
   if (IsValid(voLineData.AudioEvent))
   {
      audioEvent = voLineData.AudioEvent;
      duration = voLineData.AudioEvent->MaximumDuration;
      
      // If min/max duration don't match, prefer max to avoid a line being interrupted 
      const bool hasDifferentMinMaxDurations = voLineData.AudioEvent->MinimumDuration != voLineData.AudioEvent->MaximumDuration;
      UE_CLOG(hasDifferentMinMaxDurations, LogOSEVoiceController, Error, 
         TEXT("Selected VOL data has audio event %s with inconsistent MinimumDuration (%f) / MaximumDuration (%f)! Will use maximum duration...")
         , *voLineData.AudioEvent->GetName()
         , voLineData.AudioEvent->MinimumDuration
         , voLineData.AudioEvent->MaximumDuration);
   }
   else
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Selected a FOSEVoiceOverLineData from VoiceLine %s with missing audio event required to play for %s."), *line->GetName(), *voActor->GetName());
      return false;
   }

   if (duration <= 0)
   {
#if UE_SERVER
      // TODO: Remove the assumption that a dedicated server needs to know the exact length of a VoiceLine.
      //       Even if we cooked this data for dedicated servers (which we don't), supporting players with different
      //       localization settings means that the duration of a single VoiceLine could vary even within a single match.

      // HACK: Setting a fake duration because we don't have access to the cooked audio data on dedicated servers
      duration = 4.0f;
#else
      UE_LOG(LogOSEVoiceController, Error, TEXT("Selected a FOSEVoiceOverLineData from VoiceLine with a duration of %f!"), duration);
      return false;
#endif
   }

   playingRequest.AudioEvent = audioEvent;
   playingRequest.Duration = duration;

   return true;
}

bool UOSEVoiceOverControllerComponent::_SetPlayingConversation(FOSEVoiceOverPlayingRequest& playingRequest, const UOSEVoiceOverConversation* conversation, const UAkComponent* voComponent)
{
   AActor* owner = GetOwner();
   check(owner->HasAuthority());
   check(IsValid(conversation));

   if (conversation->Participants.Num() < 1)
   {
      // Must have at least one partipant.
      return false;
   }

   //Check the speaker's conditions, _FindConversationParticipants assumes Speaker[0] is valid.
   FOSEConditionContext conditionContext(playingRequest.Speakers[0]);
   if (!conversation->Participants[0].Conditions.Satisfied(conditionContext))
   {
      // Speaker[0] isn't valid.
      return false;
   }

   if (!_FindConversationParticipants(playingRequest, conversation, voComponent, owner->GetWorld()))
   {
      //We failed to find all the participants
      return false;
   }

   //Choose starting node
   UOSEVoiceOverConversationNode* startNode = conversation->ChooseStartingNode(playingRequest.Speakers, _randomStream);

   if (!IsValid(startNode))
   {
      //There's no valid starting node.
      return false;
   }

   UE_LOG(LogOSEVoiceController, Verbose, TEXT("Starting Conversation %s"), *conversation->GetName());

   for (AActor* speaker : playingRequest.Speakers)
   {
      UE_LOG(LogOSEVoiceController, Verbose, TEXT("  Speaker: %s"), *speaker->GetFName().ToString());
   }

   if (startNode->SpeakerIndex < 0 || startNode->SpeakerIndex >= playingRequest.Speakers.Num())
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Requested conversation %s has an invalid speaker index %d"), *conversation->GetName(), startNode->SpeakerIndex);
      return false;
   }

   //Set the voice line.
   if (!_SetPlayingVoiceLine(playingRequest, startNode->Line, voComponent, playingRequest.Speakers[startNode->SpeakerIndex]))
   {
      return false;
   }

   playingRequest.CurrentNode = startNode;
   return true;
}

struct FConversationParticipant
{
   AActor* Actor;
   UAkComponent* VOComponent;
   float DistanceSq;
};

bool UOSEVoiceOverControllerComponent::_FindConversationParticipants(FOSEVoiceOverPlayingRequest &playingRequest, const UOSEVoiceOverConversation* conversation, const UAkComponent* voComponent, const UWorld* world)
{
   check(IsValid(world));
   check(IsValid(voComponent));
   check(IsValid(conversation));

   if (conversation->FindParticipants)
   {
      const AActor* voActor = playingRequest.Speakers[0];

      float audibilityRadius = conversation->GetParticipantSearchRadius();

      //Find Participants.
      TArray<FConversationParticipant> participantCandidates;
      const auto* participantSubsystem = GetWorld()->GetSubsystem<UOSEVoiceOverParticipantSubsystem>();
      check(participantSubsystem);


      participantSubsystem->FindPossibleParticipantsInRange(VOHelpers::GetLocationForComponent(voComponent), audibilityRadius, voComponent->GetOwner(), [voActor, &participantCandidates](AActor* candidate) {
         // Make sure the actor actually has a VO component and is set up for requests,
         // but since we expect other pawns to be in the overlap list, don't log errors if
         // we don't find a valid VO component
         constexpr bool logErrors = false;
         if (VOHelpers::IsValidRequestActor(candidate, logErrors))
         {
            UAkComponent* participantVoComponent = VOHelpers::GetVOComponent(candidate);
            participantCandidates.Add({ candidate, participantVoComponent, static_cast<float>(FVector::DistSquared(candidate->GetActorLocation(), voActor->GetActorLocation())) });
         }
      });

      if (participantCandidates.Num() == 0)
      {
         return false;
      }


      //Sort potential conversation participants by their distance from the first person
      participantCandidates.Sort([](const FConversationParticipant& a, const FConversationParticipant& b)
         {
            return a.DistanceSq < b.DistanceSq;
         });

      TArray<AActor*> result;
      // Greedily choose participants as they match our requirements.
      // While a greedy approach could lead to failed matchings it is considerably cheaper than other matching algorithms.
      auto matcherIt = conversation->Participants.CreateConstIterator();
      ++matcherIt; //We've already selected the first participant.
      while(matcherIt)
      {
         bool found = false;
         for (auto candidateIt = participantCandidates.CreateIterator(); candidateIt; ++candidateIt)
         {
            check(candidateIt->VOComponent->Implements<UOSEVoiceOverEventHandlerInterface>());
            FGameplayTag identity = IOSEVoiceOverEventHandlerInterface::Execute_GetVoiceIdentity(candidateIt->VOComponent);
            FOSEConditionContext conditionContext(candidateIt->Actor);

            if (matcherIt->AcceptedTags.Matches(identity.GetSingleTagContainer()) && matcherIt->Conditions.Satisfied(conditionContext))
            {
               //Use this actor as a speaker
               playingRequest.Speakers.Add(candidateIt->Actor);
            
               //Remove it from the candidates.
               candidateIt.RemoveCurrent();
               ++matcherIt;
               found = true;
               break;
            }
         }

         if (!found)
         {
            break;
         }
      }

      if (matcherIt)
      {
         //We couldn't find enough conversation participants
         return false;
      }
   }
   else
   {
      // forced participants can be passed directly into AuthorityRequestVOConversation
      playingRequest.Speakers.Append(playingRequest.ForcedParticipants);
   }

   return true;
}

bool UOSEVoiceOverControllerComponent::_ShouldPlayRequest(const FOSEVoiceOverRequest& request, int playersAvailable) const
{
   if (request.AudibilityMask == 0)
      return false;

   if (request.Interrupting)
   {
      for (const FOSEVoiceOverPlayingRequest& playingRequest : _playingVO)
      {
         if ((playingRequest.AudibilityMask & request.AudibilityMask) != 0)
         {
            if (playingRequest.Interrupting && playingRequest.Priority >= request.Priority)
            {
               return false;
            }
         }
      }

      for (const FOSEVoiceOverPlayingRequest& playingGrunt : _playingGrunts)
      {
         //We can use Speakers[0] here since grunts will always only have one speaker.
         check(playingGrunt.Speakers.Num() > 0);
         if (request.VOActor == playingGrunt.Speakers[0] && playingGrunt.Priority >= request.Priority)
         {
            return false;
         }
      }
      return true;
   }
   else
   {
      //For non-interrupting requests make sure every player in the AudibilityMask is also available.
      return (request.AudibilityMask & playersAvailable) == request.AudibilityMask;
   }
}

void UOSEVoiceOverControllerComponent::_HandlePendingRequests()
{
   AActor* owner = GetOwner();
   check(owner->HasAuthority());

   AOSEGameState* gameState = CastChecked<AOSEGameState>(owner);
   const TArray<AOSEPlayerState*>& playerStates = gameState->GetOSEPlayerStates();

   if (_pendingRequests.Num() == 0)
   {
      return; //There's no requests so don't bother doing anything.
   }

   //Sort our queue placing highest priority first.
   _pendingRequests.Sort([](const FOSEVoiceOverRequest& a, const FOSEVoiceOverRequest& b) 
      {
         //Sort interrupting requests to the front.
         if (a.Interrupting && !b.Interrupting) 
            return true;
         if (!b.Interrupting && b.Interrupting)
            return false;
         //If they're equally interrupting or not, sort by priority.
         return a.Priority > b.Priority;
      });

   int newPlayingMask = 0;
   for (FOSEVoiceOverPlayingRequest& playing : _playingVO)
   {
      newPlayingMask |= playing.AudibilityMask;
   }

   int allPlayersMask = 0;
   for (auto playerIt = playerStates.CreateConstIterator(); playerIt; ++playerIt)
   {
      allPlayersMask |= (1 << playerIt.GetIndex());
   }
   
   int playersAvailable = allPlayersMask ^ newPlayingMask;

   while (true)
   {
      bool foundVO = false;
      for (auto requestIt = _pendingRequests.CreateIterator(); requestIt; ++requestIt)
      {
         
         if (playersAvailable == 0 && !requestIt->Interrupting)
         {
            break;
         }

         if (_ShouldPlayRequest(*requestIt, playersAvailable))
         {
            //This request is only audibile to players who aren't currently hearing VO. So we start playing it.
            const FOSEVoiceOverRequest& chosenRequest = *requestIt;

            
            FOSEVoiceOverPlayingRequest playingRequest;
            
            playingRequest.Priority = chosenRequest.Priority;
            playingRequest.Interrupting = chosenRequest.Interrupting;
            playingRequest.AudibilityMask = chosenRequest.AudibilityMask;
            playingRequest.AuthorityStartTime = gameState->GetServerWorldTimeSeconds();

            //Add the first actor to the playing actors.
            playingRequest.Speakers.Add(chosenRequest.VOActor.Get());
            //Copy over the forced participants, if any were passed in
            playingRequest.ForcedParticipants = chosenRequest.ForcedParticipants;

            UAkComponent* voComponent = playingRequest.GetSpeakerVOComponent();
            if (!IsValid(voComponent))
            {
               //This request no longer has a valid actor attached to it. Clean it up and move on. 
               requestIt.RemoveCurrent();
               continue;
            }

            if (IsValid(chosenRequest.VoiceLine))
            {
               ensure(chosenRequest.Conversation == nullptr);
               ensure(chosenRequest.Bucket == nullptr);

               if (!_SetPlayingVoiceLine(playingRequest, chosenRequest.VoiceLine, voComponent, chosenRequest.VOActor.Get()))
               {
                  continue;
               }
            }
            else if (IsValid(chosenRequest.Conversation))
            {
               ensure(chosenRequest.VoiceLine == nullptr);
               ensure(chosenRequest.Bucket == nullptr);

               if (!_SetPlayingConversation(playingRequest, chosenRequest.Conversation, voComponent))
               {
                  continue;
               }
            }
            else if (IsValid(chosenRequest.Bucket))
            {
               ensure(chosenRequest.VoiceLine == nullptr);
               ensure(chosenRequest.Conversation == nullptr);

               FOSEConditionContext conditionContext(playingRequest.Speakers[0]);

               bool found = false;
               for (const FOSEVoiceOverBucketEntry& entry : chosenRequest.Bucket->Entries)
               {
                  if (entry.Conditions.Satisfied(conditionContext))
                  {
                     if (UOSEVoiceOverLine* line = Cast<UOSEVoiceOverLine>(entry.VoiceItem))
                     {
                        if (_SetPlayingVoiceLine(playingRequest, line, voComponent, playingRequest.Speakers[0]))
                        {
                           found = true;
                           break;
                        }
                     }
                     else if (UOSEVoiceOverConversation* conversation = Cast<UOSEVoiceOverConversation>(entry.VoiceItem))
                     {
                        if (_SetPlayingConversation(playingRequest, conversation, voComponent))
                        {
                           found = true;
                           break;
                        }
                     }
                  }
               }

               if (!found)
               {
                  continue;
               }
            }
            else
            {
               UE_LOG(LogOSEVoiceController, Error, TEXT("VO Request issued for %s is not valid"), *chosenRequest.VOActor->GetName());
               continue;
            }


            if (playingRequest.Interrupting)
            {
               for (auto playingIt = _playingVO.CreateIterator(); playingIt; ++playingIt)
               {
                  if ((playingIt->AudibilityMask & playingRequest.AudibilityMask) != 0)
                  {
                     bool hasOverlapInSpeakers = false;
                     for (AActor* speaker : playingRequest.Speakers)
                     {
                        if (playingIt->Speakers.Contains(speaker))
                        {
                           hasOverlapInSpeakers = true;
                           break;
                        }
                     }

                     // If any of the same speakers from a current line are being interrupted by the new line,
                     // then cancel the outstanding, lower-priority line.
                     // NOTE: In the future we may want finer control over interrupts, but for now this is the desired behavior
                     if (hasOverlapInSpeakers)
                     {
                        _HaltPlayingRequest(*playingIt);
                        playingIt.RemoveCurrent();
                     }
                  }
               }

               for (auto gruntIt = _playingGrunts.CreateIterator(); gruntIt; ++gruntIt)
               {
                  //Only interrupt grunts if the new request is using the speaker. (We only need to check Speakers[0] since grunts can only be single speaker)
                  check(gruntIt->Speakers.Num() > 0);
                  if (playingRequest.Speakers.Contains(gruntIt->Speakers[0]))
                  {
                     _HaltPlayingRequest(*gruntIt);
                     gruntIt.RemoveCurrent();
                  }
               }
            }

            
            //Notify blueprints it's started playing and post AkEvents if we're not a dedicated server.
            playingRequest.NotifySpeakersStarted();

            _PlayRequest(playingRequest, gameState);
            
            _playingVO.Add(playingRequest);
            
            newPlayingMask |= playingRequest.AudibilityMask;
            requestIt.RemoveCurrent();

            foundVO = true;
            break;
         }
      }

      if (!foundVO)
      {
         //Don't get stuck when there's no available VO requests to play.
         break;
      }

      playersAvailable = allPlayersMask ^ newPlayingMask;
   }

   _playingMask = newPlayingMask;
}

void UOSEVoiceOverControllerComponent::_CleanupExpiredRequests()
{
   AActor* owner = GetOwner();
   check(owner->HasAuthority());

   AOSEGameState* gameState = CastChecked<AOSEGameState>(owner);
   float worldTime = gameState->GetServerWorldTimeSeconds();

   for (auto requestIt = _pendingRequests.CreateIterator(); requestIt; ++requestIt)
   {
      if (worldTime > requestIt->ExpirationTime)
      {
         requestIt.RemoveCurrent();
      }
   }
}

bool UOSEVoiceOverControllerComponent::_GetPriorityForTag(FGameplayTag priorityTag, int32* outPriorityValue) const
{
   if (const int32* priorityValuePtr = _priorityTagToPriorityValue.Find(priorityTag))
   {
      *outPriorityValue = *priorityValuePtr;
      return true;
   }
   else
   {
      *outPriorityValue = 0;
      return false;
   }
}

void UOSEVoiceOverControllerComponent::_PlayRequest(FOSEVoiceOverPlayingRequest& playingRequest, AOSEGameState* gameState) const
{
   check(IsValid(gameState));
   check(IsValid(playingRequest.AudioEvent));
   check(playingRequest.AkPlayingID == 0);
   check(playingRequest.Speakers.Num() > playingRequest.GetSpeakerIndex());
   check(playingRequest.AuthorityStartTime != 0);

   UAkComponent* voComponent = playingRequest.GetSpeakerVOComponent();
   if (!IsValid(voComponent))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("PlayRequest run on actor without a valid voComponent. This should not happen."));
      return;
   }

   //We could potentially join the game late into an already playing VO line.
   //So we want to skip ahead in the dialog if the difference in started server time and now is above a certain threshold.
   //We need a threshold because we don't want to always clip the beginning of voice lines based on normal latency fluxuations. 
   //We only want to skip ahead in the extreme case, sort of like an an authorative positional correction for player movement.

   float authorityTimeOffset = gameState->GetServerWorldTimeSeconds() - playingRequest.AuthorityStartTime;
   float voStartTime = authorityTimeOffset > AuthorityTimeOffsetTolerance ? authorityTimeOffset : 0;

   if (GetOwner()->HasAuthority())
   {
      UE_LOG(LogOSEVoiceController, Verbose, TEXT("Authority Playing %s for players %d (%fs)"), *playingRequest.AudioEvent->GetName(), playingRequest.AudibilityMask, playingRequest.Duration);
   }
   else
   {
      UE_LOG(LogOSEVoiceController, Verbose, TEXT("Replicated Playing %s (%fs) (so %f)"), *playingRequest.AudioEvent->GetName(), playingRequest.Duration, authorityTimeOffset);
   }



   if (!IsNetMode(NM_DedicatedServer))
   {
      //Post the new external source from the selected voice line.
      playingRequest.AkPlayingID = voComponent->PostAkEvent(playingRequest.AudioEvent);


      if (voStartTime != 0)
      {
         //TODO: Skip ahead in the audio if we're above the start-time offset threshold.
         // It looks like there's missing functions in FAkAudioDevice to seek forward
         // FAkAudioDevice::Get()->SeekOnEvent() uses named events not UAkAudioEvents
         // In the next few changes we'll be modifing how these events get posted anyway
         // so this is left for a future revision.
      }
   }

   IOSEVoiceOverEventHandlerInterface::Execute_OnVoiceOverLineStarted(voComponent, playingRequest, voStartTime);
}

void UOSEVoiceOverControllerComponent::_PlayGrunt(FOSEVoiceOverPlayingRequest& playingRequest) const
{
   check(IsValid(playingRequest.AudioEvent));
   check(playingRequest.AkPlayingID == 0);
   check(playingRequest.Speakers.Num() > playingRequest.GetSpeakerIndex());
   check(playingRequest.AuthorityStartTime != 0);

   UAkComponent* voComponent = playingRequest.GetSpeakerVOComponent();
   if (!IsValid(voComponent))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("PlayGrunt run on actor without a valid voComponent. This should not happen."));
      return;
   }

   UE_LOG(LogOSEVoiceController, Verbose, TEXT("Playing Grunt (%s) %s"), GetOwner()->HasAuthority() ? TEXT("Authority") : TEXT("Client"), *playingRequest.AudioEvent->GetName());

   if (!IsNetMode(NM_DedicatedServer))
   {
      //Post the new external source from the selected voice line.
      playingRequest.AkPlayingID = voComponent->PostAkEvent(playingRequest.AudioEvent);
   }

   IOSEVoiceOverEventHandlerInterface::Execute_OnVoiceOverLineStarted(voComponent, playingRequest, 0);
}

void UOSEVoiceOverControllerComponent::_HaltPlayingRequest(FOSEVoiceOverPlayingRequest& playingRequest)
{
   bool shouldNotify = true;
   if (!IsNetMode(NM_DedicatedServer))
   {
      if (playingRequest.AkPlayingID != 0)
      {
         FAkAudioDevice::Get()->StopPlayingID(playingRequest.AkPlayingID);
         playingRequest.AkPlayingID = 0;
      }
      else
      {
         //This request was already interrupted by a client side prediction.
         shouldNotify = false;
      }
   }

   if (shouldNotify)
   {
      UE_LOG(LogOSEVoiceController, Verbose, TEXT("Interrupting %s"), *playingRequest.AudioEvent->GetName());

      UAkComponent* voComponent = VOHelpers::GetVOComponent(playingRequest.GetSpeakerActor());
      if (IsValid(voComponent))
      {
         IOSEVoiceOverEventHandlerInterface::Execute_OnVoiceOverLineInterrupted(voComponent, playingRequest);
      }

      playingRequest.NotifySpeakersInterrupted();
   }
}

void UOSEVoiceOverControllerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(UOSEVoiceOverControllerComponent, _playingVO);
}

float UOSEVoiceOverControllerComponent::_GetExpirationTime(float timeInQueue) const
{
   if (timeInQueue < 0.0f)
      return TNumericLimits<float>::Max();

   check(GetOwner()->HasAuthority());
   
   return GetWorld()->GetTimeSeconds() + timeInQueue;
}

void UOSEVoiceOverControllerComponent::AuthorityRequestVOLine(AActor* actor, UOSEVoiceOverLine* line, FGameplayTag priorityTag, bool interrupting, float timeInQueue)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to request VO without authority! This request was ignored."));
      return;
   }

   if (!IsValid(line))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to request VO with no OSEVoiceOverLine. This request was ignored."));
      return;
   }

   if (!VOHelpers::IsValidRequestActor(actor))
   {
      return;
   }
   
   UAkComponent* voComponent = VOHelpers::GetVOComponent(actor);
   check(voComponent); //This should never hit because IsValidRequestActor should prevent it.

   //Look up the container ahead of time to make sure it has lines.
   FOSEVoiceOverLineIdentityData* identityData = line->GetIdentityData(voComponent);
   if (!identityData)
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to play voice line %s which doesn't contain data for actor %s"), *line->GetName(), *actor->GetName());
      return;
   }
   
   FOSEVoiceOverRequest request;
   request.VoiceLine = line;
   request.VOActor = actor;
   if (!_GetPriorityForTag(priorityTag, &request.Priority))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("VO Priority tag '%s' (for VO Line '%s' on actor '%s') was not found in the priority table (VoiceOverPriority is '%s')"),
         *priorityTag.ToString(),
         *line->GetName(),
         *actor->GetName(),
         _voiceOverPriority ? *_voiceOverPriority->GetName() : TEXT("<null>"));
   }
   request.Interrupting = interrupting;
   request.AudibilityRadius = _GetAudibilityRadius(identityData->AudioEvent);
   request.ExpirationTime = _GetExpirationTime(timeInQueue);

   _pendingRequests.Add(request);
}

void UOSEVoiceOverControllerComponent::RequestGrunt(AActor* actor, UOSEVoiceOverLine* line, FGameplayTag priorityTag, bool interrupting)
{
   if (!IsValid(line))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to request VO Grunt with no OSEVoiceOverLine. This request was ignored."));
      return;
   }

   if (!VOHelpers::IsValidRequestActor(actor))
   {
      return;
   }

   AActor* owner = GetOwner();

   AOSEGameState* gameState = CastChecked<AOSEGameState>(GetWorld()->GetGameState());

   FOSEVoiceOverPlayingRequest playingRequest;

   if (!_GetPriorityForTag(priorityTag, &playingRequest.Priority))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("VO Priority tag '%s' (for Grunt VO Line '%s' on actor '%s') was not found in the priority table (VoiceOverPriority is '%s')"),
         *priorityTag.ToString(),
         *line->GetName(),
         *actor->GetName(),
         _voiceOverPriority ? *_voiceOverPriority->GetName() : TEXT("<null>"));
   }

   playingRequest.Interrupting = interrupting;
   playingRequest.AuthorityStartTime = gameState->GetServerWorldTimeSeconds();
   playingRequest.Speakers.Add(actor);

   //Check to see if we can play and capture which playing VO needs interrupting
   auto interruptedPlayingVOIt = _playingVO.CreateIterator();
   bool interruptPlaying = false;
   
   for (; interruptedPlayingVOIt; ++interruptedPlayingVOIt)
   {
      if (interruptedPlayingVOIt->Speakers.Contains(actor))
      {
         if (interrupting && (!interruptedPlayingVOIt->Interrupting || interruptedPlayingVOIt->Priority < playingRequest.Priority))
         {
            interruptPlaying = true;
            break;
         }

         return; //Early out since we can't interrupt this already playing VO.
      }
   }

   //Check to see if we can play and capture which playing grunt needs interrupting
   auto interruptedPlayingGruntIt = _playingGrunts.CreateIterator();
   bool interruptGrunt = false;
   for (; interruptedPlayingGruntIt; ++interruptedPlayingGruntIt)
   {
      if (interruptedPlayingGruntIt->Speakers.Contains(actor))
      {
         if (interrupting && (!interruptedPlayingGruntIt->Interrupting || interruptedPlayingGruntIt->Priority < playingRequest.Priority))
         {
            interruptGrunt = true;
            break;
         }

         return; //Early out since we can't interrupt this already playing grunt.
      }
   }


   UAkComponent* voComponent = VOHelpers::GetVOComponent(actor);
   check(voComponent); //This should never hit because IsValidRequestActor should prevent it.

   if (!_SetPlayingVoiceLine(playingRequest, line, voComponent, actor))
   {
      return;
   }

   //Calculate the audibility here so we can do interruptions.
   _UpdatePlayingAudibility(playingRequest, gameState->GetOSEPlayerStates());

   if (interruptPlaying)
   {
      _HaltPlayingRequest(*interruptedPlayingVOIt);
      if (gameState->HasAuthority())
      {
         //On the server we remove the playing vo, on the client we wait until the replication from the server removes it.
         interruptedPlayingVOIt.RemoveCurrent();
      }
   }

   if (interruptGrunt)
   {
      _HaltPlayingRequest(*interruptedPlayingGruntIt);
      interruptedPlayingGruntIt.RemoveCurrent();
   }

   playingRequest.NotifySpeakersStarted();

   _PlayGrunt(playingRequest);

   _playingGrunts.Add(playingRequest);
}

void UOSEVoiceOverControllerComponent::AuthorityRequestVOConversation(AActor* actor, UOSEVoiceOverConversation* conversation, FGameplayTag priorityTag, bool interrupting, TArray<AActor*> forcedParticipants, float timeInQueue)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to request VO without authority! This request was ignored."));
      return;
   }

   if (!IsValid(conversation))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to request VO with no OSEVoiceOverConversation. This request was ignored."));
      return;
   }

   if (!VOHelpers::IsValidRequestActor(actor))
   {
      return;
   }

   VOHelpers::FilterValidForcedParticipants(actor, forcedParticipants);

   UAkComponent* voComponent = VOHelpers::GetVOComponent(actor);
   check(voComponent); //This should never hit because IsValidRequestActor should prevent it.

   if (conversation->ConversationRoots.Num() == 0)
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("OSEVoiceOverConversation has no root nodes. This request was ignored."), *actor->GetName());
      return;
   }

   // We need an AudioEvent so the controller can gauge audbility with it's radius. We pull the first root conversation node and it's AudioEvent for use.
   // This has a subtle requirement that the root conversation node must always be usable on the requesting VO actor.
   // This ultimately needs a better solutioon but that's for a later change.
   UOSEVoiceOverConversationNode* rootNode = conversation->ConversationRoots[0];
   if (!IsValid(rootNode->Line))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Root node has no OSEVoiceOverLine. This request was ignored."), *actor->GetName());
   }

   FOSEVoiceOverLineIdentityData* identityData = rootNode->Line->GetIdentityData(voComponent);
   if (!identityData)
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to play voice line %s which doesn't contain data for actor %s"), *rootNode->Line->GetName(), *actor->GetName());
      return;
   }

   FOSEVoiceOverRequest request;
   request.Conversation = conversation;
   request.VOActor = actor;
   if (!_GetPriorityForTag(priorityTag, &request.Priority))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("VO Priority tag '%s' (for VO Conversation '%s' on actor '%s') was not found in the priority table (VoiceOverPriority is '%s')"),
         *priorityTag.ToString(),
         *conversation->GetName(),
         *actor->GetName(),
         _voiceOverPriority ? *_voiceOverPriority->GetName() : TEXT("<null>"));
   }
   request.Interrupting = interrupting;
   request.AudibilityRadius = _GetAudibilityRadius(identityData->AudioEvent);
   request.ExpirationTime = _GetExpirationTime(timeInQueue);
   request.ForcedParticipants = forcedParticipants;

   _pendingRequests.Add(request);
}

void UOSEVoiceOverControllerComponent::AuthorityRequestVOBucket(AActor* actor, UOSEVoiceOverBucket* bucket, FGameplayTag priorityTag, bool interrupting, float timeInQueue)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to request VO without authority! This request was ignored."));
      return;
   }

   if (!IsValid(bucket))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to request VO with no OSEVoiceOverBucket. This request was ignored."));
      return;
   }

   if (!VOHelpers::IsValidRequestActor(actor))
   {
      return;
   }

   FOSEVoiceOverRequest request;
   request.Bucket = bucket;
   request.VOActor = actor;
   if (!_GetPriorityForTag(priorityTag, &request.Priority))
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("VO Priority tag '%s' (for VO Bucket '%s' on actor '%s') was not found in the priority table (VoiceOverPriority is '%s')"),
         *priorityTag.ToString(),
         *bucket->GetName(),
         *actor->GetName(),
         _voiceOverPriority ? *_voiceOverPriority->GetName() : TEXT("<null>"));
   }
   request.Interrupting = interrupting;
   request.AudibilityRadius = _GetAudibilityRadius(bucket->GetMostAudibleEvent(actor));
   request.ExpirationTime = _GetExpirationTime(timeInQueue);

   _pendingRequests.Add(request);
}

void UOSEVoiceOverControllerComponent::AuthorityClearVORequests(AActor* actor)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to clear VO requests without authority! This request was ignored."));
      return;
   }

   _pendingRequests.RemoveAll([=](const FOSEVoiceOverRequest& request)
      {
         return request.VOActor == actor;
      });
}

void UOSEVoiceOverControllerComponent::AuthorityHaltPlayingVO(AActor* actor)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogOSEVoiceController, Error, TEXT("Attempted to halt playing VO without authority! This request was ignored."));
      return;
   }

   for (auto playingIt = _playingVO.CreateIterator(); playingIt; ++playingIt)
   {
      if (playingIt->Speakers.Contains(actor))
      {
         _HaltPlayingRequest(*playingIt);
         playingIt.RemoveCurrent();
      }
   }
}

//-------------------------------------
// FOSEVoiceOverPlayingList

int32 FOSEVoiceOverPlayingList::Add(const FOSEVoiceOverPlayingRequest& request)
{
   int32 index = _requests.Add(request);
   MarkItemDirty(_requests[index]);
   return index;
}

void FOSEVoiceOverPlayingList::SetController(UOSEVoiceOverControllerComponent* controller)
{
   check(IsValid(controller));
   _controller = controller;
}

bool FOSEVoiceOverPlayingList::NetDeltaSerialize(FNetDeltaSerializeInfo& deltaParms)
{
   return FFastArraySerializer::FastArrayDeltaSerialize<FOSEVoiceOverPlayingRequest, FOSEVoiceOverPlayingList>(_requests, deltaParms, *this);
}

void FOSEVoiceOverPlayingList::PreReplicatedRemove(const TArrayView<int32>& removedIndices, int32 finalSize)
{
   check(IsValid(_controller));
   AOSEGameState* gameState = CastChecked<AOSEGameState>(_controller->GetOwner());

   float authorityTime = gameState->GetServerWorldTimeSeconds();

   for (int32 removedIdx : removedIndices)
   {
      FOSEVoiceOverPlayingRequest& removedRequest = _requests[removedIdx];

      float authorityElapsed = gameState->GetServerWorldTimeSeconds() - removedRequest.AuthorityStartTime;
      float authorityTimeOffset = removedRequest.Duration - authorityElapsed;
      float remainingTime = authorityTimeOffset > AuthorityTimeOffsetTolerance ? authorityTimeOffset : 0;

      if (remainingTime > 0)
      {
         //Accounting for some tolerance in latency, the VO line is has been interrupted by being removed early.
         _controller->_HaltPlayingRequest(removedRequest);
      }
      else
      {
         UAkComponent* voComponent = removedRequest.GetSpeakerVOComponent();
         if (IsValid(voComponent))
         {
            IOSEVoiceOverEventHandlerInterface::Execute_OnVoiceOverLineFinished(voComponent, removedRequest);
         }

         removedRequest.NotifySpeakersFinished();
         UE_LOG(LogOSEVoiceController, Verbose, TEXT("Finished %s"), *removedRequest.AudioEvent->GetName());
      }
   }
}

void FOSEVoiceOverPlayingList::PostReplicatedAdd(const TArrayView<int32>& addedIndices, int32 finalSize)
{
   check(IsValid(_controller));
   AOSEGameState* gameState = CastChecked<AOSEGameState>(_controller->GetOwner());

   for (int32 addedIdx : addedIndices)
   {
      FOSEVoiceOverPlayingRequest& newRequest = _requests[addedIdx];

      if (newRequest.AkPlayingID == 0
         && newRequest.Speakers.Num()
         && IsValid(newRequest.Speakers[newRequest.GetSpeakerIndex()])
         && IsValid(newRequest.AudioEvent)
         && newRequest.AuthorityStartTime != 0)
      {
         //Interrupt any grunts playing locally. We don't check priority here since if this replication is happening that doesn't respect the priorities,
         //we favor the authority's decision.
         for (auto gruntIt = _controller->_playingGrunts.CreateIterator(); gruntIt; ++gruntIt)
         {
            check(gruntIt->Speakers.Num() > 0);
            if (newRequest.Speakers.Contains(gruntIt->Speakers[0]))
            {
               //This grunt needs interrupting
               _controller->_HaltPlayingRequest(*gruntIt);
               gruntIt.RemoveCurrent();
            }
         }

         newRequest.NotifySpeakersStarted();
         _controller->_PlayRequest(newRequest, gameState);
      }
      else
      {
         UE_LOG(LogOSEVoiceController, Warning, TEXT("Missed Replicated Play %s"), IsValid(newRequest.AudioEvent) ? *newRequest.AudioEvent->GetName() : TEXT("Missing Event"))
      }
   }
}

void FOSEVoiceOverPlayingList::PostReplicatedChange(const TArrayView<int32>& changedIndices, int32 finalSize)
{
   check(IsValid(_controller));
   AOSEGameState* gameState = CastChecked<AOSEGameState>(_controller->GetOwner());

   for (int32 changedIdx : changedIndices)
   {
      FOSEVoiceOverPlayingRequest& changedRequest = _requests[changedIdx];

      if (!IsValid(changedRequest.CurrentNode))
      {
         UE_LOG(LogOSEVoiceController, Warning, TEXT("Received Strange VO Change Update with No Conversation, This Shouldn't Happen"));
         continue;
      }

      changedRequest.AkPlayingID = 0;

      if (changedRequest.AkPlayingID == 0
         && changedRequest.Speakers.Num()
         && IsValid(changedRequest.Speakers[changedRequest.GetSpeakerIndex()])
         && IsValid(changedRequest.AudioEvent)
         && changedRequest.AuthorityStartTime != 0)
      {
         _controller->_PlayRequest(changedRequest, gameState);
      }
      else
      {
         UE_LOG(LogOSEVoiceController, Warning, TEXT("Missed Replicated Conversation Play %s"), IsValid(changedRequest.AudioEvent) ? *changedRequest.AudioEvent->GetName() : TEXT("Missing Event"))
      }
   }
}

void FOSEVoiceOverPlayingList::FIterator::RemoveCurrent()
{
   TArray<FOSEVoiceOverPlayingRequest>::TIterator::RemoveCurrent();
   _list.MarkArrayDirty();
}
