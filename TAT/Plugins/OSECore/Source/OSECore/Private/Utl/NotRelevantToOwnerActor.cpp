// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Utl/NotRelevantToOwnerActor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NotRelevantToOwnerActor)

// ose

// ue
#include "Iris/ReplicationSystem/NetRefHandle.h"
#include "Iris/ReplicationSystem/ReplicationSystem.h"
#include "Iris/ReplicationSystem/Filtering/NetObjectFilter.h"
#include "Net/Iris/ReplicationSystem/EngineReplicationBridge.h"
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"

ANotRelevantToOwnerActor::ANotRelevantToOwnerActor()
{
}

void ANotRelevantToOwnerActor::BeginReplication()
{
   Super::BeginReplication();
   
   _TryUpdateConnectionFilter();
}

bool ANotRelevantToOwnerActor::IsNetRelevantFor(const AActor* realViewer, const AActor* viewTarget, const FVector& srcLocation) const
{
   if (IsOwnedBy(realViewer) || IsOwnedBy(viewTarget))
   {
      return false;
   }

   return Super::IsNetRelevantFor(realViewer, viewTarget, srcLocation);
}

void ANotRelevantToOwnerActor::SetOwner(AActor* newOwner)
{
   if (Owner != newOwner)
   {
      Super::SetOwner(newOwner);
      // Owner might have been set post-spawn
      _TryUpdateConnectionFilter();
   }
}

void ANotRelevantToOwnerActor::_TryUpdateConnectionFilter()
{
   UEngineReplicationBridge* replicationBridge = UE::Net::FReplicationSystemUtil::GetActorReplicationBridge(this);
   if(replicationBridge == nullptr)
   {
      return;
   }

   UE::Net::FNetRefHandle netRefHandle = replicationBridge->GetReplicatedRefHandle(this);
   if(!netRefHandle.IsValid())
   {
      return;
   }
   
   if (UNetConnection* connection = GetNetConnection())
   {
      // exclude the owning connection from the allowed connections
      const uint32 connectionId = connection->GetConnectionHandle().GetParentConnectionId();
      TBitArray<> connectionMask(false, connectionId + 1);
      connectionMask.Insert(true, connectionId);

      UReplicationSystem* replicationSystem = UE::Net::FReplicationSystemUtil::GetReplicationSystem(this);
      check(replicationSystem);
      replicationSystem->SetConnectionFilter(netRefHandle, connectionMask, UE::Net::ENetFilterStatus::Disallow);
   }
}

