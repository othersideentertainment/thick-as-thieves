// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Breakables/TATPropAbilitySystemComponent.h"

// tat
#if DEBUG_PROP_EFFECTS
#include "Abilities/Attributes/TATPropHealthAttributeSet.h"
#endif

// ue5
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPropAbilitySystemComponent)

DEFINE_LOG_CATEGORY_STATIC(TATPropAbilitySystemComponent, Log, All);


#if DEBUG_PROP_EFFECTS
static TAutoConsoleVariable<int32> CVarBreakablePrintEffects(
   TEXT("TAT.Breakables.PrintEffects"),
   1,
   TEXT("Whether to print the probably-not-intended gameplay effects applied to props, only on server (default 1)")
);

static TAutoConsoleVariable<int32> CVarBreakablePrintEffectsFilter(
   TEXT("TAT.Breakables.PrintEffects.Filter"),
   1,
   TEXT("Whether to filter out effects that are likely intended (default 1)")
);
#endif

UTATPropAbilitySystemComponent::UTATPropAbilitySystemComponent()
{
   //SetIsReplicatedByDefault(false);
}

void UTATPropAbilitySystemComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DISABLE_REPLICATED_PRIVATE_PROPERTY(UAbilitySystemComponent, SpawnedAttributes);
   DISABLE_REPLICATED_PRIVATE_PROPERTY(UAbilitySystemComponent, OwnerActor);
   DISABLE_REPLICATED_PRIVATE_PROPERTY(UAbilitySystemComponent, AvatarActor);
}

void UTATPropAbilitySystemComponent::InitializeComponent()
{
   // Have bReplicateUsingRegisteredSubObjectList be false when initialing, so added attribute sets are not added as a replicated subobject
   FGuardValue_Bitfield(bReplicateUsingRegisteredSubObjectList, false);
   Super::InitializeComponent();

   // TODO: check that attribute set not added as a subobject
}

const UAttributeSet* UTATPropAbilitySystemComponent::GetOrCreateUnreplicatedAttributeSubobject(TSubclassOf<UAttributeSet> attributeClass)
{
   // Have bReplicateUsingRegisteredSubObjectList be false when initialing, so added attribute sets are not added as a replicated subobject
   FGuardValue_Bitfield(bReplicateUsingRegisteredSubObjectList, false);

   AActor* owningActor = GetOwner();
   const UAttributeSet* myAttributes = nullptr;
   if (owningActor && attributeClass)
   {
      myAttributes = GetAttributeSubobject(attributeClass);
      if (!myAttributes)
      {
         UAttributeSet* spawned = NewObject<UAttributeSet>(owningActor, attributeClass);
         AddSpawnedAttribute(spawned);
         myAttributes = spawned;
      }
   }

   return myAttributes;
}

bool UTATPropAbilitySystemComponent::ReplicateSubobjects(class UActorChannel* channel, class FOutBunch* bunch, FReplicationFlags* repFlags)
{
   // Skip ASC to avoid replicating attribute sets
   return UGameplayTasksComponent::ReplicateSubobjects(channel, bunch, repFlags);
}

void UTATPropAbilitySystemComponent::ReadyForReplication()
{
   // Skip ASC to avoid replicating attribute sets
   UGameplayTasksComponent::ReadyForReplication();
}

void UTATPropAbilitySystemComponent::ForceReplication()
{
   // Enable component for replication once forced
   // This is generally only called when adding gameplay cues, which is good for this use-case
   if (!_hasForcedReplication)
   {
      _hasForcedReplication = true;
      if(GetOwner()->IsReplicatedActorComponentRegistered(this))
      {
         GetOwner()->SetReplicatedComponentNetCondition(this, COND_None);
      }
   }

   Super::ForceReplication();
}

ELifetimeCondition UTATPropAbilitySystemComponent::GetReplicationCondition() const
{
   return _hasForcedReplication ? COND_None : COND_OwnerOnly;
}

#if DEBUG_PROP_EFFECTS
void UTATPropAbilitySystemComponent::BeginPlay()
{
   Super::BeginPlay();

   if(GetOwner()->HasAuthority())
   {
      OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::_DebugGameplayEffectAppliedToSelf);
   }
}

void UTATPropAbilitySystemComponent::_DebugGameplayEffectAppliedToSelf(UAbilitySystemComponent* source, const FGameplayEffectSpec& specApplied, FActiveGameplayEffectHandle activeHandle)
{
   if (!CVarBreakablePrintEffects.GetValueOnGameThread())
   {
      return;
   }

   if(CVarBreakablePrintEffectsFilter.GetValueOnGameThread())
   {
      // Assumption: if an effect modifies a prop attribute, then it is probably intended. If not, ???
      for (const FGameplayEffectModifiedAttribute& mod : specApplied.ModifiedAttributes)
      {
         if (mod.Attribute.GetAttributeSetClass() == UTATPropHealthAttributeSet::StaticClass())
         {
            return;
         }
      }

      for (const FGameplayModifierInfo& mod : specApplied.Def->Modifiers)
      {
         if (mod.Attribute.GetAttributeSetClass() == UTATPropHealthAttributeSet::StaticClass())
         {
            return;
         }
      }
   }

   FString effectName = specApplied.Def->GetName();
   effectName.RemoveFromStart(TEXT("Default__"));
   effectName.RemoveFromEnd(TEXT("_c"));

   const FString message = FString::Printf(TEXT("Breakable %s had effect %s applied to it"), *GetOwner()->GetActorNameOrLabel(), *effectName);
   GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10, FColor::Orange, message);
   UE_LOG(TATPropAbilitySystemComponent, Warning, TEXT("BreakableEffect: %s"), *message);
}
#endif
