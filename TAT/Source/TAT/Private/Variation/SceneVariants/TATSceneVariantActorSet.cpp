// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneVariantActorSet.h"

// tat
#include "Variation/SceneVariants/TATSceneRequirementVisComponent.h"
#include "Variation/SceneVariants/TATSceneVariantUtils.h"

// ue
#include "Misc/UObjectToken.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSceneVariantActorSet)

// Sets default values
ATATSceneVariantActorSet::ATATSceneVariantActorSet()
{
   PrimaryActorTick.bCanEverTick = false;

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
   RootComponent->SetMobility(EComponentMobility::Static);

#if WITH_EDITOR
   _visualizationComponent = CreateEditorOnlyDefaultSubobject<UTATSceneVariantActorSetVisComponent>(TEXT("VisComponent"));
   _requirementVisComponent = CreateEditorOnlyDefaultSubobject<UTATSceneRequirementVisComponent>("SceneRequirementVis");
   if (_requirementVisComponent)
   {
      _requirementVisComponent->ShouldValidate = false;
   }
#endif
}

// Called when the game starts or when spawned
void ATATSceneVariantActorSet::BeginPlay()
{
   Super::BeginPlay();

   if (!SceneRequirement.IsNone() && !UTATSceneVariantUtils::ResolveBoolRequirement(GetWorld(), SceneRequirement))
   {
      // Skip replication of these destructions, since clients will also do that separately
      UE::Net::FScopedIgnoreStaticActorDestruction ignoredActorDestruction;
      for (AActor* actorToDestroy : Actors)
      {
         if (IsValid(actorToDestroy))
         {
            constexpr bool kNetForce = true; // might as well not wait for the server, if there may be side effects
            actorToDestroy->Destroy(kNetForce);
         }
      }
   }
}

#if WITH_EDITOR
void ATATSceneVariantActorSet::CheckForErrors()
{
   Super::CheckForErrors();

   FMessageLog messageLog("MapCheck");
   SceneRequirement.ValidateRequirement(messageLog, this, [this] { return FUObjectToken::Create(this, FText::FromString(GetActorNameOrLabel())); });
}
#endif

UTATSceneVariantActorSetVisComponent::UTATSceneVariantActorSetVisComponent()
{
   bIsEditorOnly = true;
   bEditableWhenInherited = false;
#if WITH_EDITORONLY_DATA
   SetIsVisualizationComponent(true);
#endif
}
