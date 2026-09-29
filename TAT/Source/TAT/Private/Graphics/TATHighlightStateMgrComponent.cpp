// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/TATHighlightStateMgrComponent.h"

// tat
#include "Interactables/TATInteractHighlightUtils.h"
#include "Player/TATCharacter.h"
#include "Player/TATPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHighlightStateMgrComponent)

// ue4

DEFINE_LOG_CATEGORY_STATIC(LogTATHighlightStateMgr, Log, All);

UTATHighlightStateMgrComponent::UTATHighlightStateMgrComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   SetIsReplicatedByDefault(false);
}

// static
void UTATHighlightStateMgrComponent::HighlightComponent(UPrimitiveComponent* const component, const bool isHighlighted)
{
   check(component);
   component->SetCustomDepthStencilValue(1);
   component->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_1);
   component->SetRenderCustomDepth(isHighlighted);
}

// static
void UTATHighlightStateMgrComponent::HighlightMeshesWithTag(AActor* actor, FName tag, bool isHighlighted)
{
   check(actor);
   TInlineComponentArray<UPrimitiveComponent*> primitives;
   actor->GetComponents(primitives);
   for (auto component : primitives)
   {
      if (component->ComponentHasTag(tag))
      {
         HighlightComponent(component, isHighlighted);
      }
   }
}

// static
UTATHighlightStateMgrComponent* UTATHighlightStateMgrComponent::FindOrAdd(AActor* actor)
{
   if (!actor)
      return nullptr;
 
   UTATHighlightStateMgrComponent* comp = actor->FindComponentByClass<UTATHighlightStateMgrComponent>();

   if (!comp)
   {
      comp = NewObject<UTATHighlightStateMgrComponent>(actor, NAME_None, RF_Transient);
      check(comp);
      comp->RegisterComponent();
   }
   
   return comp;
}

void UTATHighlightStateMgrComponent::RequestActorHighlight(FName requestingSystemName, AActor* actor, bool isHighlighted)
{
   if (UTATHighlightStateMgrComponent* mgr = FindOrAdd(actor))
   {
      mgr->RequestHighlightChange(requestingSystemName, isHighlighted);
   }
}

void UTATHighlightStateMgrComponent::RequestHighlightChange(FName requestingSystemName, bool isHighlighted)
{
   bool& newIsHighlighted = _highlightRequests.FindOrAdd(requestingSystemName);
   newIsHighlighted = isHighlighted;
   _UpdateHighlights();
}

void UTATHighlightStateMgrComponent::_UpdateHighlights()
{
   bool isHighlighted = false;
   for (const auto& entry : _highlightRequests)
   {
      const bool isSystemHighlighted = entry.Value;
      isHighlighted = isSystemHighlighted;
      if (isHighlighted)
      {
         // just need one thing to be true and we'll highlight
         break;
      }
   }

   if (isHighlighted != _wasHighlighted)
   {
      AActor* ownerActor = GetOwner();
      check(ownerActor);

      // TODO: Do we want to come up with a system to highlight different meshes for different systems?  For now we're just going to use interact for everything...
      HighlightMeshesWithTag(ownerActor, UTATInteractHighlightUtils::InteractHighlightTag_NAME, isHighlighted);

      UE_LOG(LogTATHighlightStateMgr, Verbose, TEXT("Actor %s is %s"), *ownerActor->GetName(), isHighlighted ? TEXT("highlighted") : TEXT("unhighlighted"));

      _wasHighlighted = isHighlighted;

      // update the local player
      // TODO: This isn't a real longterm solution, but should hold us over till we do a better highlight impl
      if (ATATPlayerController* localPC = ATATPlayerController::GetLocalTATPlayerController(this))
      {
         if (ATATCharacter* character = Cast<ATATCharacter>(localPC->GetCharacter()))
         {
            if (isHighlighted)
               character->AddObjectHighlight();
            else
               character->RemoveObjectHighlight();
         }
      }
   }
}

