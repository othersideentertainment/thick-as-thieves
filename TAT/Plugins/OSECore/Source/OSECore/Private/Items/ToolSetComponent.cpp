// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#include "Items/ToolSetComponent.h"

#include "Items/ToolComponent.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/OSEGameplayAbility.h"
#include "Character/OSECharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "ComponentReregisterContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolSetComponent)


//---------------------------------------------------------------------------------------------------
// Tool set component class
//--------------------------------------------------------------------------------------------------

// Sets default values for this component's properties
UToolSetComponent::UToolSetComponent()
   : Super()
   , EnableLocalPrediction(true)
   , AutoEquipDefaultTool(false)
   , CheckForAutoEquip(false)
{
   // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
   // off to improve performance if you don't need them.
   PrimaryComponentTick.bCanEverTick = true;

   // Replicate and auto activate
   SetIsReplicatedByDefault(true);
   SetAutoActivate(true);
}

FString UToolSetComponent::GetReadableName() const
{
   return Super::GetReadableName() + TEXT(" ") + UEnum::GetValueAsString(GetOwnerRole());
}

// Begins play for this component
void UToolSetComponent::BeginPlay()
{
   Super::BeginPlay();

   // @TODO maybe an interface instead? Probably need an initialization interface with "match start" type events as well, BeginPlay is usually way too early in a networked game
   if (AOSECharacterBase* character = Cast<AOSECharacterBase>(GetOwner()))
   {
      // Need to wait for ability system to be initialized
      character->CallOrRegisterAbilitiesInitializedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UToolSetComponent::CreateDefaultTools));
   }
   else
   {
      // Just do it now
      CreateDefaultTools();
   }
}

// Ends play for this component
void UToolSetComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (endPlayReason == EEndPlayReason::Destroyed)
   {
      DestroyAllTools();
   }
   Super::EndPlay(endPlayReason);
}

#if WITH_EDITOR
void UToolSetComponent::PostLoad()
{
   Super::PostLoad();

   // migrate from old list to new struct
   // TODO: remove this once all affected assets have been re-saved
   if (DefaultToolClasses_DEPRECATED.Num() > 0 && DefaultTools.Num() == 0)
   {
      DefaultTools.Reserve(DefaultToolClasses_DEPRECATED.Num());
      for (TSubclassOf<UToolComponent> toolClass : DefaultToolClasses_DEPRECATED)
      {
         DefaultTools.Emplace_GetRef().ToolClass = toolClass;
      }
      DefaultToolClasses_DEPRECATED.Empty();
   }
}
#endif

// [Server] Creates default tools
void UToolSetComponent::CreateDefaultTools()
{
   // Check for auto equip when we create default tools
   CheckForAutoEquip = AutoEquipDefaultTool;

   // Server only
   if (GetOwnerRole() < ROLE_Authority)
      return;

   int32 toolsAdded = 0;

   if (ShouldCreateDefaultTools())
   {
      UE_LOG(LogTools, Log, TEXT("[%s] Creating default tools"), *GetReadableName());

      const UOSEAbilitySystemComponent* asc = GetAbilitySystemComponent();

      for (const FOSEDefaultToolEntry& toolEntry : DefaultTools)
      {
         TSubclassOf<UToolComponent> toolClass = toolEntry.ToolClass;

         if (!toolEntry.Requirements.IsMet(asc))
         {
            continue;
         }

         if (AuthorityAddToolClass(toolClass))
         {
            toolsAdded++;
         }
      }
   }

   // doing this in here rather than add-tool, so that non-starting tools do not count towards this
   _startingToolCountPlusOne = toolsAdded + 1;
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(UToolSetComponent, _startingToolCountPlusOne, this);
   #endif
   ensureMsgf(!_authorityHasCreatedDefaultTools,
      TEXT("ToolSet '%s' possibly called CreateDefaultTools twice"),
      *GetOwner()->GetName());

   if (!_authorityHasCreatedDefaultTools)
   {
      _authorityHasCreatedDefaultTools = true;
      _onAuthorityDefaultToolsCreatedDelegate.Broadcast();
      _onAuthorityDefaultToolsCreatedDelegate.Clear();
   }
}

