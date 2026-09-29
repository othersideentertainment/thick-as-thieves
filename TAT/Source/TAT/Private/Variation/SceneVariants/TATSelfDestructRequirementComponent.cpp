// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSelfDestructRequirementComponent.h"

// tat
#include "Variation/SceneVariants/TATSceneVariantUtils.h"

// ue5
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSelfDestructRequirementComponent)

UTATSelfDestructRequirementComponent::UTATSelfDestructRequirementComponent()
{
}

void UTATSelfDestructRequirementComponent::BeginPlay()
{
   Super::BeginPlay();

   if (!_requirement.IsNone() && !UTATSceneVariantUtils::ResolveBoolRequirement(GetWorld(), _requirement))
   {
      // NB: destruction waits until end of begin play evaluation, so should be safe
      GetOwner()->Destroy();
   }
}

#if WITH_EDITOR
void UTATSelfDestructRequirementComponent::CheckForErrors()
{
   Super::CheckForErrors();

   FMessageLog messageLog("MapCheck");
   _requirement.ValidateRequirement(messageLog, GetOwner(), [this] { return FUObjectToken::Create(GetOwner(), FText::FromString(GetReadableName())); });
}

const FTATSceneRequirement* UTATSelfDestructRequirementComponent::FindSceneRequirement() const
{
   return &_requirement;
}

#endif
