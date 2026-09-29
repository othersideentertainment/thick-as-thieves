// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/GameplayBehavior/GameplayBehavior_BehaviorTreeInjection.h"

// ose
#include "AI/GameplayBehavior/GameplayBehaviorConfig_BehaviorTreeInjection.h"

// ue
#include "AIController.h"
#include "TimerManager.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayBehavior_BehaviorTreeInjection)

//----------------------------------------------------------------------//
// UGameplayBehavior_BehaviorTreeInjection
//----------------------------------------------------------------------//

UGameplayBehavior_BehaviorTreeInjection::UGameplayBehavior_BehaviorTreeInjection(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{

}

bool UGameplayBehavior_BehaviorTreeInjection::Trigger(AActor& inAvatar, const UGameplayBehaviorConfig* config /* = nullptr*/, AActor* smartObjectOwner /* = nullptr*/)
{
   const UGameplayBehaviorConfig_BehaviorTreeInjection* btConfig = Cast<const UGameplayBehaviorConfig_BehaviorTreeInjection>(config);
   if (!btConfig)
   {
      UE_VLOG(&inAvatar, LogGameplayBehavior, Warning, TEXT("Failed to trigger behavior %s for %s due to Config being null"),
         *GetName(), *inAvatar.GetName());
      return false;
   }

   UBehaviorTree* behaviorTree = btConfig->GetBehaviorTree();
   if (!behaviorTree)
   {
      UE_VLOG(&inAvatar, LogGameplayBehavior, Warning, TEXT("Failed to trigger behavior %s for %s due to Config->BehaviorTree being null"),
         *GetName(), *inAvatar.GetName());
      return false;
   }

   const FGameplayTag& injectionTag = btConfig->GetInjectionTag();
   if (!injectionTag.IsValid())
   {
      UE_VLOG(&inAvatar, LogGameplayBehavior, Warning, TEXT("Failed to trigger behavior %s for %s due to Config->InjectionTag being invalid"),
         *GetName(), *inAvatar.GetName());
      return false;
   }

   // note that the value stored in this property is unreliable if we're in the CDO
   // If you need this to be reliable set InstantiationPolicy to Instantiate
   _aiController = UAIBlueprintHelperLibrary::GetAIController(&inAvatar);
   if (!_aiController)
   {
      UE_VLOG(&inAvatar, LogGameplayBehavior, Warning, TEXT("Failed to trigger behavior %s due to %s not being AI-controlled"),
         *GetName(), *inAvatar.GetName());
      return false;
   }

   UBehaviorTreeComponent* btComponent = Cast<UBehaviorTreeComponent>(_aiController->GetBrainComponent());
   if (!btComponent)
   {
      UE_VLOG(&inAvatar, LogGameplayBehavior, Warning, TEXT("Failed to trigger behavior %s due to %s missing a BehaviorTreeComponent"),
         *GetName(), *inAvatar.GetName());
      return false;
   }

   btComponent->SetDynamicSubtree(injectionTag, behaviorTree);
   return true;
}

void UGameplayBehavior_BehaviorTreeInjection::EndBehavior(AActor& inAvatar, const bool interrupted)
{
   Super::EndBehavior(inAvatar, interrupted);
}

