// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/LivingWorld/TATLivingWorldAgentComponent.h"

// tat
#include "AI/LivingWorld/TATLivingWorldAgentInterface.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"

// ose 
#include "AI/OSEAIFunctionLibrary.h"

// ue
#include "AIController.h"
#include "SmartObjectSubsystem.h"

#include "AI/SmartObjects/TATSmartObjectComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLivingWorldAgentComponent)

DEFINE_LOG_CATEGORY(LogTATLivingWorldAgent);

UTATLivingWorldAgentComponent::UTATLivingWorldAgentComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

UTATLivingWorldAgentComponent* UTATLivingWorldAgentComponent::Get(AActor* actor)
{
   if (ITATLivingWorldAgentInterface* livingWorldAgent = Cast<ITATLivingWorldAgentInterface>(actor))
   {
      return livingWorldAgent->GetLivingWorldAgentComponent();
   }

   return nullptr;
}

void UTATLivingWorldAgentComponent::ForceSmartObjects(TConstArrayView<TObjectPtr<AActor>> smartObjectActors)
{
   check(GetOwner()->HasAuthority());
   HasForcedSelection = true;

   USmartObjectSubsystem* smartObjectSubsystem = GetWorld()->GetSubsystem<USmartObjectSubsystem>();
   if (smartObjectSubsystem == nullptr)
   {
      return;
   }

   FSmartObjectRequestFilter filter;
   filter.UserTags = UserTags;
   filter.ActivityRequirements = ActivityTagQuery;

   int totalInteractions = 0;
   TArray<FSmartObjectSlotHandle> slots;
   for (AActor* actor : smartObjectActors)
   {
      if (totalInteractions >= MaxNumberOfInteractions)
         break;

      ITATSmartObjectOwnerInterface* smartObjectActor = Cast<ITATSmartObjectOwnerInterface>(actor);
      if(smartObjectActor == nullptr) continue;

      UTATSmartObjectComponent* smartObject = smartObjectActor->GetSmartObjectComponent();
      if(smartObject == nullptr) continue;


      slots.Reset();
      smartObjectSubsystem->FindSlots(smartObject->GetRegisteredHandle(), filter, slots);
      if (slots.IsEmpty())
      {
         continue;
      }

      FSmartObjectClaimHandle claimHandle = smartObjectSubsystem->MarkSlotAsClaimed(slots[0], ESmartObjectClaimPriority::Normal);
      if (claimHandle.IsValid())
      {
         smartObjectSubsystem->GetSlotEventDelegate(claimHandle.SlotHandle)->AddUObject(this, &ThisClass::OnSlotStateChanged);
         ClaimedSmartObjectHandles.Enqueue(claimHandle);
         HasValidSmartObjectHandles = true;
         ++totalInteractions;
      }
   }
}

bool UTATLivingWorldAgentComponent::HasValidClaimedSmartObjectHandles() const
{
   return HasValidSmartObjectHandles;
}

FSmartObjectClaimHandle UTATLivingWorldAgentComponent::GetNextClaimHandle()
{
   FSmartObjectClaimHandle outHandle = FSmartObjectClaimHandle::InvalidHandle;
   if(ensure(HasValidClaimedSmartObjectHandles()))
   {
      ClaimedSmartObjectHandles.Dequeue(outHandle);
   }
   return outHandle;
}

