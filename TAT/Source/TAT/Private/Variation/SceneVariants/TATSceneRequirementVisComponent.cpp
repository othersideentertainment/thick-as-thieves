// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneRequirementVisComponent.h"

// tat
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSceneRequirementVisComponent)

// Sets default values for this component's properties
UTATSceneRequirementVisComponent::UTATSceneRequirementVisComponent()
{
   bIsEditorOnly = true;
   bEditableWhenInherited = false;
#if WITH_EDITORONLY_DATA
   SetIsVisualizationComponent(true);
#endif
}

#if WITH_EDITOR
const FTATSceneRequirement* UTATSceneRequirementVisComponent::FindSceneRequirement() const
{
   const AActor* actor = GetOwner();
   check(actor);
   UClass* actorClass = actor->GetClass();
   for (const FStructProperty* property : TFieldRange<FStructProperty>(actorClass, EFieldIterationFlags::IncludeSuper))
   {
      if (property->Struct == StaticStruct<FTATSceneRequirement>())
      {
         const FTATSceneRequirement* requirement = property->ContainerPtrToValuePtr<FTATSceneRequirement>(actor);
         return requirement;
      }
   }

   return nullptr;
}

void UTATSceneRequirementVisComponent::CheckForErrors()
{
   Super::CheckForErrors();

   if (ShouldValidate)
   {
      if (const FTATSceneRequirement* requirement = FindSceneRequirement())
      {
         FMessageLog messageLog("MapCheck");
         requirement->ValidateRequirement(messageLog, GetOwner(), [this] { return FUObjectToken::Create(GetOwner(), FText::FromString(GetOwner()->GetActorNameOrLabel())); });
      }
   }
}
#endif

