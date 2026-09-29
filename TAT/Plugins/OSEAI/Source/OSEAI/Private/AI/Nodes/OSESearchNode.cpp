// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Nodes/OSESearchNode.h"

// ose
#include "AI/Nodes/OSESearchNodeManagerComponent.h"
#include "AI/Utility/UtilityAITokenOwnerGameplayTagCount.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESearchNode)

DEFINE_LOG_CATEGORY(LogSearchNode);

AOSESearchNode::AOSESearchNode()
   : Super()
{
   bNetLoadOnClient = false; // this is a server-only object
}

void AOSESearchNode::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      const TArray<FOSEAITokenInfo> defaultTokens = { _searchNodeToken };
      const TArray<FOSEAITokenInfo> maxTokenDebt = { };
      _tokenOwner = UUtilityAITokenOwnerGameplayTagCount::AuthorityCreate(this, defaultTokens, maxTokenDebt);

      if (UOSESearchNodeManagerComponent* manager = GetWorld()->GetSubsystem<UOSESearchNodeManagerComponent>())
      {
         manager->AddSearchNode(this);
      }
      else
      {
         UE_LOG(LogSearchNode, Warning, TEXT("Unable to register with SearchNodeManager!"));
      }
   }
}

void AOSESearchNode::BeginCooldown()
{
   _cooldownStartTime = GetWorld()->GetTimeSeconds();
}

bool AOSESearchNode::IsCoolingDown()
{
   // If "_cooldownStartTime" is 0.0, the search node has never been activated.
   // Without this check, `IsCoolingDown` will return `true` as long as the game has been running
   // for less than `_cooldownInSeconds`.
   if (_cooldownStartTime <= 0.0)
      return false;
   return GetWorld()->GetTimeSeconds() - _cooldownStartTime <= _cooldownInSeconds;
}

