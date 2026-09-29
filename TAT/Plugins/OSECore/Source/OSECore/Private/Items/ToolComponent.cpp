// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolComponent.h"

// OSE
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/OSEGameplayAbilitySet.h"
#include "Graphics/Mesh/SkeletalMeshComponent1P.h"
#include "Input/OSEEnhancedInputPriority.h"
#include "Items/ToolAbility.h"
#include "Items/ToolSetComponent.h"

// UE4
#include "Animation/AnimInstance.h"
#include "EnhancedInputSubsystems.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolComponent)

namespace ToolHelpers
{
   static void GrantAbilities(UToolComponent* owner, const TArray<UOSEGameplayAbilitySet*>& abilitySets, TArray<FGameplayAbilitySpecHandle>& grantedSpecs)
   {
      UAbilitySystemComponent* asc = owner->GetAbilitySystemComponent();
      if (asc == nullptr || !asc->IsOwnerActorAuthoritative())
      {
         return;
      }

      for (const UOSEGameplayAbilitySet* abilitySet : abilitySets)
      {
         if (!abilitySet)
            continue;

         grantedSpecs.Append(abilitySet->GiveAbilities(asc, owner));
      }
   }

   static void RemoveAbilities(UAbilitySystemComponent* asc, TArray<FGameplayAbilitySpecHandle>& grantedSpecs)
   {
      if (asc == nullptr)
      {
         return;
      }

      for (FGameplayAbilitySpecHandle abilityHandle : grantedSpecs)
      {
         asc->ClearAbility(abilityHandle);
      }
      grantedSpecs.Reset();
   }
}

// Sets default values for this component's properties
UToolComponent::UToolComponent()
   : Super()
   , CounterEquipped(0)
{
   // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
   // off to improve performance if you don't need them.
   PrimaryComponentTick.bCanEverTick = true;
   SetIsReplicatedByDefault(true);
   SetAutoActivate(true);

   ToolVisuals3P.Perspective = EMeshPerspective::ThirdPerson;
   ToolVisuals3P_Unequipped.Perspective = EMeshPerspective::ThirdPerson;
   ToolVisuals1P.Perspective = EMeshPerspective::FirstPerson;

   ToolVisuals3P.MeshClass = USkeletalMeshComponent::StaticClass();
   ToolVisuals3P_Unequipped.MeshClass = USkeletalMeshComponent::StaticClass();
   ToolVisuals1P.MeshClass = USkeletalMeshComponent1P::StaticClass();
}

void UToolComponent::OnComponentCreated()
{
   Super::OnComponentCreated();
   
   AActor* myOwner = GetOwner();
   _toolVisSkeletalMeshComp3p           = ToolVisuals3P.CreateMeshComponent(myOwner);
   _toolVisSkeletalMeshComp3pUnequipped = ToolVisuals3P_Unequipped.CreateMeshComponent(myOwner);
   _toolVisSkeletalMeshComp1p           = ToolVisuals1P.CreateMeshComponent(myOwner);

   // initial hide, to get the unequipped version to show
   if (!IsEquipped())
   {
      HideMesh();
   }
}

void UToolComponent::OnRegister()
{
   Super::OnRegister();

   AActor* myOwner = GetOwner();
   _cachedOwningToolset = myOwner ? myOwner->FindComponentByClass<UToolSetComponent>() : nullptr;

   ToolVisuals3P.RegisterMeshComponent(myOwner);
   ToolVisuals3P_Unequipped.RegisterMeshComponent(myOwner);
   ToolVisuals1P.RegisterMeshComponent(myOwner);

   if (_ShouldModifyVisualsOnRegister())
   {
      _ModifyAllVisuals();
   }
}

void UToolComponent::OnUnregister()
{
   ToolVisuals3P.UnregisterMeshComponent();
   ToolVisuals3P_Unequipped.UnregisterMeshComponent();
   ToolVisuals1P.UnregisterMeshComponent();

   _cachedOwningToolset = nullptr;

   Super::OnUnregister();
}

void UToolComponent::OnComponentDestroyed(bool destroyingHierarchy)
{
   ToolVisuals3P.DestroyMeshComponent();
   ToolVisuals3P_Unequipped.DestroyMeshComponent();
   ToolVisuals1P.DestroyMeshComponent();

   Super::OnComponentDestroyed(destroyingHierarchy);
}

void UToolComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (endPlayReason == EEndPlayReason::Destroyed && IsEquipped())
   {
      _OnDestroyedWhileEquipped();
   }

   Super::EndPlay(endPlayReason);
}

void UToolComponent::ShowMesh()
{
   ToolVisuals3P.ShowMeshComponent();
   ToolVisuals3P_Unequipped.HideMeshComponent();
   ToolVisuals1P.ShowMeshComponent();
   BP_OnShowMesh();
}

void UToolComponent::HideMesh()
{
   ToolVisuals3P.HideMeshComponent();
   ToolVisuals3P_Unequipped.ShowMeshComponent();
   ToolVisuals1P.HideMeshComponent();
   BP_OnHideMesh();
}

//---------------------------------------------------------------------------------------
// IUpgradeQueryInterface
//---------------------------------------------------------------------------------------

int32 UToolComponent::GetUpgradeValue(FGameplayTag tag, int32 fallback /*= 0*/) const
{
   if (const UOSEAbilitySystemComponent* asc = _GetOSEAbilitySystemComponent())
   {
      return asc->GetUpgradeValue(tag, fallback);
   }

   return fallback;
}

void UToolComponent::ModifyVisuals_Implementation(EMeshPerspective perspective, USkeletalMeshComponent* mesh)
{
}

void UToolComponent::_ModifyAllVisuals()
{
   auto dispatchModify = [this](FToolVisuals& visuals)
   {
      if (visuals.MeshComponent)
      {
         ModifyVisuals(visuals.Perspective, visuals.MeshComponent);
      }
   };
   dispatchModify(ToolVisuals3P);
   dispatchModify(ToolVisuals3P_Unequipped);
   dispatchModify(ToolVisuals1P);
}

void UToolComponent::_StowBecauseUnusable()
{
   Execute_SetStowed(this, true);
   _isStowedBecauseUnusable = true;
}

void UToolComponent::_UnstowBecauseUsable()
{
   Execute_SetStowed(this, false);
   _isStowedBecauseUnusable = false;
}

void UToolComponent::_UpdateStowedStateBasedOnUsability()
{
   // We only need to change the Stowed State Based On Usability for tools that are equipped and aren't always unstowed by default
   if (IsEquipped() && StowToolIfUnusable)
   {
      const bool bUsable = CanCurrentlyBeUsed();
      // If we were stowed because our tool was unsuable but can now be used, let's unstow it
      if (_isStowedBecauseUnusable && bUsable)
      {
         _UnstowBecauseUsable();
      }
      // If we're not stowed due to usability but can no longer be used, let's stow it
      else if (!_isStowedBecauseUnusable && !bUsable)
      {
         _StowBecauseUnusable();
      }
   }
}

//---------------------------------------------------------------------------------------
// IToolInterface
//---------------------------------------------------------------------------------------

FOSEToolInput UToolComponent::GetToolInput_Implementation() const
{
   return ToolInput;
}

bool UToolComponent::OnAddToToolSet_Implementation()
{
   // Give the tool ability when tool is added
   _GiveToolAbility();

   // Give the tool set abilities when the tool is added
   ToolHelpers::GrantAbilities(this, InitialAbilitySets, GrantedInitialAbilities);

   // Call base class
   return IToolInterface::OnAddToToolSet_Implementation();
}

bool UToolComponent::OnRemoveFromToolSet_Implementation()
{
   // Remove the tool ability when tool is removed
   _ClearToolAbility();

   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      ToolHelpers::RemoveAbilities(asc, GrantedInitialAbilities);
      ToolHelpers::RemoveAbilities(asc, GrantedEquippedAbilities);
   }

   // Call base class
   return IToolInterface::OnRemoveFromToolSet_Implementation();
}

// NOTE: If the tool set system is locally predicting the equipping of tools,
// equip / unequip may execute on the local client immediately. Execution on server
// and on remote clients will happen later. Keep this in mind when adding anything
// to the equip / unequip methods!

