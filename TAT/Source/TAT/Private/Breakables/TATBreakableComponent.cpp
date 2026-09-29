// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Breakables/TATBreakableComponent.h"

// tat
#include "Abilities/Attributes/TATPropHealthAttributeSet.h"
#include "AI/Perception/TATAISense_Hearing.h"
#include "Breakables/TATBreakableAudioInterface.h"
#include "Breakables/TATBreakableTags.h"
#include "Breakables/TATPropAbilitySystemComponent.h"
#include "Developer/TATProjectSettings.h"
#include "Graphics/TATHighlightStateMgrComponent.h"
#include "Player/TATPlayerStatsTags.h"

// ose
#include "Abilities/Effects/OSEGameplayEffectSet.h"
#include "Player/OSEPlayerStats.h"

// wwise?
#include "AkAudioEvent.h"
#include "AkComponent.h"

// ue5
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Engine/CurveTable.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATBreakableComponent)


#define LOCTEXT_NAMESPACE "TATBreakableComponent"

DEFINE_LOG_CATEGORY_STATIC(LogTATBreakableComponent, Log, All);

TAutoConsoleVariable<int32> CVarBreakableDebugVisShowTags(
   TEXT("TAT.Breakables.DebugVis.ShowTags"),
   0,
   TEXT("Whether to show the tags of breakables")
);

UTATBreakableComponent::UTATBreakableComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;

   bWantsInitializeComponent = true;

   // lazily enabled
   SetIsReplicatedByDefault(true);
}

UTATBreakableComponent* UTATBreakableComponent::GetBreakableComponentFromActor(AActor* actor)
{
   return actor ? actor->FindComponentByClass<UTATBreakableComponent>() : nullptr;
}

void UTATBreakableComponent::DispatchBreakableDamageFromGameplayCue(AActor* actor, const FGameplayCueParameters& params)
{
   if (UTATBreakableComponent* breakable = GetBreakableComponentFromActor(actor))
   {
      breakable->_HandleDamageCue(params);
   }
}

void UTATBreakableComponent::InitializeComponent()
{
   Super::InitializeComponent();

   UTATPropAbilitySystemComponent* propAsc = GetOwner()->FindComponentByClass<UTATPropAbilitySystemComponent>();
   _abilitySystemComponent = propAsc;
   if (!ensure(_abilitySystemComponent))
   {
      return;
   }

   // Explicitly call initialize component to ensure ordering
   if (!_abilitySystemComponent->bWantsInitializeComponent)
   {
      _abilitySystemComponent->InitializeComponent();
   }

   // Note on const-cast: The attribute set could have been passed directly to this component by the owning actor,
   // but I am trying to reduce the surface area needed on an actor to correctly set this up, since there are going to
   // be multiple classes that use it, and it will be important to keep the maintenance surface area minimal.
   // CONSIDER: Should it skip creating the attribute set if non-breakable?
   _healthAttributes = const_cast<UTATPropHealthAttributeSet*>(propAsc->AddUnreplicatedSet<UTATPropHealthAttributeSet>());
   if (ensure(_healthAttributes))
   {
      // We init _health to max-health here to handle the case of dynamically-spawned actors, which do not call PostLoad()
      if (!_everDamaged)
      {
         _health = GetMaxHealth();
      }

      _healthAttributes->InitPropHealth(_health);
      _healthAttributes->InitPropHealthMax(GetMaxHealth());
   }

   const UTATProjectSettings& settings = UTATProjectSettings::Get();
   if (!_isBreakable)
   {
      // If not breakable, gain invulnerability
      _abilitySystemComponent->AddLooseGameplayTag(settings.InvulnerableStatusTag);
   }

   _abilitySystemComponent->AddLooseGameplayTags(settings.BreakableIntrinsicTags);
   _abilitySystemComponent->AddLooseGameplayTags(_intrinsicTags);
}

