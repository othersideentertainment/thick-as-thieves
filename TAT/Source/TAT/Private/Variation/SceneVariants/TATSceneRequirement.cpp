// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneRequirement.h"

// tat
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/SceneVariants/TATSceneAsset.h"
#include "Variation/SceneVariants/TATSceneSet.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"

// ue5
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSceneRequirement)

#if WITH_EDITOR
FString FTATSceneRequirement::ToDebugString() const
{
   switch (Type)
   {
   case ETATSceneRequirementType::None:
   {
      return TEXT("None");
   }
   case ETATSceneRequirementType::RequireSceneElement:
   {
      FString elementStr = ElementTag.ToString();
      elementStr.RemoveFromStart(TEXT("MapVariation.SceneElement."));
      return FString::Format(TEXT("{0}:{1}"), {GetNameSafe(Scene), elementStr});
   }
   case ETATSceneRequirementType::RequireVariant:
   {
      return GetNameSafe(Variant);
   }
   default:
      checkNoEntry();
      return FString();
   }
}

void FTATSceneRequirement::ValidateRequirement(FMessageLog& msgLog, const AActor* owningActor, TFunctionRef<TSharedRef<IMessageToken>()> createOwnerToken) const
{
   return ValidateRequirement(msgLog, [owningActor]() { return MapVariationValidationHelper::FindRelevantSceneSets(owningActor); }, createOwnerToken);
}

void FTATSceneRequirement::ValidateRequirement(FMessageLog& msgLog, TFunctionRef<TConstArrayView<TObjectPtr<UTATSceneSetAsset>>()> getSceneSets,
   TFunctionRef<TSharedRef<IMessageToken>()> createOwnerToken) const
{
      if (Type == ETATSceneRequirementType::RequireVariant)
   {
      if (Variant == nullptr)
      {
         msgLog.Warning()
            ->AddToken(createOwnerToken())
            ->AddToken(FTextToken::Create(FText::FromString(TEXT("RequireVariant requirement has no variant set. This will be treated as if it requires a variant that is never found."))));
      }
      else
      {
         TConstArrayView<TObjectPtr<UTATSceneSetAsset>> sceneSets = getSceneSets();
         if (sceneSets.Num())
         {
            const bool hasVariant = sceneSets.ContainsByPredicate([this] (const UTATSceneSetAsset* sceneSet) { return sceneSet && sceneSet->HasVariant(Variant);});
            if(!hasVariant)
            {
               msgLog.Warning()
                  ->AddToken(createOwnerToken())
                  ->AddToken(FTextToken::Create(FText::FromString(TEXT("RequireVariant depends on variant not configured in level"))))
                  ->AddToken(FUObjectToken::Create(Variant));
            }
         }
      }
   }
   else if (Type == ETATSceneRequirementType::RequireSceneElement)
   {
      if (Scene == nullptr)
      {
         msgLog.Warning()
            ->AddToken(createOwnerToken())
            ->AddToken(FTextToken::Create(FText::FromString(TEXT("RequireSceneElement requirement has no scene set. This will be treated as if it is never found."))));
      }
      else
      {
         TConstArrayView<TObjectPtr<UTATSceneSetAsset>> sceneSets = getSceneSets();
         if (sceneSets.Num())
         {
            const bool hasScene = sceneSets.ContainsByPredicate([this](const UTATSceneSetAsset* sceneSet) { return sceneSet && sceneSet->HasScene(Scene); });
            if (!hasScene)
            {
               msgLog.Warning()
                  ->AddToken(createOwnerToken())
                  ->AddToken(FTextToken::Create(FText::FromString(TEXT("RequireSceneElement depends on scene not configured in level"))))
                  ->AddToken(FUObjectToken::Create(Scene));
            }
         }
      }


      // TODO: do cross-checks against available tags in containing world (possible cached)?
      if (!ElementTag.IsValid())
      {
         msgLog.Warning()
            ->AddToken(createOwnerToken())
            ->AddToken(FTextToken::Create(FText::FromString(TEXT("RequireSceneElement requirement has no element tag set. This will be treated as if it is never found."))));
      }
   }
}
#endif
