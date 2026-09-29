// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueHighlightComponent.h"

// tat
#include "Variation/Clues/TATLocalClueFactSubsystem.h"
#include "Variation/SceneVariants/TATSceneVariantUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueHighlightComponent)

UTATClueHighlightComponent::UTATClueHighlightComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

void UTATClueHighlightComponent::BeginPlay()
{
   Super::BeginPlay();

   if(IsNetMode(NM_DedicatedServer))
   {
      return;
   }

   if(!UTATSceneVariantUtils::ResolveBoolRequirement(GetWorld(), _highlightSceneRequirement))
   {
      return;
   }

   if(UTATLocalClueFactSubsystem* factSubsystem = GetWorld()->GetSubsystem<UTATLocalClueFactSubsystem>())
   {
      // NOTE: explicitly not unregistering in EndPlay, because each callback is only called once, so the cost of registering
      //       would dominate any savings
      factSubsystem->CallOrRegisterFactDelegate(_clueFactTag, FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::_OnFactKnown));
   }
}

bool UTATClueHighlightComponent::NeedsLoadForServer() const
{
   return false;
}

void UTATClueHighlightComponent::_OnFactKnown()
{
   OnClueFactLearned.Broadcast();
}