// [Server] Removes all tools and destroys them
void UToolSetComponent::DestroyAllTools()
{
   // Server only
   if (GetOwnerRole() < ROLE_Authority)
      return;

   UE_LOG(LogTools, Log, TEXT("[%s] Destroying tool set"), *GetReadableName());

   // Unequip an tool if possible
   UnequipCurrentTool();

   // Remove all tools and destroy them
   for (int32 i = Tools.Num() - 1; i >= 0; i--)
   {
      UToolComponent* tool = Tools[i];
      if (tool)
      {
         RemoveTool(tool);
         tool->DestroyComponent();
      }
   }
}

// Returns ability system component
UOSEAbilitySystemComponent* UToolSetComponent::GetAbilitySystemComponent() const
{
   return UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(GetOwner());
}

bool UToolSetComponent::_CanToolBeEquipped(const UToolComponent* tool) const
{
   if (!IsValid(tool))
      return false;
   return true;
}

void UToolSetComponent::_CallOrRegisterAuthorityDefaultToolsCreatedDelegate(const FSimpleMulticastDelegate::FDelegate& toolsCreatedDelegate)
{
   check(GetOwner()->HasAuthority());

   if (_authorityHasCreatedDefaultTools)
   {
      toolsCreatedDelegate.Execute();
   }
   else
   {
      _onAuthorityDefaultToolsCreatedDelegate.Add(toolsCreatedDelegate);
   }
}

void UToolSetComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   #if UE_WITH_IRIS
   FDoRepLifetimeParams Params;
   Params.bIsPushBased = true;
   // Replicated to everyone
   DOREPLIFETIME_WITH_PARAMS_FAST(UToolSetComponent, Tools, Params);      // @TODO: Use COND_OwnerOnly?
   DOREPLIFETIME_WITH_PARAMS_FAST(UToolSetComponent, CurrentToolData, Params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UToolSetComponent, bIsEquipmentLocked, Params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UToolSetComponent, _startingToolCountPlusOne, Params); // Use COND_OwnerOnly? (what is the expected semantics for non-local for things like IsReady, etc?)   
   #else
   // Replicated to everyone
   DOREPLIFETIME(UToolSetComponent, Tools);      // @TODO: Use COND_OwnerOnly?
   DOREPLIFETIME(UToolSetComponent, CurrentToolData);
   DOREPLIFETIME(UToolSetComponent, bIsEquipmentLocked);
   DOREPLIFETIME(UToolSetComponent, _startingToolCountPlusOne); // Use COND_OwnerOnly? (what is the expected semantics for non-local for things like IsReady, etc?)
   #endif
}

// [Server + Local] Equips an existing tool from the set
void UToolSetComponent::EquipTool(UToolComponent* tool)
{
   if (bIsEquipmentLocked)
   {
      return;
   }

   const bool isAuthority = GetOwnerRole() == ROLE_Authority;
   // If looking at prediction issues, also check the SetLockEquipment method.
   const bool usePrediction = EnableLocalPrediction && (CurrentToolData.ToolObject != nullptr);

   if (usePrediction)
   {
      // Local prediction: Always send a server RPC to keep it
      // in sync if we're not the authority, then set the tool
      // locally for immediate response

      if (!isAuthority)
         ServerEquipTool(tool);

      SetCurrentTool(tool, CurrentToolData.ToolObject);
   }
   else
   {
      // No local prediction: Only set the tool if we're the
      // authority; otherwise send a server RPC and wait for
      // replication to propagate

      if (!isAuthority)
         ServerEquipTool(tool);
      else
         SetCurrentTool(tool, CurrentToolData.ToolObject);
   }
}

bool UToolSetComponent::ServerEquipTool_Validate(UToolComponent* tool)
{
   return true;
}

