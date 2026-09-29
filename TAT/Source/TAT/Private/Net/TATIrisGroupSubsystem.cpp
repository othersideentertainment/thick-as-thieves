// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Net/TATIrisGroupSubsystem.h"

// ue
#include "Iris/ReplicationSystem/NetObjectGroupHandle.h"
#include "Iris/ReplicationSystem/ReplicationSystem.h"
#include "Iris/ReplicationSystem/Filtering/NetObjectFilter.h"
#include "Net/Iris/ReplicationSystem/EngineReplicationBridge.h"
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATIrisGroupSubsystem)

UE::Net::FNetObjectGroupHandle UTATIrisGroupSubsystem::FGroupLookup::GetGroup(const FName& key, UReplicationSystem* replicationSystem)
{
   if(const UE::Net::FNetObjectGroupHandle* found = _groups.Find(key))
   {
      return *found;
   }

   if(replicationSystem == nullptr)
   {
      return UE::Net::FNetObjectGroupHandle::GetInvalid();
   }

   UE::Net::FNetObjectGroupHandle group = replicationSystem->CreateGroup(key);
   replicationSystem->AddExclusionFilterGroup(group);
   _groups.Add(key, group);
   return group;
}


void UTATIrisGroupSubsystem::FGroupLookup::Cleanup(UReplicationSystem* replicationSystem)
{
   if(!IsValid(replicationSystem))
   {
      return;
   }

   for(const TPair<FName, UE::Net::FNetObjectGroupHandle>& pair : _groups)
   {
      replicationSystem->DestroyGroup(pair.Value);
   }
}

bool UTATIrisGroupSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   UWorld* world = CastChecked<UWorld>(outer);
   return !world->IsNetMode(NM_Client);
}

void UTATIrisGroupSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
}

void UTATIrisGroupSubsystem::Deinitialize()
{
   Super::Deinitialize();

   if (_replicationSystem)
   {
      _groups.Cleanup(_replicationSystem);
   }
}

void UTATIrisGroupSubsystem::AddActorToGroup(const AActor* actor, const FName& groupName)
{
   check(actor);
   UReplicationSystem* replicationSystem = _GetReplicationSystem(actor);
   if(replicationSystem == nullptr)
   {
      return;
   }
   
   UEngineReplicationBridge* replicationBridge = UE::Net::FReplicationSystemUtil::GetActorReplicationBridge(actor);
   if(replicationBridge == nullptr)
   {
      return;
   }

   UE::Net::FNetObjectGroupHandle groupHandle = _groups.GetGroup(groupName, replicationSystem);
   UE::Net::FNetRefHandle objectHandle = replicationBridge->GetReplicatedRefHandle(actor);
   if(objectHandle.IsValid() && groupHandle.IsValid())
   {
      replicationSystem->AddToGroup(groupHandle, objectHandle);
   }
}

void UTATIrisGroupSubsystem::AllowGroupForPlayer(const AActor* actorConnection, const FName& groupName)
{
   check(actorConnection);
   UNetConnection* connection = actorConnection->GetNetConnection();
   if (connection == nullptr)
   {
      return;
   }

   UReplicationSystem* replicationSystem = _GetReplicationSystem(actorConnection);
   if(replicationSystem == nullptr)
   {
      return;
   }

   UE::Net::FNetObjectGroupHandle groupHandle = _groups.GetGroup(groupName, replicationSystem);
   if(groupHandle.IsValid())
   {
      replicationSystem->SetGroupFilterStatus(groupHandle, connection->GetConnectionHandle().GetParentConnectionId(), UE::Net::ENetFilterStatus::Allow);
   }
}

bool UTATIrisGroupSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

UReplicationSystem* UTATIrisGroupSubsystem::_GetReplicationSystem(const AActor* source)
{
   if(_replicationSystem)
   {
      return _replicationSystem;
   }

   // Just need to keep track of one replications system so the groups can be cleaned up
   _replicationSystem = UE::Net::FReplicationSystemUtil::GetReplicationSystem(source);
   return _replicationSystem;
}
