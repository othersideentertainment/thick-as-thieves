// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/WorldActors/TATToolWorldActorFillBoundingBoxComponent.h"

// tat
#include "Tools/TATToolFunctionLibrary.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"

// ue5
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActorFillBoundingBoxComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATToolWorldActorFillBoundingBoxComponent, Log, All)

UTATToolWorldActorFillBoundingBoxComponent::UTATToolWorldActorFillBoundingBoxComponent()
{
   PrimaryComponentTick.bCanEverTick = false;

   SetIsReplicatedByDefault(true);
}

void UTATToolWorldActorFillBoundingBoxComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(UTATToolWorldActorFillBoundingBoxComponent, _worldActorAdjustedTransform, COND_InitialOnly);
}

void UTATToolWorldActorFillBoundingBoxComponent::AuthorityFindAdjustedPlacement(const FTATWorldActorBoxFillExtentConstraints& extentConstraints)
{
   UTATToolFunctionLibrary::TryAdjustWorldActorBoxFillPlacement(GetOwner(), extentConstraints, _worldActorAdjustedTransform);

   _authorityHasUpdatedWorldActorAdjustedTransform = true;

   ensureMsgf(!_authorityHasReplicatedAtLeastOnce,
      TEXT("UTATToolWorldActorFillBoundingBoxComponent on '%s' is calling AuthorityFindAdjustedPlacement after replicating"),
      *GetOwner()->GetName());
}

void UTATToolWorldActorFillBoundingBoxComponent::PreReplication(IRepChangedPropertyTracker& changedPropertyTracker)
{
   Super::PreReplication(changedPropertyTracker);

   ensureMsgf(_authorityHasUpdatedWorldActorAdjustedTransform,
      TEXT("UTATToolWorldActorFillBoundingBoxComponent on '%s' is replicating without having AuthorityFindAdjustedPlacement called"),
      *GetOwner()->GetName());

   _authorityHasReplicatedAtLeastOnce = true;
}

void UTATToolWorldActorFillBoundingBoxComponent::AdjustPlacement()
{
   // Sanity-check that the value has actually changed from the default
   if (_worldActorAdjustedTransform != FTATWorldActorBoxFillAdjustedTransform())
   {
      OnAdjustedPlacement.Broadcast(_worldActorAdjustedTransform);
   }
   else
   {
      UE_LOG(LogTATToolWorldActorFillBoundingBoxComponent, Error,
         TEXT("UTATToolWorldActorFillBoundingBoxComponent on '%s' did not get its adjusted transform: do you need to call AuthorityFindAdjustedPlacement on the server?"),
         *GetOwner()->GetName());
   }
}