void UToolSetComponent::ServerEquipTool_Implementation(UToolComponent* tool)
{
   EquipTool(tool);
}

void UToolSetComponent::OnRep_Tools(const TArray<UToolComponent*>& oldTools)
{
   for (UToolComponent* newToolComp : Tools)
   {
      if (newToolComp != nullptr && !oldTools.Contains(newToolComp))
      {
         OnToolAdded.Broadcast(newToolComp);
         _OnToolAdded(newToolComp);
      }
   }

   // broadcast removes
   for (UToolComponent* oldToolComp : oldTools)
   {
      if (oldToolComp != nullptr && !Tools.Contains(oldToolComp))
      {
         OnToolRemoved.Broadcast(oldToolComp);
         _OnToolRemoved(oldToolComp);
      }
   }
}

void UToolSetComponent::OnRep_CurrentToolInfo(const FOSECurrentToolData& prevToolInfo)
{
   SetCurrentTool(CurrentToolData.ToolObject, prevToolInfo.ToolObject);
}

void UToolSetComponent::SetCurrentTool(UToolComponent* newTool, UToolComponent* prevTool /* = nullptr */)
{
   checkf(newTool == nullptr || !_isChangingEquippedTool,
      TEXT("Actor '%s' is calling SetCurrentTool to equip tools in a re-entrant manner: this is not supported, and will likely cause bugs. ")
      TEXT("newTool = '%s', prevTool = '%s', CurrentToolData.ToolObject = '%s'"),
      *GetOwner()->GetName(),
      *GetNameSafe(newTool),
      *GetNameSafe(prevTool),
      *GetNameSafe(CurrentToolData.ToolObject));

   TGuardValue<bool> guardIsEquippingTool(_isChangingEquippedTool, true);

   UToolComponent* localPrevTool = nullptr;

   if (prevTool != nullptr)
   {
      localPrevTool = prevTool;
   }
   else if (newTool != CurrentToolData.ToolObject)
   {
      localPrevTool = CurrentToolData.ToolObject;
   }

   // Whether or not this is a different tool
   const bool isDifferentTool = newTool != localPrevTool;

   // Unequip previoustool
   if (localPrevTool != nullptr && isDifferentTool && localPrevTool->IsEquipped())
   {
      UE_LOG(LogTools, Log, TEXT("[%s] Unequipping tool `[%s]` in tool set")
         , *GetReadableName()
         , *localPrevTool->GetName());

      IToolInterface::Execute_OnUnequip(localPrevTool);
   }

   CurrentToolData.ToolObject = newTool;
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(UToolSetComponent, CurrentToolData, this);
   #endif
   // Equip new tool
   if (newTool != nullptr && isDifferentTool && !newTool->IsEquipped())
   {
      UE_LOG(LogTools, Log, TEXT("[%s] Equipping tool `[%s]` in tool set")
         , *GetReadableName()
         , *newTool->GetName());

      // Bind to new tool's equip-finish delegate
      newTool->OnEquipToolFinished.AddDynamic(this, &UToolSetComponent::_OnEquipToolFinished);

      IToolInterface::Execute_OnEquip(newTool, localPrevTool);
   }

   // an event that UI can bind to for the hotbar
   if (isDifferentTool)
   {
      OnEquippedToolChanged.Broadcast();

      // Unbind from previous tool's equip-finish delegate
      if (localPrevTool != nullptr)
      {
         localPrevTool->OnEquipToolFinished.RemoveAll(this);
      }
   }
}

// [Server] Add tool to tool set
bool UToolSetComponent::AddTool(UToolComponent* tool)
{
   // Server only
   if (GetOwnerRole() < ROLE_Authority)
      return false;

   if (tool != nullptr)
   {
      UE_LOG(LogTools, Log, TEXT("[%s] Adding tool `[%s]` to tool set")
         , *GetReadableName()
         , *tool->GetName());

      Tools.AddUnique(tool);

      IToolInterface::Execute_OnAddToToolSet(tool);

      OnToolAdded.Broadcast(tool);
      _OnToolAdded(tool);

#if UE_WITH_IRIS
      MARK_PROPERTY_DIRTY_FROM_NAME(UToolSetComponent, Tools, this);
#endif

      return true;
   }

   return false;
}