void UTATLivingWorldAgentComponent::HandleSetupOfLivingWorldComponents()
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_TATLivingWorldAgentComponent_HandleSetupOfLivingWorldComponents);
   if (HasForcedSelection)
   {
      return;
   }

   USmartObjectSubsystem* smartObjectSubsystem = GetWorld()->GetSubsystem<USmartObjectSubsystem>();
   if (smartObjectSubsystem == nullptr)
   {
      return;
   }

   APawn* ownerPawn = GetOwner<APawn>();
   if(ownerPawn == nullptr)
   {
      UE_LOG(LogTATLivingWorldAgent, Error, TEXT("Living world agent component only works on AI Controlled Pawns, it's currently on %s"), *GetNameSafe(GetOwner()));
      return;
   }
   
   AAIController* aiController = ownerPawn->GetController<AAIController>();
   if(ownerPawn == nullptr)
   {
      UE_LOG(LogTATLivingWorldAgent, Error, TEXT("Living world agent component only works on AI Controllers, it's currently on a pawn controleld by a %s"), *GetNameSafe(ownerPawn->GetController()));
      return;
   }
   
   FSmartObjectRequest civilianRequests;
   civilianRequests.Filter.UserTags = UserTags;
   civilianRequests.Filter.ActivityRequirements = ActivityTagQuery;

   
   const FVector userLocation = GetOwner()->GetActorLocation();
   civilianRequests.QueryBox = FBox(userLocation, userLocation).ExpandBy(FVector(QuerySize), FVector(QuerySize));
   int totalInteractions = 0;

   //TODO: Spread the cost of this over multiple frames if the cost is too large. Keep an eye on the performance checks.
   //This only happens once at start per civilian NPC with this component and the search area is relatively small, so I'm
   //deferring the timeslicing for now.
   for(int i = 0; i < MaxNumberOfInteractions; ++i)
   {
      if(totalInteractions >= MaxNumberOfInteractions)
         break;
      TArray<FSmartObjectRequestResult> civilianResults;
      if(smartObjectSubsystem->FindSmartObjects(civilianRequests, civilianResults))
      {
         for (FSmartObjectRequestResult& civilianResult : civilianResults)
         {
            UTATSmartObjectComponent* smartObjectComponent = Cast<UTATSmartObjectComponent>(
               smartObjectSubsystem->GetSmartObjectComponentByRequestResult(civilianResult));
            if(smartObjectComponent == nullptr)
               continue;

            TOptional<FVector> targetLocation = smartObjectSubsystem->GetSlotLocation(civilianResult);
            if(targetLocation.IsSet() == false)
               continue;

            constexpr bool allowPartial = false;
            if(UOSEAIFunctionLibrary::HasPathToLocation(aiController, targetLocation.GetValue(), allowPartial) == false)
               continue;

            FSmartObjectClaimHandle handle = smartObjectSubsystem->MarkSlotAsClaimed(civilianResult.SlotHandle, ESmartObjectClaimPriority::Normal);
            if(handle.IsValid())
            {
               smartObjectSubsystem->GetSlotEventDelegate(handle.SlotHandle)->AddUObject(this, &ThisClass::OnSlotStateChanged);
               // Claim the smart object, then register for slot change events.
               ClaimedSmartObjectHandles.Enqueue(handle);
               ++totalInteractions;
               HasValidSmartObjectHandles = true;
               auto slotTransform = smartObjectSubsystem->GetSlotTransform(civilianResult);
               civilianRequests.QueryBox = FBox(slotTransform->GetLocation(), slotTransform->GetLocation()).ExpandBy(FVector(QuerySize), FVector(QuerySize));
            }
            break;
         }
      }
   }
}

void UTATLivingWorldAgentComponent::BeginPlay()
{
   Super::BeginPlay();
   if(GetOwner()->HasAuthority() && !HasForcedSelection)
   {
      HasValidSmartObjectHandles = false;
      // All of the smart objects are registered on the world begin play (though we may have issues with streaming levels?)
      // So in order for the NPC to be able to find the smart objects, register them the frame _after_ begin play is called.
      // TODO: I want to find a cleaner way to do this - I'm not really happy with this as it feels hacky. 
      // NOTE: This is now potentially clobbered by forced selection (it is hard to do it earlier for BP-added components)
      GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleSetupOfLivingWorldComponents);
   }
}

void UTATLivingWorldAgentComponent::OnSlotStateChanged(const FSmartObjectEventData& event)
{
   if(event.Reason == ESmartObjectChangeReason::OnReleased)
   {
      USmartObjectSubsystem* smartObjectSubsystem = GetWorld()->GetSubsystem<USmartObjectSubsystem>();
      if (smartObjectSubsystem == nullptr)
      {
         return;
      }
      // if the slot is released, then reclaim it and add it back to the queue of handles to use
      ClaimedSmartObjectHandles.Enqueue(smartObjectSubsystem->MarkSlotAsClaimed(event.SlotHandle, ESmartObjectClaimPriority::Normal));
   }
}
