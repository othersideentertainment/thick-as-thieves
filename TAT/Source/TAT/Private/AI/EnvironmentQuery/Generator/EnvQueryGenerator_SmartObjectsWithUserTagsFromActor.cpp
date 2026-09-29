// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/EnvironmentQuery/Generator/EnvQueryGenerator_SmartObjectsWithUserTagsFromActor.h"

// ue
#include "GameplayTagAssetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryGenerator_SmartObjectsWithUserTagsFromActor)

#define LOCTEXT_NAMESPACE "TATEnvQueryGenerator"

FText UEnvQueryGenerator_SmartObjectsWithUserTagsFromActor::GetDescriptionTitle() const
{
   FFormatNamedArguments Args;
   Args.Add(TEXT("DescribeContext"), UEnvQueryTypes::DescribeContext(QueryOriginContext));
   return FText::Format(LOCTEXT("DescriptionGenerateSmartObjects", "[TAT] Smart Object slots around {DescribeContext} with extracted user tags"), Args);
}

void UEnvQueryGenerator_SmartObjectsWithUserTagsFromActor::InjectIntoSmartObjectRequest(FEnvQueryInstance& queryInstance, FSmartObjectRequest& request) const
{
   const AController* queryOwner = Cast<AController>(queryInstance.Owner.Get());
   if (queryOwner == nullptr)
   {	
      return;
   }
   
   // The pawn is the owner of the gameplay tags in TAT.
   const IGameplayTagAssetInterface* assetInterface = Cast<IGameplayTagAssetInterface>(queryOwner->GetPawn());
   if(assetInterface == nullptr)
   {
      return;      
   }
   assetInterface->GetOwnedGameplayTags(request.Filter.UserTags);
}

#undef LOCTEXT_NAMESPACE
