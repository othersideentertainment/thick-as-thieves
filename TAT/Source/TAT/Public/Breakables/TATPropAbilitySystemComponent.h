// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/OSEAbilitySystemComponent.h"

#include "TATPropAbilitySystemComponent.generated.h"

#define DEBUG_PROP_EFFECTS !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

// A breakable-specific subclass of the ability system component in order to have
// more control over (and reduce) what is replicated.
//
// NOTE(2023-09-19): The specific replication is still in flux, and is subject to change
UCLASS()
class TAT_API UTATPropAbilitySystemComponent : public UOSEAbilitySystemComponent
{
	GENERATED_BODY()

public:
   UTATPropAbilitySystemComponent();

   virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& outLifetimeProps) const override;

   template <class T >
   const T* AddUnreplicatedSet()
   {
      return (T*)GetOrCreateUnreplicatedAttributeSubobject(T::StaticClass());
   }

protected:

   virtual void InitializeComponent() override;
   const UAttributeSet* GetOrCreateUnreplicatedAttributeSubobject(TSubclassOf<UAttributeSet> attributeClass);
   virtual bool ReplicateSubobjects(class UActorChannel* channel, class FOutBunch* bunch, FReplicationFlags* repFlags) override;
   virtual void ReadyForReplication() override;

   virtual void ForceReplication() override;
   virtual ELifetimeCondition GetReplicationCondition() const override;

#if DEBUG_PROP_EFFECTS
   virtual void BeginPlay() override;
#endif

private:
#if DEBUG_PROP_EFFECTS
   void _DebugGameplayEffectAppliedToSelf(UAbilitySystemComponent* source, const FGameplayEffectSpec& specApplied, FActiveGameplayEffectHandle activeHandle);
#endif

   bool _hasForcedReplication = false;
};
