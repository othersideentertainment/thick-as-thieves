// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/Electrical/TATPowerSourceVisComponent.h"

// tat
#include "Interactables/Electrical/TATPowerSource.h"

// ue
#include "EngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPowerSourceVisComponent)

UTATPowerSourceVisComponent::UTATPowerSourceVisComponent()
{
   bIsEditorOnly = true;
   bEditableWhenInherited = false;
#if WITH_EDITORONLY_DATA
   SetIsVisualizationComponent(true);
#endif
}

void UTATPowerSourceVisComponent::OnRegister()
{
   Super::OnRegister();
   IsDirty = true;
}

const ATATPowerSource* UTATPowerSourceVisComponent::GetPowerSource() const
{
   return Cast<ATATPowerSource>(GetOwner());
}

void UTATPowerSourceVisComponent::TryRefreshChildPowerSources() const
{
   if (!IsDirty)
   {
      return;
   }

#if WITH_EDITORONLY_DATA
   if (GetWorld()->IsEditorWorld() && !GetWorld()->IsGameWorld())
   {
      _childPowerSources.Reset();
      if (const ATATPowerSource* owner = GetPowerSource())
      {
         // Gather child power sources for component visualizer
         for (TActorIterator<ATATPowerSource> it(GetWorld(), ATATPowerSource::StaticClass()); it; ++it)
         {
            ATATPowerSource* otherPowerSource = *it;
            if (otherPowerSource == owner)
            {
               continue;
            }

            if (otherPowerSource->GetParentPowerSource() == owner)
            {
               _childPowerSources.Add(otherPowerSource);
            }
         }
      }
      IsDirty = false;
   }
#endif // WITH_EDITORONLY_DATA
}