bool UToolComponent::OnEquip_Implementation(const TScriptInterface<IToolInterface>& prevTool)
{
   check(prevTool != this);
   
   check(CounterEquipped == 0);
   CounterEquipped++;

   // reset any temporarily stowed status whenever the tool equip status changes
   _counterStowed = 0;
   _isStowedBecauseUnusable = false;

   // Check if the tool should be stowed already
   const bool bIsStowed = IsToolStowedOnEquip();

   if (!bIsStowed)
   {
      ShowMesh();
      // If the mesh is visible, link anim class layers after mesh is visible but before equip montage
      _LinkAnimClassLayers();
   }
   else
   {
      // We don't call the stow function here so that we don't play the stow animation
      // But we should increment the stowed counter and flag the stow reason as unusable
      ++_counterStowed;
      _isStowedBecauseUnusable = StowToolIfUnusable && !CanCurrentlyBeUsed();
   }

   EToolAnimation equipAnim = !bIsStowed ? EToolAnimation::Equip : EToolAnimation::EquipUnusable;

   float delay = _PlayToolAnimation(equipAnim, ToolAnimations.EquipPlaybackRate);

   // Enforce default timer delay
   if (delay <= 0.0f)
      delay = 0.25f;

   if (ToolAnimations.ShouldOverrideEquipCompleteDelay)
   {
      delay = ToolAnimations.EquipCompleteDelayOverride;
   }

   // Call base class (which currently does nothing)
   IToolInterface::OnEquip_Implementation(prevTool);

   // If we can enter the input context, do it now
   bool didEnterInputContext = _EnterInputContext(ToolInput.InputContext);
   if (ToolInput.InputContext && !didEnterInputContext)
   {
      // If we have a context but couldn't enter it yet, we need to defer it to when the equip finishes,
      // such as in the case of a possession change
      _shouldEnterInputContextAfterFinishEquip = true;
   }
   else
   {
      _shouldEnterInputContextAfterFinishEquip = false;
   }

   check(_isPlayingEquipAnim == false);
   _isPlayingEquipAnim = true;
   if (delay > 0)
   {
      GetOwner()->GetWorldTimerManager().SetTimer(_timerHandle_OnEquipFinished, this, &UToolComponent::_OnEquipFinished, delay, false);
   }
   else
   {
      _OnEquipFinished();
   }

   // Call base class
   return false;
};

// Executes when equip animation is completed
void UToolComponent::_OnEquipFinished()
{
   check(_isPlayingEquipAnim == true);

   if (!IsReady())
   {
      const UOSEAbilitySystemComponent* asc = _GetOSEAbilitySystemComponent();
      // N.B. This calls into FGameplayAbilityActorInfo::IsLocallyControlled, which looks at the cached player controller,
      // so even if the avatar loses possession this will still return true
      if (asc && asc->IsLocallyControlled())
      {
         // If we're locally controlled and not ready, need to stall for replication
         GetOwner()->GetWorldTimerManager().SetTimer(_timerHandle_OnEquipFinished, this, &UToolComponent::_OnEquipFinished, 0.1, false);
         return;
      }
   }

   // Enter this tool's input context if it had to be defered
   // This should succeed since we've waited for replication/possession to finish
   if (_shouldEnterInputContextAfterFinishEquip)
   {
      _EnterInputContext(ToolInput.InputContext);
      _shouldEnterInputContextAfterFinishEquip = false;
   }

   _isPlayingEquipAnim = false;

   // Activate the ability
   _ActivateToolAbility();

   // NOTE: granting the ability on equip finish exposes a client player to half a round trip of latency in addition to any equip delay
   //       However, this does match the behavior of abilities granted by an effect granted by the tool ability (the prior pattern)
   // CONSIDER: Grant earlier, but gate activation until fully equipped?
   ToolHelpers::GrantAbilities(this, EquippedAbilitySets, GrantedEquippedAbilities);

   // Notify listeners (primarily UI waiting for input context to be applied before showing prompts)
   OnEquipToolFinished.Broadcast(this);
}