void UTATBreakableComponent::PostLoad()
{
   Super::PostLoad();

   // Initialize health from max health, so that it is the value compared against in the CDO for the replicator
   _health = GetMaxHealth();

   // Workaround for bug where _maxHealth would get stuck with the wrong curve when overridden in a child (in PIE)
   // 
   // FScalableFloat caches a reference to its curve internally, which is initialized on evaluate. By evaluating
   // the curve in post load, this appears to set the cached field on a value that is then carried over the inherited
   // values, even if the curve is overridden. `SetScalingValue` sets the all the fields to its current value, but
   // resets the cached curve. This should still be valid if no curve is set.
   _maxHealth.SetScalingValue(_maxHealth.Value, _maxHealth.Curve.RowName, const_cast<UCurveTable*>(_maxHealth.Curve.CurveTable.Get()));
}

void UTATBreakableComponent::SetIsBreakableByDefault(bool breakable)
{
   check(FUObjectThreadContext::Get().IsInConstructor);
   _isBreakable = breakable;
}

void UTATBreakableComponent::ForceRepair()
{
   _CompleteRepair();
}

bool UTATBreakableComponent::IsBroken() const
{
   return _health == 0;
}

float UTATBreakableComponent::GetHealth() const
{
   return _health;
}

float UTATBreakableComponent::GetMaxHealth() const
{
   // There is room to cache this if it is very hot, but no need to start there
   return _maxHealth.GetValue();
}

void UTATBreakableComponent::AppendDebugString(FString& outResult) const
{
   if (IsBroken())
   {
      outResult.Append(TEXT("Broken"));
   }
   else
   {
      // for now don't try to show fractions, even though it will happen
      outResult.Appendf(TEXT("%.0f/%.0f"), GetHealth(), GetMaxHealth());
   }

   if (!IsBreakable())
   {
      outResult.Append(TEXT("\nInvulnerable"));
   }

   if (CVarBreakableDebugVisShowTags.GetValueOnGameThread() && _abilitySystemComponent)
   {
      FGameplayTagContainer tags;
      _abilitySystemComponent->GetOwnedGameplayTags(tags);
      outResult.Append(TEXT("\n"));
      outResult.Append(tags.ToStringSimple());
   }
}

bool UTATBreakableComponent::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return IsBroken() && (_interactableWhenBroken || _isRepairable);
}

void UTATBreakableComponent::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   // TODO: move to settings
   prompt.ErrorMessage = LOCTEXT("Broken", "Broken");

   if (_isRepairable)
   {
      prompt.HoldAction = LOCTEXT("Repair", "Repair");
   }
}

FInteractStartResult UTATBreakableComponent::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   if (_isRepairable)
   {
      const UTATProjectSettings& settings = UTATProjectSettings::Get();
      FInteractStartResult result = FInteractStartResult::Wait(_repairDuration.GetValue());
      result.HoldActionCues = settings.BreakableRepairHeldActionCues;
      result.HoldAnimationTag = settings.BreakableRepairInteractAnimation;
      return result;
   }

   return FInteractStartResult();
}

bool UTATBreakableComponent::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (_isRepairable && IsBroken() && context.IsComplete())
   {
      _CompleteRepair();
   }

   return true;
}

void UTATBreakableComponent::ShowHighlight_Implementation(bool showHighlight)
{
   // I am not in love with this, and there may not always be a mesh that can be separately highlighted that makes sense
   if (_isRepairable && _highlightTagForRepair.IsValid())
   {
      UTATHighlightStateMgrComponent::HighlightMeshesWithTag(GetOwner(), _highlightTagForRepair, showHighlight);
   }
}

// Called when the game starts
void UTATBreakableComponent::BeginPlay()
{
   Super::BeginPlay();

   if (!ensure(_abilitySystemComponent))
   {
      return;
   }

   if (IsBreakable() && ensure(_healthAttributes) && GetOwner()->HasAuthority())
   {
      _healthAttributes->OnOutOfHealth.AddUObject(this, &UTATBreakableComponent::_AuthorityOnOutOfHealth);
      _abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UTATPropHealthAttributeSet::GetPropHealthAttribute()).AddUObject(
         this, &UTATBreakableComponent::_AuthorityOnHealthAttributeChanged);

      _abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UTATPropHealthAttributeSet::GetPropDamageAttribute()).AddUObject(
         this, &UTATBreakableComponent::_AuthorityOnDamageAttributeChanged);

      _AuthorityGrantInitialEffects();
   }

   _UpdateBrokenTag();

   if (IsBroken())
   {
      OnBrokenChanged.Broadcast(true);
   }
}

void UTATBreakableComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(UTATBreakableComponent, _health, params);
}

void UTATBreakableComponent::_AuthorityGrantInitialEffects()
{
   check(_abilitySystemComponent);
   if (_intrinsicEffects.Num() == 0)
   {
      return;
   }

   // Granting effects would normally flush dormancy, which is undesirable
   // if they are just granting (conditional) modifiers to non-replicated attributes
   // So clear the owner while it is being set
   check(_abilitySystemComponent->GetOwnerActor() == GetOwner());
   _abilitySystemComponent->SetOwnerActor(nullptr);

   for (UOSEGameplayEffectSet* effectSet : _intrinsicEffects)
   {
      effectSet->ApplyEffects(_abilitySystemComponent);
   }

   _abilitySystemComponent->SetOwnerActor(GetOwner());

   // TODO: add some safety checks?
}

void UTATBreakableComponent::_UpdateBrokenTag()
{
   check(_abilitySystemComponent);
   _abilitySystemComponent->SetLooseGameplayTagCount(TAG_Status_Broken, IsBroken() ? 1 : 0);
}

void UTATBreakableComponent::_OnRep_Health(float previousHealth)
{
   if(_health == previousHealth) return;

   // Might not help much on clients, but keeping in sync may help with replays
   _MarkEverDamaged();

   // pipe value back to ASC, in case something uses it
   if (_abilitySystemComponent)
   {
      _abilitySystemComponent->SetNumericAttributeBase(UTATPropHealthAttributeSet::GetPropHealthAttribute(), _health);
   }

   _OnHealthChanged(previousHealth);
}

void UTATBreakableComponent::_OnHealthChanged(float previousHealth)
{
   UE_LOG(LogTATBreakableComponent, Verbose, TEXT("OnHealthChanged [%s] %f/%f"), GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"), GetHealth(), GetMaxHealth());

   // probably guaranteed, but just in case
   if (_health != previousHealth)
   {
      OnHealthChanged.Broadcast(previousHealth, _health);
   }

   const bool wasBroken = previousHealth == 0;
   if (IsBroken() != wasBroken)
   {
      _UpdateBrokenTag();
      OnBrokenChanged.Broadcast(IsBroken());
   }
}

void UTATBreakableComponent::_AuthorityOnHealthAttributeChanged(const FOnAttributeChangeData& data)
{
   // Allow it to replicate once damaged
   _MarkEverDamaged();

   // Proxy health to replicated property
   GetOwner()->FlushNetDormancy();
   MARK_PROPERTY_DIRTY_FROM_NAME(UTATBreakableComponent, _health, this);
   _health = data.NewValue;

   _OnHealthChanged(data.OldValue);
}

void UTATBreakableComponent::_AuthorityOnDamageAttributeChanged(const FOnAttributeChangeData& data)
{
   if (data.NewValue <= data.OldValue)
   {
      return;
   }

   AActor* instigator = nullptr;
   FVector origin = FVector::ZeroVector;
   if (const FGameplayEffectModCallbackData* modData = data.GEModData)
   {
      const FGameplayEffectContextHandle context = modData->EffectSpec.GetContext();
      instigator = context.GetInstigator();
      origin = context.GetOrigin();
   }

   const float damageAmount = FMath::Max(0.0f, data.NewValue - data.OldValue);
   AuthorityOnDamageTaken.Broadcast(instigator, origin, damageAmount);
 
   // This is primarily fired in _HandleDamageCue, but adding a fallback call
   // here if a dedicated server. It isn't needed if all uses are cosmetic,
   // but it may be surprising if missing
   if (IsNetMode(NM_DedicatedServer))
   {
      UnreliableOnDamageTaken.Broadcast(instigator, origin, damageAmount);
   }
}

void UTATBreakableComponent::_MarkEverDamaged()
{
   if (!_everDamaged)
   {
      _everDamaged = true;
      if(GetOwner()->HasActorBegunPlay())
      {
         GetOwner()->SetReplicatedComponentNetCondition(this, COND_None);
      }
   }
}

void UTATBreakableComponent::_AuthorityOnOutOfHealth(AActor* instigator, const FGameplayEffectSpec* spec)
{
   UE_LOG(LogTATBreakableComponent, Verbose, TEXT("OnOutOfHealth [%s]"), GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));

   FVector origin = spec ? spec->GetEffectContext().GetOrigin() : FVector::ZeroVector;
   if (!_skipRecentlyBrokenRpc)
   {
      _MulticastOnRecentlyBroken(origin);
   }

   FTATAuthorityBreakContext context(spec);
   context.OptionalOrigin = origin;
   OnBrokenAuthority.Broadcast(context);

   _AuthorityEmitBrokenStim(origin);
   UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(instigator, TAG_PlayerStats_Objects_Broken);
}

