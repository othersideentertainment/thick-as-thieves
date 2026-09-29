// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Breakables/TATBreakableDebugVis.h"

// tat
#include "Breakables/TATBreakableComponent.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue5
#include "Engine/Canvas.h"
#include "GameFramework/HUD.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
namespace BreakableDebugVis
{
   TAutoConsoleVariable<float> CVarBreakableDebugVisRange(
      TEXT("TAT.Breakables.DebugVis.Range"),
      1000.0f,
      TEXT("Range in cm at which breakables are visible using `ShowDebug Breakables` (default 1000)")
   );

   TAutoConsoleVariable<int32> CVarBreakableDebugVisShowNames(
      TEXT("TAT.Breakables.DebugVis.ShowNames"),
      0,
      TEXT("Whether to show the names of breakables")
   );

   TAutoConsoleVariable<int32> CVarBreakableDebugVisIncludeInvulnerable(
      TEXT("TAT.Breakables.DebugVis.IncludeInvulnerable"),
      0,
      TEXT("Whether to include non-breakable breakable components")
   );

   FVector FindInteractCenter(const AActor* actor)
   {
      check(actor);

      FBox box(ForceInit);
      actor->ForEachComponent<UPrimitiveComponent>(false, [&](const UPrimitiveComponent* primComp)
         {
            if (primComp->IsRegistered() && (primComp->ComponentHasTag(UOSEInteractionHelpers::kInteractTag) || primComp->ComponentHasTag(UOSEInteractionHelpers::kInteractTag)))
            {
               box += primComp->Bounds.GetBox();
            }
         });

      return box.IsValid ? box.GetCenter() : actor->GetActorLocation();
   }

   void OnShowDebugInfo(AHUD* hud, UCanvas* canvas, const FDebugDisplayInfo& displayInfo, float& yl, float& yPos)
   {
      static const FName kNameBreakables("Breakables");
      if (canvas == nullptr || !hud->ShouldDisplayDebug(kNameBreakables))
      {
         return;
      }

      FVector viewPos = canvas->SceneView->ViewLocation;

      FString scratch;

      // Add all actors with a breakable component
      // Probably not fast, but this is debug vis
      for (TObjectIterator<UTATBreakableComponent> it(RF_ClassDefaultObject | RF_ArchetypeObject); it; ++it)
      {
         UTATBreakableComponent* breakableComponent = *it;
         if (breakableComponent == nullptr || breakableComponent->GetWorld() != hud->GetWorld())
         {
            continue;
         }

         if (!breakableComponent->IsBreakable() && !CVarBreakableDebugVisIncludeInvulnerable.GetValueOnGameThread())
         {
            continue;
         }

         AActor* owner = breakableComponent->GetOwner();
         if (owner == nullptr) continue;

         FVector origin = FindInteractCenter(owner);

         if (FVector::DistSquared(origin, viewPos) > FMath::Square(CVarBreakableDebugVisRange.GetValueOnGameThread()))
         {
            continue;
         }

         FVector screenPos = canvas->Project(origin, false);
         if (screenPos.Z < 0)
         {
            continue;
         }

         scratch.Reset();
         if (CVarBreakableDebugVisShowNames.GetValueOnGameThread())
         {
            scratch.Append(owner->GetActorNameOrLabel());
            scratch.Append(TEXT("\n"));
         }
         breakableComponent->AppendDebugString(scratch);

         FCanvasTextStringViewItem textItem(FVector2D(screenPos), scratch, GEngine->GetMediumFont(), FColor::Cyan);
         textItem.EnableShadow(FLinearColor::Black);
         textItem.bCentreX = true;
         canvas->DrawItem(textItem);
      }
   }
}
#endif