bool UToolComponent::OnUnequip_Implementation()
{
   check(CounterEquipped == 1);
   CounterEquipped--;

   // reset any temporarily stowed status whenever the tool equip status changes
   bool wasStowed = _counterStowed > 0;
   _counterStowed = 0;

   // Cancel the ability
   _CancelToolAbility();

   // remove equipped abilities
   ToolHelpers::RemoveAbilities(GetAbilitySystemComponent(), GrantedEquippedAbilities);

   // Exit this tool's input context, if it has one
   _ExitInputContext(ToolInput.InputContext);

   if (_isPlayingEquipAnim)
   {
      _isPlayingEquipAnim = false;
      _StopToolAnimation(EToolAnimation::Equip);
      GetOwner()->GetWorldTimerManager().ClearTimer(_timerHandle_OnEquipFinished);
   }

   // If the mesh is unusable on unequip and is already stowed, then we don't need to do any of this
   if (!wasStowed)
   {
      HideMesh();

      _PlayToolAnimation(EToolAnimation::Unequip, ToolAnimations.EquipPlaybackRate);

      // Unlink anim class layers after mesh is hidden and unequip animation begins
      _UnlinkAnimClassLayers();
   }

   // Call base class
   return IToolInterface::OnUnequip_Implementation();
}

void UToolComponent::SetStowed_Implementation(bool stowed)
{
   const bool oldStowState = _counterStowed > 0;

   // don't allow this to dip below 0, looks like there are some scenarios where this can happen currently because we don't block tool changes
   // in some places where maybe we should, so let's treat mismatched "unstow" requests gracefully
   _counterStowed = FMath::Max(0, _counterStowed + (stowed ? 1 : -1));
   const bool newStowState = _counterStowed > 0;

   // Don't play the animation or change mesh visibility if the Stow State hasn't changed
   if (oldStowState == newStowState)
   {
      return;
   }

   if (newStowState)
   {
      _PlayToolAnimation(EToolAnimation::Stow, ToolAnimations.StowPlaybackRate);
      HideMesh();
      _UnlinkAnimClassLayers();
   }
   else
   {
      _PlayToolAnimation(EToolAnimation::Unstow, ToolAnimations.StowPlaybackRate);
      ShowMesh();
      _LinkAnimClassLayers();
   }
}

// Checks that our ability has been fully replicated, locally controlled, and can be activated
bool UToolComponent::IsReady() const
{
   const UOSEAbilitySystemComponent* asc = _GetOSEAbilitySystemComponent();
   if (asc == nullptr)
   {
      // No ability system. We're only ready if we aren't using abilities anyways
      return (ToolAbilityClass == nullptr);
   }

   // We're only ready if we're locally controlled. Will be true in single player.
   if (!asc->IsLocallyControlled())
      return false;

   // Also make sure the asc's avatar is locally controlled: otherwise, we will not be able to activate the ToolAbility on it
   if (APawn* avatarPawn = Cast<APawn>(asc->GetAvatarActor()))
   {
      if (!avatarPawn->IsLocallyControlled())
      {
         return false;
      }
   }

   // requires any granted initial set abilities to be ready
   for (UOSEGameplayAbilitySet* abilitySet : InitialAbilitySets)
   {
      if (!abilitySet)
         continue;

      if (!abilitySet->IsReady(asc, this))
         return false;
   }

   // Ready if we're not using an ability
   if (!ToolAbilityClass)
      return true;

   // Look for our specific ability spec; this ensures all required data (source object)
   // has been set (or replicated)
   const FGameplayAbilitySpec* spec = asc->FindAbilitySpec(ToolAbilityClass, this);
   return (spec != nullptr);
}

bool UToolComponent::IsEquipped() const
{
   check(CounterEquipped >= 0);
   return CounterEquipped > 0;
}

void UToolComponent::_OnDestroyedWhileEquipped()
{
   // Handle the component being removed out from under itself while currently equipped
   // Other code-paths handle the authority-only stuff already, but there is local-client
   // things that have to be done (such as anim layers, etc)
   // The component isn't fully destroyed when this is called, and it isn't called when loading
   // so it should be about as safe as other calls to UnEquip in EndPlay, but we should still
   // make sure that implementations do not make bad assumptions.
   check(IsEquipped());
   IToolInterface::Execute_OnUnequip(this);
}

UToolSetComponent* UToolComponent::_GetOwningToolSet() const
{
   return _cachedOwningToolset.Get();
}

ACharacter* UToolComponent::GetOwnerCharacter() const
{
   return Cast<ACharacter>(GetOwner());
}