// [Server] Remove tool from tool set
void UToolSetComponent::RemoveTool(UToolComponent* tool)
{
   // Server only
   if (GetOwnerRole() < ROLE_Authority)
      return;

   if (tool != nullptr)
   {
      UE_LOG(LogTools, Log, TEXT("[%s] Removing tool `[%s]` from tool set")
         , *GetReadableName()
         , *tool->GetName());

      IToolInterface::Execute_OnRemoveFromToolSet(tool);

      bool removed = Tools.RemoveSingle(tool) > 0;

      if (removed)
      {
         tool->OnEquipToolFinished.RemoveAll(this);

         OnToolRemoved.Broadcast(tool);
         _OnToolRemoved(tool);

#if UE_WITH_IRIS
         MARK_PROPERTY_DIRTY_FROM_NAME(UToolSetComponent, Tools, this);
#endif
      }
   }
}

// IToolSetSystemInterface
TScriptInterface<IToolSetInterface> UToolSetComponent::GetToolSetInterface() const
{
   // Cast is needed only to satisfy blueprint code gen when returning an interface from a blueprint function
   return const_cast<UToolSetComponent*>(this);
}

bool UToolSetComponent::IsReady() const
{
   if (_startingToolCountPlusOne == 0 || (Tools.Num() + 1 < _startingToolCountPlusOne))
      return false;

   for (UToolComponent* tool : Tools)
   {
      if (tool == nullptr)
         return false;

      if (!tool->IsReady())
         return false;
   }

   // All tools are ready
   return true;
}

int32 UToolSetComponent::GetNumTools() const
{
   return Tools.Num();
}

bool UToolSetComponent::IsEquipmentLocked() const
{
   return bIsEquipmentLocked;
}

void UToolSetComponent::LockEquipmentByClass(const TSubclassOf<UToolComponent> toolClass)
{
   if (toolClass)
   {
      EquipToolByClass(toolClass);
   }
   else
   {
      UnequipCurrentTool();
   }
   SetLockEquipment(true);
}

void UToolSetComponent::LockEquipment(bool bUnequipCurrentTool/* = false*/)
{
   if (bUnequipCurrentTool)
   {
      UnequipCurrentTool();
   }

   SetLockEquipment(true);
}

void UToolSetComponent::UnlockEquipment()
{
   SetLockEquipment(false);
}

void UToolSetComponent::SetLockEquipment(bool lockEquipment)
{
   const bool isAuthority = GetOwnerRole() == ROLE_Authority;
   // If looking at prediction issues, also check the EquipTool method.
   const bool usePrediction = EnableLocalPrediction && (CurrentToolData.ToolObject != nullptr);

   if (usePrediction)
   {
      // Local prediction: Always send a server RPC to keep it
      // in sync if we're not the authority, then set the lock
      // locally for immediate response

      if (!isAuthority)
         ServerSetLockEquipment(lockEquipment);

      InternalSetLockEquipment(lockEquipment);
   }
   else
   {
      // No local prediction: Only set the lock if we're the
      // authority; otherwise send a server RPC and wait for
      // replication to propagate

      if (!isAuthority)
         ServerSetLockEquipment(lockEquipment);
      else
         InternalSetLockEquipment(lockEquipment);
   }
}

void UToolSetComponent::ServerSetLockEquipment_Implementation(bool lockEquipment)
{
   InternalSetLockEquipment(lockEquipment);
}

void UToolSetComponent::InternalSetLockEquipment(bool bLockEquipment)
{
   bIsEquipmentLocked = bLockEquipment;
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(UToolSetComponent, bIsEquipmentLocked, this);
   #endif
}

