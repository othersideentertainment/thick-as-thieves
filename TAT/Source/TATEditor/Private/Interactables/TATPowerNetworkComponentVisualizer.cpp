// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATPowerNetworkComponentVisualizer.h"

// tat
#include "Interactables/Electrical/TATPowerNetworkComponent.h"
#include "Interactables/Electrical/TATPowerNetworkUtils.h"
#include "Interactables/Electrical/TATPowerNetworkInterface.h"
#include "Interactables/Electrical/TATPowerNetworkSubsystem.h"
#include "Variation/ComponentVisualizerTextHelper.h"

// ue
#include "Selection.h"

namespace TATPowerNetworkUtils
{
   class FPowerNetworkEditorVis : public FPowerNetworkDebugDraw
   {
      FPrimitiveDrawInterface* _pdi = nullptr;

   public:
      explicit FPowerNetworkEditorVis(FPrimitiveDrawInterface* pdi)
         : _pdi(pdi)
      {
      }

      virtual void DrawLine(const FVector& start, const FVector& end, const FLinearColor& color, float thickness) override
      {
         check(_pdi != nullptr);
         _pdi->DrawLine(start, end, color, SDPG_Foreground, thickness);
      }
   };
}

// static
void FTATPowerNetworkComponentVisualizer::_DrawPowerNetworkRecursive(FPrimitiveDrawInterface* pdi, AActor* actor, AActor* selectedActor, TSet<AActor*>& visitedActors)
{
   check(actor != nullptr);
   check(!visitedActors.Contains(actor));

   visitedActors.Add(actor);

   UTATPowerNetworkComponent* powerComponent = UTATPowerNetworkComponent::GetPowerNetworkComponent(actor);
   if (powerComponent == nullptr)
   {
      return;
   }

   auto shouldDrawActor = [&visitedActors](AActor* powerActor)
   {
      return powerActor != nullptr && !visitedActors.Contains(powerActor);
   };

   static constexpr FLinearColor connectionColorForward = FLinearColor(0.9f, 0.9f, 0.75f);
   static constexpr FLinearColor connectionColorReverse = FLinearColor(1.0f, 0.2f, 0.2f);

   const bool drawActorBounds = powerComponent->PowerNetworkRole != ETATPowerNetworkRole::Connector;
   const bool drawSplineComponent = powerComponent->PowerNetworkRole == ETATPowerNetworkRole::Connector;
   static const TATPowerNetworkUtils::FDashedLine splineDashed{ true };
   FLinearColor debugVisColor = TATPowerNetworkUtils::GetColorForRole(powerComponent->PowerNetworkRole);
   float colorIntensity = 0.55f;
   float lineThickness = 0.0f;

   if (actor == selectedActor)
   {
      colorIntensity = 1.0f;
      lineThickness = 1.0f;
   }
   else if (_IsObjectSelected(actor))
   {
      colorIntensity = 0.85f;
      lineThickness = 0.5f;
   }

   TATPowerNetworkUtils::FPowerNetworkEditorVis visualizer{ pdi };

   if (drawActorBounds)
   {
      FVector origin, extent;
      TATPowerNetworkUtils::GetActorBoundsInLocalSpace(actor, origin, extent);
      visualizer.DrawBox(origin, extent, TATPowerNetworkUtils::ApplyColorIntensity(debugVisColor, colorIntensity), lineThickness, actor->GetActorTransform());
   }

   if (drawSplineComponent)
   {
      visualizer.DrawActorSplineComponent(actor, TATPowerNetworkUtils::ApplyColorIntensity(debugVisColor, colorIntensity), lineThickness, splineDashed);
   }

   for (AActor* connectedActor : powerComponent->GetLinkSlotActors())
   {
      if (connectedActor != nullptr)
      {
         visualizer.DrawPowerLink(actor, connectedActor, connectionColorForward, lineThickness);

         if (shouldDrawActor(connectedActor))
         {
            _DrawPowerNetworkRecursive(pdi, connectedActor, selectedActor, visitedActors);
         }
      }
   }

   if (const UTATPowerNetworkSubsystem* powerSubsystem = actor->GetWorld()->GetSubsystem<UTATPowerNetworkSubsystem>())
   {
      for (TWeakObjectPtr<AActor> reverseConnection : powerSubsystem->GetReverseConnections(actor))
      {
         AActor* connectedActor = reverseConnection.Get();
         if (connectedActor == nullptr)
         {
            continue;
         }

         visualizer.DrawPowerLink(actor, connectedActor, connectionColorReverse, lineThickness * 0.5f, FVector::UpVector * 10.0f);

         if (shouldDrawActor(connectedActor))
         {
            _DrawPowerNetworkRecursive(pdi, connectedActor, selectedActor, visitedActors);
         }
      }
   }
}

// static
bool FTATPowerNetworkComponentVisualizer::_IsObjectSelected(const UObject* obj)
{
#if WITH_EDITOR
   check(GEditor != nullptr);
   if (const USelection* editorSelection = GEditor->GetSelectedActors())
   {
      return editorSelection->IsSelected(obj);
   }
#endif
   return false;
}

void FTATPowerNetworkComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   const UTATPowerNetworkComponent* powerComponent = Cast<UTATPowerNetworkComponent>(component);
   if (powerComponent == nullptr)
   {
      return;
   }

   AActor* ownerActor = powerComponent->GetOwner();
   if (ownerActor == nullptr)
   {
      return;
   }

   TSet<AActor*> visitedActors;
   _DrawPowerNetworkRecursive(pdi, ownerActor, ownerActor, visitedActors);
}

void FTATPowerNetworkComponentVisualizer::DrawVisualizationHUD(const UActorComponent* component, const FViewport* viewport, const FSceneView* view, FCanvas* canvas)
{
   const UTATPowerNetworkComponent* powerComponent = Cast<UTATPowerNetworkComponent>(component);
   if (powerComponent == nullptr)
   {
      return;
   }

   AActor* ownerActor = powerComponent->GetOwner();
   if (ownerActor == nullptr)
   {
      return;
   }

   ITATPowerNetworkInterface* ownerActorPower = Cast<ITATPowerNetworkInterface>(ownerActor);
   if (ownerActorPower == nullptr)
   {
      return;
   }

   // For connectors, show the index for each side so it's clear which side will be connected when assigning slot connections
   if (powerComponent->PowerNetworkRole == ETATPowerNetworkRole::Connector)
   {
      static constexpr bool forDebugVis = true;
      const FVector textOffset = FVector::UpVector * 40.0f;

      for (int32 slotIndex = 0; slotIndex < powerComponent->NumLinkSlots(); slotIndex++)
      {
         if (TOptional<FVector> linkLocation = ownerActorPower->GetPowerNetworkWorldLocationForConnectionIndex(slotIndex, forDebugVis))
         {
            if (VisualizerHelper::FTextDrawer drawer = { *linkLocation + textOffset, view, canvas })
            {
               drawer.AddTextItem(FString::Printf(TEXT("Index [%i]"), slotIndex), FColor::Green);
            }
         }
      }
   }
}
