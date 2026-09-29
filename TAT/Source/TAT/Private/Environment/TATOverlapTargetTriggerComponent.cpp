// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATOverlapTargetTriggerComponent.h"

// ue5
#include "AI/UnifiedStealthSystem/TATStealthScoreInterface.h"

#include "GameFramework/Actor.h"
#include "GameplayTagAssetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATOverlapTargetTriggerComponent)

// Sets default values for this component's properties
UTATOverlapTargetTriggerComponent::UTATOverlapTargetTriggerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   PrimaryComponentTick.TickInterval = 0.25f;

	// ...
}


void UTATOverlapTargetTriggerComponent::StartTracking()
{
   check(_candidateActors.Num() == 0);
   AActor* owner = GetOwner();
   owner->OnActorBeginOverlap.AddUniqueDynamic(this, &UTATOverlapTargetTriggerComponent::_OnActorBeginOverlap);
   owner->OnActorEndOverlap.AddUniqueDynamic(this, &UTATOverlapTargetTriggerComponent::_OnActorEndOverlap);

   TSet<AActor*> overlappedActors;
   owner->GetOverlappingActors(overlappedActors, _candidateRequiredClass);

   for (AActor* overlappedActor : overlappedActors)
   {
      _TryAddCandidate(overlappedActor);
   }
}

void UTATOverlapTargetTriggerComponent::StopTracking()
{

   AActor* owner = GetOwner();
   if(owner)
   {
      owner->OnActorBeginOverlap.RemoveAll(this);
      owner->OnActorEndOverlap.RemoveAll(this);
   }

   _candidateActors.Reset();
   _targetActors.Reset();

   SetComponentTickEnabled(false);
}

bool UTATOverlapTargetTriggerComponent::HasValidTargets() const
{
   return _targetActors.Num() > 0;
}

TArrayView<const TWeakObjectPtr<AActor>> UTATOverlapTargetTriggerComponent::GetCurrentTargets() const
{
   return MakeArrayView(_targetActors.GetData(), _targetActors.Num());
}

TArrayView<const TWeakObjectPtr<AActor>> UTATOverlapTargetTriggerComponent::GetCurrentCandidates() const
{
   return MakeArrayView(_candidateActors.GetData(), _candidateActors.Num());
}

// Called when the game starts
void UTATOverlapTargetTriggerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UTATOverlapTargetTriggerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


   const bool hadTargets = _targetActors.Num() > 0;

	_candidateActors.RemoveSwap(nullptr);

   for (int32 i = 0; i < _targetActors.Num(); i++)
   {
      AActor* target = _targetActors[i].Get();
      if (!_IsValidTarget(target))
      {
         _targetActors.RemoveAtSwap(i);
         i--;

         if (target)
         {
            OnTargetLost.Broadcast(target);
         }
      }
   }

   for (TWeakObjectPtr<AActor> candidate : _candidateActors)
   {
      if(_targetActors.Contains(candidate)) continue;

      if (_IsValidTarget(candidate.Get()))
      {
         _targetActors.Add(candidate);
      }
   }


   SetComponentTickEnabled(_candidateActors.Num() > 0);

   if (!hadTargets && _targetActors.Num() > 0)
   {
      OnTargetFound.Broadcast(ETATOverlapTargetTriggerReason::StateChange);
   }
}

void UTATOverlapTargetTriggerComponent::_OnActorBeginOverlap(AActor* overlappedActor, AActor* otherActor)
{
   _TryAddCandidate(otherActor);
}

void UTATOverlapTargetTriggerComponent::_OnActorEndOverlap(AActor* overlappedActor, AActor* otherActor)
{
   _candidateActors.RemoveSingleSwap(otherActor);
   int32 numTargetsRemoved = _targetActors.RemoveSingleSwap(otherActor);

   if (numTargetsRemoved > 0)
   {
      OnTargetLost.Broadcast(otherActor);
   }

   SetComponentTickEnabled(_candidateActors.Num() > 0);
}

void UTATOverlapTargetTriggerComponent::_TryAddCandidate(AActor* actor)
{
   IGameplayTagAssetInterface* tagSource = Cast<IGameplayTagAssetInterface>(actor);
   if (tagSource == nullptr)
   {
      return;
   }

   if (_candidateRequiredClass != nullptr && !actor->IsA(_candidateRequiredClass))
   {
      return;
   }

   if (!tagSource->HasAllMatchingGameplayTags(_candidateRequiredTags) ||
        tagSource->HasAnyMatchingGameplayTags(_candidateBlockedTags))
   {
      return;
   }

   _candidateActors.AddUnique(actor);

   if (_IsValidTarget(actor))
   {
      _targetActors.Add(actor);

      OnTargetFound.Broadcast(ETATOverlapTargetTriggerReason::Entered);
   }

   SetComponentTickEnabled(true);
}

bool UTATOverlapTargetTriggerComponent::_IsValidTarget(AActor* candidate) const
{
   if(candidate == nullptr) return false;

   IGameplayTagAssetInterface* tagSource = CastChecked<IGameplayTagAssetInterface>(candidate);
   const bool tagsMatch = tagSource->HasAllMatchingGameplayTags(_targetRequiredTags) &&
         !tagSource->HasAnyMatchingGameplayTags(_targetBlockedTags);
   if(!tagsMatch)
   {
      return false;
   }

   if(_filterByStealthScore)
   {
      if(const ITATStealthScoreInterface* stealthScoreInterface = Cast<ITATStealthScoreInterface>(candidate))
      {
         if(stealthScoreInterface->GetStealthScore() >= _maximumStealthScore.GetValue())
         {
            return false;
         }
      }
      else
      {
         return false;
      }
   }

   return true;
}