UToolComponent* UToolSetComponent::GetCurrentTool() const
{
   return CurrentToolData.ToolObject;
}

int32 UToolSetComponent::GetIndexOfCurrentTool() const
{
   if (CurrentToolData.ToolObject)
   {
      return Tools.Find(CurrentToolData.ToolObject);
   }
   return INDEX_NONE;
}

FGameplayTag UToolSetComponent::GetCurrentToolCategory() const
{
   if (CurrentToolData.ToolObject)
   {
      return CurrentToolData.ToolObject->GetToolInfo().ToolCategory;
   }
   return FGameplayTag::EmptyTag;
}

UToolComponent* UToolSetComponent::GetToolAtIndex(int32 toolIndex) const
{
   if (Tools.IsValidIndex(toolIndex))
   {
      return Tools[toolIndex];
   }
   return nullptr;
}

UToolComponent* UToolSetComponent::GetToolByCategory(const FGameplayTag& toolCategory, bool matchesExact) const
{
   for (UToolComponent* toolComponent : Tools)
   {
      if (!toolComponent)
         continue;

      FGameplayTag toolInfoCategory = toolComponent->GetToolInfo().ToolCategory;
      const bool matches = matchesExact ? toolInfoCategory.MatchesTagExact(toolCategory) : toolInfoCategory.MatchesTag(toolCategory);
      if (matches)
      {
         return toolComponent;
      }
   }
   return nullptr;
}

UToolComponent* UToolSetComponent::GetToolByClass(TSubclassOf<UToolComponent> toolClass, bool matchesExact) const
{
   for (UToolComponent* tool : Tools)
   {
      if (!tool)
         continue;

      if (tool->GetClass() == toolClass || (!matchesExact && tool->GetClass()->IsChildOf(toolClass)))
         return tool;
   }
   return nullptr;
}

TArray<UToolComponent*> UToolSetComponent::FindToolsInCategories(const FGameplayTagContainer& toolCategories) const
{
   TArray<UToolComponent*> tools;
   for(UToolComponent* toolComponent : Tools)
   {
      if (!toolComponent)
         continue;

      if (toolComponent->GetToolInfo().ToolCategory.MatchesAny(toolCategories))
      {
         tools.Add(toolComponent);
      }
   }
   return tools;
}

bool UToolSetComponent::HasToolClass(TSubclassOf<UToolComponent> toolClass) const
{
   for (UToolComponent* tool : Tools)
   {
      if (!tool)
         continue;

      if (tool->GetClass() == toolClass || tool->GetClass()->IsChildOf(toolClass))
         return true;
   }
   return false;
}

bool UToolSetComponent::HasToolCategory(const FGameplayTag& toolCategory) const
{
   for (UToolComponent* tool : Tools)
   {
      if (!tool)
         continue;

      if (tool->GetToolInfo().ToolCategory.MatchesTag(toolCategory))
         return true;
   }
   return false;
}

bool UToolSetComponent::HasToolEquipped(const FGameplayTag& toolCategory) const
{
   return CurrentToolData.ToolObject &&
      CurrentToolData.ToolObject->IsReady() &&
         CurrentToolData.ToolObject->IsPlayingEquipAnimation() == false &&
            CurrentToolData.ToolObject->GetToolInfo().ToolCategory.MatchesTag(toolCategory);
}

bool UToolSetComponent::EquipToolByClass(TSubclassOf<UToolComponent> toolClass)
{
   // TODO: should this be exact class, or allow subclasses?
   UToolComponent* tool = _FindToolOfExactClass(toolClass);
   return _TryEquipTool(tool);
}

bool UToolSetComponent::EquipToolByCategory(const FGameplayTag& toolCategory, bool matchesExact)
{
   UToolComponent* tool = GetToolByCategory(toolCategory, matchesExact);
   return _TryEquipTool(tool);
}

bool UToolSetComponent::UnequipCurrentTool()
{
   bool isEquipped = CurrentToolData.ToolObject != nullptr;
   EquipTool(nullptr);
   return isEquipped;
}

