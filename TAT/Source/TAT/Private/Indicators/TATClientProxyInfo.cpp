// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Indicators/TATClientProxyInfo.h"

// tat
#include "Indicators/TATThiefVisionSubsystem.h"

// ue
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClientProxyInfo)

bool FTATClientProxyInfo::IsActive(float indicatorLifeSpan, float currentServerWorldTime) const
{
   return LastRefreshServerWorldTime > 0 && indicatorLifeSpan > 0 && (LastRefreshServerWorldTime + indicatorLifeSpan > currentServerWorldTime);
}

float FTATClientProxyInfo::GetRemainingLifeSpan(float indicatorLifeSpan, float currentServerWorldTime) const
{
   if (LastRefreshServerWorldTime > 0 && indicatorLifeSpan > 0)
   {
      return FMath::Max(0.0f, indicatorLifeSpan + LastRefreshServerWorldTime - currentServerWorldTime);
   }
   return 0.0f;
}

bool FTATClientProxyInfo::ShouldUpdate(const FTATClientProxyInfo& newInfo) const
{
   return newInfo.LastRefreshServerWorldTime > LastRefreshServerWorldTime;
}

void FTATClientProxyInfoArray::PreReplicatedRemove(const TArrayView<int32>& removedIndices, int32 finalSize)
{
   APlayerController* pc = OwningLocalPlayerControllerWeak.Get();
   if (pc != nullptr && pc->IsLocalController())
   {
      if (UTATThiefVisionSubsystem* thiefVisionSubsystem = pc->GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>())
      {
         thiefVisionSubsystem->_ClientOnPreReplicatedRemove(pc, Items, removedIndices, finalSize);
      }
   }
}

void FTATClientProxyInfoArray::PostReplicatedAdd(const TArrayView<int32>& addedIndices, int32 finalSize)
{
   APlayerController* pc = OwningLocalPlayerControllerWeak.Get();
   if (pc != nullptr && pc->IsLocalController())
   {
      if (UTATThiefVisionSubsystem* thiefVisionSubsystem = pc->GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>())
      {
         thiefVisionSubsystem->_ClientOnPostReplicatedAdd(pc, Items, addedIndices, finalSize);
      }
   }
}

void FTATClientProxyInfoArray::PostReplicatedChange(const TArrayView<int32>& changedIndices, int32 finalSize)
{
   APlayerController* pc = OwningLocalPlayerControllerWeak.Get();
   if (pc != nullptr && pc->IsLocalController())
   {
      if (UTATThiefVisionSubsystem* thiefVisionSubsystem = pc->GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>())
      {
         thiefVisionSubsystem->_ClientOnPostReplicatedChange(pc, Items, changedIndices, finalSize);
      }
   }
}