USkeletalMesh* UToolComponent::GetToolSkeletalMeshAssetFirstPerson() const
{
   if (ToolVisuals1P.MeshComponent != nullptr)
   {
      return ToolVisuals1P.MeshComponent->GetSkeletalMeshAsset();
   }
   return ToolVisuals1P.MeshAsset;
}

USkeletalMesh* UToolComponent::GetToolSkeletalMeshAssetThirdPerson() const
{
   if (ToolVisuals3P.MeshComponent != nullptr)
   {
      return ToolVisuals3P.MeshComponent->GetSkeletalMeshAsset();
   }
   return ToolVisuals3P.MeshAsset;
}

UAbilitySystemComponent* UToolComponent::GetAbilitySystemComponent() const
{
   if (const IAbilitySystemInterface* asi = Cast<IAbilitySystemInterface>(GetOwner()))
   {
      return asi->GetAbilitySystemComponent();
   }
   return nullptr;
}

// Plays an animation montage on the owner. Returns the length of the animation montage in seconds. Returns 0 if failed to play.
float UToolComponent::_PlayToolAnimation(EToolAnimation toolAnimation, float playbackRate)
{
   return _PlayToolAnimation(ToolAnimations.GetMontage(toolAnimation), playbackRate);
}

float UToolComponent::_PlayToolAnimation(class UAnimMontage* animMontage, float playbackRate)
{
   if (animMontage == nullptr)
      return 0.0f;

   ACharacter* ownerCharacter = GetOwnerCharacter();   
   if (ownerCharacter == nullptr)
      return 0.0f;

   if (UToolSetComponent* toolSetComponent = _cachedOwningToolset.Get())
   {
      if (toolSetComponent->IsSuppressingAnimations())
      {
         return 0.0f;
      }
   }

   return ownerCharacter->PlayAnimMontage(animMontage, playbackRate);
}

// Stops a playing tool animation
void UToolComponent::_StopToolAnimation(EToolAnimation toolAnimation)
{
   _StopToolAnimation(ToolAnimations.GetMontage(toolAnimation));
}

void UToolComponent::_StopToolAnimation(class UAnimMontage* animMontage)
{
   if (animMontage != nullptr)
   {
      if (ACharacter* ownerCharacter = GetOwnerCharacter())
      {
         ownerCharacter->StopAnimMontage(animMontage);
      }
   }
}

bool UToolComponent::_EnterInputContext(UInputMappingContext* context)
{
   if (!context)
      return false;

   if (UEnhancedInputLocalPlayerSubsystem* subsystem = _GetEnhancedInputLocalPlayerSubsystem())
   {
      subsystem->AddMappingContext(context, static_cast<int32>(EOSEEnhancedInputContextPriority::Items));

      // instant rebuild to ensure we can query the new control inputs right now.
      FModifyContextOptions opts;
      opts.bForceImmediately = true;
      opts.bIgnoreAllPressedKeysUntilRelease = true;
      subsystem->RequestRebuildControlMappings(opts);

      return true;
   }
   else
   {
      return false;
   }
}

void UToolComponent::_ExitInputContext(UInputMappingContext* context)
{
   if (!context)
      return;

   if (UEnhancedInputLocalPlayerSubsystem* subsystem = _GetEnhancedInputLocalPlayerSubsystem())
   {
      // We do want to force a rebuild, as things may want to use the base input immediately after this
      // If immediately switching to a new tool that also has an IMC, it may do slightly more work, but
      // that is likely acceptable
      FModifyContextOptions options;
      options.bForceImmediately = true;
      subsystem->RemoveMappingContext(context, options);
   }
}

void UToolComponent::_LinkAnimClassLayers()
{
   _LinkAnimClassLayers(ToolAnimations.AnimClassLayer);
}

void UToolComponent::_LinkAnimClassLayers(TSubclassOf<UAnimInstance> animClassLayer)
{
   if (animClassLayer != nullptr)
   {
      if (ACharacter* ownerCharacter = GetOwnerCharacter())
      {
         if (USkeletalMeshComponent* meshComponent = ownerCharacter->GetMesh())
         {
            meshComponent->LinkAnimClassLayers(animClassLayer);
         }
      }
   }
}