bool UToolSetComponent::UnequipCurrentToolByCategory(const FGameplayTag& toolCategory, bool matchesExact)
{
   if (CurrentToolData.ToolObject == nullptr) return false;

   FGameplayTag toolInfoCategory = CurrentToolData.ToolObject->GetToolInfo().ToolCategory;
   const bool matches = matchesExact ? toolInfoCategory.MatchesTagExact(toolCategory) : toolInfoCategory.MatchesTag(toolCategory);
   if (matches)
   {
      UnequipCurrentTool();
      return true;
   }
   return false;
}

UToolComponent* UToolSetComponent::AuthorityCreateToolClass(TSubclassOf<UToolComponent> toolClass)
{
   if (toolClass)
   {
      return NewObject<UToolComponent>(GetOwner(), toolClass);
   }
   return nullptr;
}

bool UToolSetComponent::AuthorityAddToolClass(TSubclassOf<UToolComponent> toolClass)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogTools, Error, TEXT("[%s] AuthorityAddToolClass requires authority!"), *GetReadableName());
      return false;
   }

   if (toolClass == nullptr)
   {
      UE_LOG(LogTools, Error, TEXT("[%s] AuthorityAddToolClass requires a tool class!"), *GetReadableName());
      return false;
   }

   UToolComponent* spawnedTool = AuthorityCreateToolClass(toolClass);
   if (!_ValidateSpawnedTool(spawnedTool))
   {
      if (spawnedTool)
      {
         spawnedTool->DestroyComponent();
      }
      return false;
   }

   // passed validation; register the component w/ our actor owner
   spawnedTool->RegisterComponent();

   // this should be a guaranteed success at this point, we already validated everything we'd need to...?
   const bool addSuccess = AddTool(spawnedTool);
   check(addSuccess);
   return addSuccess;
}

bool UToolSetComponent::AuthorityAddTool(UToolComponent* toolComponent)
{
   if (!GetOwner()->HasAuthority())
   {
      if (toolComponent)
      {
         toolComponent->DestroyComponent();
      }
      UE_LOG(LogTools, Error, TEXT("[%s] AuthorityAddTool requires authority!"), *GetReadableName());
      return false;
   }

   if (!_ValidateSpawnedTool(toolComponent))
   {
      if (toolComponent)
      {
         toolComponent->DestroyComponent();
      }
      return false;
   }

   // passed validation; register the component w/ our actor owner
   toolComponent->RegisterComponent();

   // this should be a guaranteed success at this point, we already validated everything we'd need to...?
   const bool addSuccess = AddTool(toolComponent);
   check(addSuccess);
   return addSuccess;
}

int UToolSetComponent::AuthorityRemoveToolsOfClass(TSubclassOf<UToolComponent> toolClass)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogTools, Error, TEXT("[%s] AuthorityRemoveToolsOfClass requires authority!"), *GetReadableName());
      return 0;
   }

   if (toolClass == nullptr)
   {
      UE_LOG(LogTools, Error, TEXT("[%s] AuthorityRemoveToolsOfClass requires a tool class!"), *GetReadableName());
      return 0;
   }

   // NOTE: Currently we only allow 1 tool per class so this will only remove one thing,
   // but I'll write the code so it'll remove all tools of this class when called if we expand that restriction
   // which seems like it may becoming down the pipe...
   int32 numToolsRemoved = 0;
   for (int32 j = Tools.Num() - 1; j >= 0; j--)
   {
      UToolComponent* tool = Tools[j];
      if (tool && tool->GetClass() == toolClass)
      {
         RemoveTool(tool);
         tool->DestroyComponent();
         ++numToolsRemoved;
      }
   }
   return numToolsRemoved;
}

