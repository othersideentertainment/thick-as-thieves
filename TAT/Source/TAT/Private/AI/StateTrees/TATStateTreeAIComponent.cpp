// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/TATStateTreeAIComponent.h"

// ue
#include "AIController.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeAIComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATStateTreeAIComponent, Log, All);

UTATStateTreeAIComponent::UTATStateTreeAIComponent(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   // Don't start logic immediately - we want to wait for the controller to possess the pawn so
   // we can have access to the pawn from the schema.
   bStartLogicAutomatically = false;
}

void UTATStateTreeAIComponent::BeginPlay()
{
   Super::BeginPlay();

   if (!ensureAlwaysMsgf(AIOwner != nullptr, TEXT("%s found no AIOwner - logic will never be able to start."), *GetName()))
   {
      return;
   }

   if (!ensureAlwaysMsgf(!IsRunning(), TEXT("%s was already found running - most likely 'bStartLogicAutomatically' was enabled. "
         "Pawn will not be available."), *GetName()))
   {
      return;
   }

   if (_shouldStartStateTreeIfPawnSet)
   {
      if (AIOwner->GetPawn() == nullptr)
      {
         AIOwner->OnPossessedPawnChanged.AddUniqueDynamic(this, &UTATStateTreeAIComponent::_OnPawnChanged);
      }
      else
      {
         StartLogic();
      }
   }
}

void UTATStateTreeAIComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (AIOwner != nullptr)
   {
      AIOwner->OnPossessedPawnChanged.RemoveAll(this);
   }

   Super::EndPlay(endPlayReason);
}

bool UTATStateTreeAIComponent::SetContextRequirements(FStateTreeExecutionContext& context, bool bLogErrors)
{
   context.SetLinkedStateTreeOverrides(&LinkedStateTreeOverrides);
   return Super::SetContextRequirements(context, bLogErrors);
}

void UTATStateTreeAIComponent::_OnPawnChanged(APawn* oldPawn, APawn* newPawn)
{
   // _OnPawnChanged is only bound when the current pawn is null
   // and we unregister as soon as this is called once, so oldPawn
   // should be null and newPawn should be non-null
   check(newPawn != nullptr);
   
   // Controller ('AIOwner') shouldn't be null if our pawn has just changed.
   check(AIOwner != nullptr);
   AIOwner->OnPossessedPawnChanged.RemoveAll(this);

   StartLogic();
}