void UTATBreakableComponent::_AuthorityEmitBrokenStim(const FVector& origin)
{
   const FGameplayTag stimTag = _breakStimOverride.IsValid() ? _breakStimOverride : UTATProjectSettings::Get().DefaultBreakableHearingStim;
   const FVector location = origin.IsZero() ? GetOwner()->GetActorLocation() : origin; //< Might want better fallback location, but this is probably okay
   UTATAISense_Hearing::ReportNoiseEvent(this, stimTag, location, GetOwner());
}

void UTATBreakableComponent::_CompleteRepair()
{
   if (GetOwner()->HasAuthority())
   {
      // The ASC does not flush dormancy for instant effects
      GetOwner()->FlushNetDormancy();

      // assumption: effect is async loaded by subsystem
      TSubclassOf<UGameplayEffect> repairEffect = UTATProjectSettings::Get().RepairBreakableEffect.LoadSynchronous();
      if (ensure(_abilitySystemComponent))
      {
         _abilitySystemComponent->ApplyGameplayEffectToSelf(repairEffect.GetDefaultObject(), 0, _abilitySystemComponent->MakeEffectContext());
      }
   }
}

void UTATBreakableComponent::_HandleDamageCue(const FGameplayCueParameters& params)
{
   {
      AActor* instigator = params.GetInstigator();
      const FVector origin = params.EffectContext.GetOrigin();
      const float damageAmount = params.RawMagnitude;
      UnreliableOnDamageTaken.Broadcast(instigator, origin, damageAmount);
   }

   if (_damageSound == nullptr) return;

   UAkComponent* audioComponent = _GetAudioComponent();
   if (audioComponent)
   {
      // TODO: Set RTPC with damage magnitude when that is a thing
      // (and also normalized current health, even though the order may not be consistent)

      constexpr bool stopWhenAttachedObjectDestroyed = true;
      _damageSound->PostOnComponent(audioComponent, nullptr, nullptr, nullptr, (AkCallbackType)0, nullptr, stopWhenAttachedObjectDestroyed);
   }
   else
   {
      // Fallback to playing at location if there is no audio component on actor
      // TODO: allow specifying offset?
      // TODO: May not be fully compatible with RTPCs, at least with this wrapper
      // Only using impact point if there is a hit result. Otherwise it might not be proximate (e.g. AOE)
      const FHitResult* hit = params.EffectContext.GetHitResult();
      const FVector location = (hit && hit->bBlockingHit) ? (FVector)hit->ImpactPoint : GetOwner()->GetActorLocation();
      _damageSound->PostAtLocation(location, FRotator(), GetWorld(), nullptr, nullptr, nullptr, (AkCallbackType)0, nullptr);
   }
}

void UTATBreakableComponent::_MulticastOnRecentlyBroken_Implementation(FVector_NetQuantize origin)
{
   UE_LOG(LogTATBreakableComponent, Verbose, TEXT("OnRecentlyBroken [%s]"), GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));
   OnRecentlyBroken.Broadcast(origin);
}

UAkComponent* UTATBreakableComponent::_GetAudioComponent()
{
   UAkComponent* result = _cachedAudioComponent.Get();
   if (result == nullptr)
   {
      const AActor* owner = GetOwner();
      result = owner->Implements<UTATBreakableAudioInterface>()
         ? ITATBreakableAudioInterface::Execute_GetAkComponentForBreakable(owner)
         : owner->FindComponentByClass<UAkComponent>();
      _cachedAudioComponent = result;
   }

   return result;
}

AActor* FTATAuthorityBreakContext::GetInstigator() const
{
   check(Spec);
   return Spec->GetEffectContext().GetInstigator();
}

#undef LOCTEXT_NAMESPACE