void UToolComponent::_UnlinkAnimClassLayers()
{
   _UnlinkAnimClassLayers(ToolAnimations.AnimClassLayer);
}

void UToolComponent::_UnlinkAnimClassLayers(TSubclassOf<UAnimInstance> animClassLayer)
{
   if (animClassLayer != nullptr)
   {
      if (ACharacter* ownerCharacter = GetOwnerCharacter())
      {
         if (USkeletalMeshComponent* meshComponent = ownerCharacter->GetMesh())
         {
            meshComponent->UnlinkAnimClassLayers(animClassLayer);
         }
      }
   }
}

// Returns ability system component
UOSEAbilitySystemComponent* UToolComponent::_GetOSEAbilitySystemComponent() const
{
   return Cast<UOSEAbilitySystemComponent>(GetAbilitySystemComponent());
}

/// Returns the local enhanced input subsystem, if we're local
class UEnhancedInputLocalPlayerSubsystem* UToolComponent::_GetEnhancedInputLocalPlayerSubsystem() const
{
   if (ACharacter* ownerCharacter = GetOwnerCharacter())
   {
      if (ownerCharacter->IsLocallyControlled())
      {
         if (APlayerController* pc = Cast<APlayerController>(ownerCharacter->Controller))
         {
            return ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer());
         }
      }
   }
   return nullptr;
}

FGameplayAbilitySpecHandle UToolComponent::_FindAbility() const
{
   if (const UOSEAbilitySystemComponent* asc = _GetOSEAbilitySystemComponent())
   {
      if (ToolAbilityClass != nullptr)
      {
         return asc->FindAbilitySpecHandle(ToolAbilityClass, this);
      }
   }

   // Not found
   return FGameplayAbilitySpecHandle();
}

bool UToolComponent::_GiveToolAbility()
{
   check(!GrantedAbilityHandle.IsValid());

   // Give the ability
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      if (asc->IsOwnerActorAuthoritative() && ToolAbilityClass != nullptr)
      {
         GrantedAbilityHandle = asc->GiveAbility(FGameplayAbilitySpec(ToolAbilityClass, 1, INDEX_NONE, this));
         return GrantedAbilityHandle.IsValid();
      }
   }

   return false;
}

bool UToolComponent::_ClearToolAbility()
{
   // Clear the ability
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      if (asc->IsOwnerActorAuthoritative() && GrantedAbilityHandle.IsValid())
      {
         asc->ClearAbility(GrantedAbilityHandle);
         GrantedAbilityHandle = FGameplayAbilitySpecHandle();
         return true;
      }
   }

   return false;
}

bool UToolComponent::_ActivateToolAbility()
{
   check(!ActivatedAbilityHandle.IsValid());

   if (UOSEAbilitySystemComponent* asc = _GetOSEAbilitySystemComponent())
   {
      // Make sure we are the ones who can initiate an ability
      if (!asc->HasAuthorityToActivateAbility(ToolAbilityClass))
         return false;

      // Find the ability
      ActivatedAbilityHandle = _FindAbility();
      if (ActivatedAbilityHandle.IsValid())
      {
         // Activate the ability
         if (asc->TryActivateAbility(ActivatedAbilityHandle, false))
         {
            // This can fail for a number of reasons, most often due to the
            // EGameplayAbilityNetExecutionPolicy for the ability. This is
            // totally fine, as this code may be called immediately on the
            // local client and then the server (or just the server if the
            // tool set system is not locally predicting)
            return true;
         }
      }
   }

   // No activated ability
   ActivatedAbilityHandle = FGameplayAbilitySpecHandle();
   return false;
}

bool UToolComponent::_CancelToolAbility()
{
   // Only cancel if we are the ones who activated it
   if (!ActivatedAbilityHandle.IsValid())
      return false;

   // Must have an ability system if we previously activated an ability
   UAbilitySystemComponent* asc = GetAbilitySystemComponent();
   check(asc != nullptr);

   // Simulate local input cancel
   asc->LocalInputCancel();

   // Cancel the ability and clear the handle
   asc->CancelAbilityHandle(ActivatedAbilityHandle);
   ActivatedAbilityHandle = FGameplayAbilitySpecHandle();
   return true;
}