int UToolSetComponent::AuthorityRemoveToolsOfCatagory(const FGameplayTag& toolCategory, bool matchesExact)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogTools, Error, TEXT("[%s] AuthorityRemoveToolsOfClass requires authority!"), *GetReadableName());
      return 0;
   }
   int32 numToolsRemoved = 0;
   for (int32 j = Tools.Num() - 1; j >= 0; j--)
   {
      UToolComponent* toolComponent = Tools[j];

      if (!toolComponent)
         continue;

      FGameplayTag toolInfoCategory = toolComponent->GetToolInfo().ToolCategory;
      const bool matches = matchesExact ? toolInfoCategory.MatchesTagExact(toolCategory) : toolInfoCategory.MatchesTag(toolCategory);
      if (matches)
      {
         RemoveTool(toolComponent);
         toolComponent->DestroyComponent();
         ++numToolsRemoved;
      }
   }
   return numToolsRemoved;
}

void UToolSetComponent::StowCurrentTool(bool stowed)
{
   if (UToolComponent* tool = GetCurrentTool())
   {
      IToolInterface::Execute_SetStowed(tool, stowed);
   }
}

// Called every frame
void UToolSetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
   Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

   // Only local net owners (those who have control over this component, such as client players)
   // should check for auto equipping an tool
   if (CheckForAutoEquip && (GetOwner()->HasLocalNetOwner()))
   {
      // Once tool set is fully ready, we no longer check for auto equipping
      if (IsReady())
      {
         CheckForAutoEquip = false;

         if (Tools.IsValidIndex(0))
         {
            EquipTool(Tools[0]);
         }
      }
   }
}

bool UToolSetComponent::_TryEquipTool(UToolComponent* tool)
{
   if (tool && tool != CurrentToolData.ToolObject && _CanToolBeEquipped(tool))
   {
      EquipTool(tool);
      return true;
   }

   return false;
}

UToolComponent* UToolSetComponent::_FindToolOfExactClass(TSubclassOf<UToolComponent> toolClass) const
{
   for (UToolComponent* tool : Tools)
   {
      if (tool && tool->GetClass() == toolClass)
      {
         return tool;
      }
   }

   return nullptr;
}

void UToolSetComponent::_OnEquipToolFinished(const UToolComponent* tool)
{
   check(tool != nullptr);
   check(tool == CurrentToolData.ToolObject);

   UE_LOG(LogTools, Log, TEXT("Tool %s finished equipping"), *tool->GetName());
   OnEquipToolFinished.Broadcast(tool);
}

bool UToolSetComponent::_ValidateSpawnedTool(UToolComponent* spawnedTool) const
{
   // Make sure we spawned it
   if (spawnedTool == nullptr)
   {
      UE_LOG(LogTools, Error, TEXT("[%s] Spawned tool is null!")
         , *GetReadableName());

      return false;
   }

   // Make sure it's unique
   if (HasToolClass(spawnedTool->GetClass()))
   {
      UE_LOG(LogTools, Error, TEXT("[%s] Duplicate tool class `%s`!")
         , *GetReadableName()
         , *spawnedTool->GetClass()->GetName());

      return false;
   }

   // Make sure it replicates
   if (!spawnedTool->GetIsReplicated())
   {
      UE_LOG(LogTools, Error, TEXT("[%s] tool `[%s]` must be replicated!")
         , *GetReadableName()
         , *spawnedTool->GetName());

      return false;
   }

   // Make sure it's owner is our owner
   if (spawnedTool->GetOwner() != GetOwner())
   {
      UE_LOG(LogTools, Error, TEXT("[%s] tool `[%s]` with owner `[%s]`, but it should be our owner `[%s!]`")
         , *GetReadableName()
         , *spawnedTool->GetName()
         , *spawnedTool->GetOwner()->GetHumanReadableName()
         , *GetOwner()->GetHumanReadableName());

   }

   // passed validation!
   return true;
}

bool FOSEDefaultToolRequirements::IsMet(const UOSEAbilitySystemComponent* asc) const
{
   return !RequiredUpgradeTag.IsValid() || (asc && asc->GetUpgradeValue(RequiredUpgradeTag, 0) >= RequiredUpgradeLevel);
}

