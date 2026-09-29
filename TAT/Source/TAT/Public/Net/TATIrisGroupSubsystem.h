// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "Iris/ReplicationSystem/NetObjectGroupHandle.h"

#include "TATIrisGroupSubsystem.generated.h"


class UReplicationSystem;

// A world subsystem that coordinates the creation of iris exclusion groups
//
// Actors can add themselves to groups, and then player connections can opt
// into those groups if it is relevant to them
UCLASS()
class TAT_API UTATIrisGroupSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

   void AddActorToGroup(const AActor* actor, const FName& groupName);
   void AllowGroupForPlayer(const AActor* connectionActor, const FName& groupName);


   struct FAdaptor
   {
      FAdaptor(UTATIrisGroupSubsystem& subsystem, const FName& name)
         : _subsystem(subsystem), _name(name)
      {}
      
      void AddActorToGroup(const AActor* actor) const
      {
         _subsystem.AddActorToGroup(actor, _name);
      }
      
      void AllowGroupForPlayer(const AActor* connection) const
      {
         _subsystem.AllowGroupForPlayer(connection, _name);
      }
   private:
      UTATIrisGroupSubsystem& _subsystem;
      FName _name;
   };
   
   FAdaptor ForTeam(const uint8 team)
   {
      return FAdaptor(*this, FName(EName::Team, team));
   }

   FAdaptor ForTag(const FGameplayTag& tag)
   {
      return FAdaptor(*this, tag.GetTagName());
   }

   FAdaptor ForName(const FName& name)
   {
      return FAdaptor(*this, name);
   }

protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;
   UReplicationSystem* _GetReplicationSystem(const AActor* source);
   
   UPROPERTY(Transient)
   TObjectPtr<UReplicationSystem> _replicationSystem;

   struct FGroupLookup
   {
      UE::Net::FNetObjectGroupHandle GetGroup(const FName& key, UReplicationSystem* replicationSystem);
      void Cleanup(UReplicationSystem* replicationSystem);
   private:
      TMap<FName, UE::Net::FNetObjectGroupHandle> _groups;
   };
   FGroupLookup _groups;
};
