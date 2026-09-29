// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATReadableClueActor.h"

// tat
#include "Interactables/TATInteractHighlightUtils.h"
#include "Net/TATIrisGroupSubsystem.h"
#include "Variation/Clues/TATKnownCluesComponent.h"
#include "Variation/Clues/TATLocalClueFactSubsystem.h"
#include "Quests/TATQuestDependentActorHelpers.h"
#include "Player/TATPlayerStatsTags.h"
#include "UI/Queue/TATUIQueue.h"

// wwise
#include "AkAudioEvent.h"

// ose
#include "Player/OSEPlayerStats.h"

// ue
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATReadableClueActor)

UTATUIQueueAction* UTATReadableClueWidget::CreateAction(const TSoftClassPtr<UTATReadableClueWidget>& widgetClass, const FText& clueText)
{
   return UTATScreenQueueAction::Create(widgetClass, [clueText] (UTATScreenWidget* widget)
         { CastChecked<UTATReadableClueWidget>(widget)->SetClueText(clueText); });
}

// Sets default values
ATATReadableClueActor::ATATReadableClueActor()
{
   PrimaryActorTick.bCanEverTick = false;
   bReplicates = true;

   // DormantAll so it doesn't re-replicate when re-entering relevancy
   NetDormancy = DORM_DormantAll;

   // short cull distance of 20m
   SetNetCullDistanceSquared(FMath::Square(20'00));
}

void ATATReadableClueActor::InitClueData(const FTATReadableClueData& clueData)
{
   _clueData = clueData;
}

void ATATReadableClueActor::BeginPlay()
{
   Super::BeginPlay();

   if(IsNetMode(NM_ListenServer) && !TATQuestDependentActorHelpers::IsRelevantToListenServerPlayer(_AuthorityGetSourceTag(), this))
   {
      SetHidden(true);
      SetActorEnableCollision(false);
      // might have to do something else if there is persistent audio or something expensive
   }

   if (!IsNetMode(NM_DedicatedServer) && _clueData.FactTags.Num() > 0)
   {
      if (UTATLocalClueFactSubsystem* localFactSubsystem = GetWorld()->GetSubsystem< UTATLocalClueFactSubsystem>())
      {
         localFactSubsystem->CallOrRegisterNamespacedFactDelegateMulti(_clueData.FactTags, _clueData.FactNamespace, FSimpleDelegate::CreateUObject(this, &ThisClass::_UpdateFactsKnown));
      }
   }
}

void ATATReadableClueActor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);

   if (endPlayReason == EEndPlayReason::Destroyed)
   {
      if (UTATLocalClueFactSubsystem* localFactSubsystem = GetWorld()->GetSubsystem<UTATLocalClueFactSubsystem>())
      {
         localFactSubsystem->UnregisterNamespacedFactDelegateMulti(_clueData.FactTags, _clueData.FactNamespace, this);
      }
   }
}

void ATATReadableClueActor::BeginReplication()
{
   Super::BeginReplication();

   // If for a contract, add to an iris group for that contract, so it is filtered out
   // for players not in that group
   if(HasAuthority() && TATQuestDependentActorHelpers::IsContract(_AuthorityGetSourceTag()))
   {
      if(UTATIrisGroupSubsystem* groupSubsystem = GetWorld()->GetSubsystem<UTATIrisGroupSubsystem>())
      {
         groupSubsystem->ForTag(_AuthorityGetSourceTag()).AddActorToGroup(this);
      }
   }
}

bool ATATReadableClueActor::IsNetRelevantFor(const AActor* realViewer, const AActor* viewTarget, const FVector& srcLocation) const
{
   // NOTE: not calling super here, as it explicitly skips hidden actors without collision, and the listen server may do this
   if(!IsWithinNetRelevancyDistance(srcLocation))
   {
      return false;
   }
   return TATQuestDependentActorHelpers::AuthorityIsRelevantToViewingActor(_AuthorityGetSourceTag(), realViewer);
}

#if WITH_EDITOR
EDataValidationResult ATATReadableClueActor::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   if(!GetClass()->HasAllClassFlags(CLASS_Abstract))
   {
      if(_readableWidget.IsNull())
      {
         context.AddError(INVTEXT("ReadableClueActor has no widget"));
      }
   }

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

void ATATReadableClueActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATReadableClueActor, _clueData, COND_InitialOnly);
   // Want to be able to hide on listen server without replicating it
   DISABLE_REPLICATED_PRIVATE_PROPERTY(AActor, bHidden);
}

bool ATATReadableClueActor::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return interactingCharacter->IsPlayerControlled() && !_clueData.ClueText.IsEmpty();
}

void ATATReadableClueActor::GetInteractPrompt_Implementation(ACharacter* interactingCharacter,
   FInteractPrompt& outPrompt)
{
   outPrompt.PressAction = _readCluePrompt;
}

FInteractStartResult ATATReadableClueActor::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   APlayerController* controller = interactingCharacter->GetController<APlayerController>();
   if(controller && controller->IsLocalController())
   {
      // NOTE: this is slightly overkill, but it avoids a little boilerplate for the async load and widget creation
      UTATUIQueue* queue = UTATUIQueue::Create(controller);
      queue->AddAction(UTATReadableClueWidget::CreateAction(_readableWidget, _clueData.ClueText));
      queue->Run();

      // Sound is "mental", so only play it locally. Convert to cue otherwise
      UAkAudioEvent* soundToPlay = _readByLocalPlayer ? _localAlreadyReadInteractSound : _localInteractSound;
      if(IsValid(soundToPlay))
      {
         soundToPlay->PostAtLocation(GetActorLocation(), FRotator(), GetWorld(), nullptr, nullptr, nullptr, (AkCallbackType)0, nullptr);
      }
      _readByLocalPlayer = true;
   }

   if(HasAuthority())
   {
      UTATKnownCluesComponent::RecordKnownClueFacts(interactingCharacter->GetPlayerState(), _clueData.Facts.Get());
      UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(interactingCharacter, TAG_PlayerStats_Clues_Collected);
   }
   
   return {};
}

void ATATReadableClueActor::ShowHighlight_Implementation(bool showHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, showHighlight);
}

FGameplayTag ATATReadableClueActor::_AuthorityGetSourceTag() const
{
   return _clueData.Facts ? _clueData.Facts->GetSourceTag() : FGameplayTag();
}

void ATATReadableClueActor::_UpdateFactsKnown()
{
   if (_allFactsKnownByLocalPlayer)
   {
      return;
   }

   UTATLocalClueFactSubsystem* localFactSubsystem = GetWorld()->GetSubsystem< UTATLocalClueFactSubsystem>();
   if (!ensure(localFactSubsystem) || !localFactSubsystem->AreAllFactsKnown(_clueData.FactTags, _clueData.FactNamespace))
   {
      return;
   }

   _allFactsKnownByLocalPlayer = true;
   BP_OnFactsKnownByLocalPlayer();
}

