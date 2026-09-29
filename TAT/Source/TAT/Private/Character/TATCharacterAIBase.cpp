// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Character/TATCharacterAIBase.h"

// tat
#include "Abilities/TATAttributeSet.h"
#include "Abilities/TATGameplayTags.h"
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"
#include "AI/TATAIStateWorldSubsystem.h"
#include "AI/TATKnowledgeComponent.h"
#include "AI/Alertness/TATAlertnessComponent.h"
#include "AI/Escalation/TATEscalationComponent.h"
#include "AI/LivingWorld/TATLivingWorldAgentComponent.h"
#include "Animation/TATCharacterAnimationMapping.h"
#include "Character/TATCharacterAIMovement.h"
#include "Collision/TATCollisionUtils.h"
#include "Collision/Overlay/CollisionOverlayCapsuleComponent.h"
#include "Combat/TATAICombatComponent.h"
#include "Combat/TATCombatFunctionLibrary.h"
#include "Combat/TATCombatSettings.h"
#include "Developer/TATProjectSettings.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Items/TATItemInfo.h"
#include "Items/TATItemInventoryComponent.h"
#include "Items/TATPickpocketableComponent.h"
#include "Online/TATGameState.h"
#include "Player/TATCharacter.h"
#include "Tools/TATToolSetComponent.h"
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Environment/TATPrivateSpaceGameplayTagDefines.h"
#include "Loot/TATLootInventory.h"
#include "Items/TATItemActor.h"
#include "AI/TATIndividualAttitudeComponent.h"
#include "AI/StateTrees/TATStateTreeEvents.h"
#include "Variation/Clues/TATNPCClueComponent.h"
#include "Variation/Clues/TATNPCClueSpawnerComponent.h"

// ose
#include "OSECoreCheats.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "AI/OSEAIFunctionLibrary.h"
#include "AI/OSEAISettings.h"
#include "VoiceOver/OSEVoiceOverParticipantSubsystem.h"
#include "Detection/OSEDetectionComponent.h"
#include "Camera/OSECameraUtils.h"

//ue4
#include "GameplayEffectExtension.h"
#include "MotionWarpingComponent.h"
#include "OSELightDetectionFunctionLibrary.h"
#include "AI/Perception/AISense_VisualEvent.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Damage.h"
#include "TimerManager.h"
#include "Engine/AssetManager.h"
#include "Significance/TATSignificanceBasedTick.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterAIBase)

DEFINE_LOG_CATEGORY(LogTATCharacterAIBase);

DECLARE_STATS_GROUP(TEXT("TATCharacterAIBase"), STATGROUP_TATCharacterAIBase, STATCAT_Advanced);

static TAutoConsoleVariable<int32> CVarTATUIDetectionVisible(
    TEXT("tat.ui.hidedetection"),
    0,
    TEXT("Force hide the detection UI if value is 1\n")
    TEXT("0: Visible\n")
    TEXT("1: Hidden\n"),
    ECVF_Default);

ATATCharacterAIBase::ATATCharacterAIBase(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer
           .SetDefaultSubobjectClass<UTATCharacterAIMovement>(ACharacter::CharacterMovementComponentName)
           .SetDefaultSubobjectClass<UCollisionOverlayCapsuleComponent>(ACharacter::CapsuleComponentName)
           .SetDefaultSubobjectClass<UTATAICombatComponent>(AOSECharacterBase::CombatComponentName)
           .SetDefaultSubobjectClass<UTATItemInventoryComponent>(AOSECharacterBase::ItemInventoryComponentName)
           .SetDefaultSubobjectClass<UTATToolSetComponent>(AOSECharacterBase::ToolSetComponentName)
           .SetDefaultSubobjectClass<UOSEAbilitySystemComponent>(TEXT("AbilitySystemComponent"))
           .SetDefaultSubobjectClass<UAttributeBaseSet>(TEXT("BaseAttributeSet"))
           .SetDefaultSubobjectClass<UItemInventoryComponent>(AOSECharacterBase::ItemInventoryComponentName)
           .DoNotCreateDefaultSubobject(AOSEPlayerCharacter1P::ComponentName_Mesh1P_LowerBody)
           .DoNotCreateDefaultSubobject(AOSEPlayerCharacter1P::ComponentName_Mesh1P_UpperBody)
           .DoNotCreateDefaultSubobject(FCameraComponentName::ParentXfm)
           .DoNotCreateDefaultSubobject(FCameraComponentName::Component)
           )
{
   // Don't replicate gameplay effects to non-owning players
   // If there is an actual use-case for player possession, it can be changed to Mixed
   GetAbilitySystemComponent()->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

   _tatItemInventory = Cast<UTATItemInventoryComponent>(ItemInventoryComponent);

   PickpocketableComponentClass = UTATPickpocketableComponent::StaticClass();

   // 3/3/22 -- let's try only ticking the mesh when it's rendered or
   // there's a montage playing, now that footsteps/stims are no longer generated via anims
   // NOTE: The blueprints downstream from here have heads / outfit parts that I set this on
   // by-hand (specifically the guard/civilian subclasses which are made up of various mesh parts)
   GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
   GetMesh3P_Head()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
   
   _motionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
   _tatLootInventory = CreateDefaultSubobject<UTATLootInventoryComponent>(TEXT("TATLootInventoryComponent"));

   _tatAttributes = CreateDefaultSubobject<UTATAttributeSet>(TEXT("TATAttributeSet"));

   _privateSpaceCharacterComponent = CreateDefaultSubobject<UTATPrivateSpaceCharacterComponent>(TEXT("PrivateSpaceComponent"));
   if (UCharacterMovementComponent* characterMovementComp = Cast<UCharacterMovementComponent>(GetCharacterMovement()))
   {
      // This smoothly rotates the Character toward the Controller's desired rotation.
      // This is typically Controller->ControlRotation, but our AIController implementation
      // overrides GetDesiredRotation to provide a separate "body" orientation.
      characterMovementComp->bUseControllerDesiredRotation = true;
      characterMovementComp->DefaultLandMovementMode = MOVE_NavWalking;
   }

   // Modifies SprintRequest() behavior so that AI's treat a SprintRequest()/Cancel as a true toggle state.
   _sprintInputBehavior = EOSESprintInputBehavior::SprintWhileRequested;

   _alertnessComponent = CreateDefaultSubobject<UTATAlertnessComponent>(TEXT("TATAlertnessComponent"));
   _detectionComponent = CreateDefaultSubobject<UOSEDetectionComponent>(TEXT("DetectionComponent"));
   _individualAttitudeComponent = CreateDefaultSubobject<UTATIndividualAttitudeComponent>(TEXT("IndividualAttitudeComponent"));

   // We don't use these camera sockets for AI, so clear them out so it doesn't warn
   CameraSocket.Name = NAME_None;
   GameCameraSocket.Name = NAME_None;

   // Set a non-null default.
   _clueComponentClass = UTATNPCClueComponent::StaticClass();

   _asyncRequestComponent = CreateDefaultSubobject<UTATAsyncRequestComponent>(TEXT("AsyncRequestComponent"));

   SetNetUpdateFrequency(15.0f); // Half of NetServerMaxTickRate, so polled every other frame
   SetNetCullDistanceSquared(FMath::Square(10'000));
}

void ATATCharacterAIBase::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   // bind to our own alertness level changing
   if (_alertnessComponent)
   {
      _alertnessComponent->OnAlertnessLevelChanged.AddUniqueDynamic(this, &ATATCharacterAIBase::_OnAlertnessLevelChanged);
   }

   // not every AI character will have this - the Civilians may add this in blueprint.
   _tatLivingWorldAgentComponent = FindComponentByClass<UTATLivingWorldAgentComponent>();

   GetAbilitySystemComponent()->AddLooseGameplayTag(WealthClass);
}

void ATATCharacterAIBase::BeginPlay()
{
   Super::BeginPlay();

   _defaultTimeDilation = CustomTimeDilation;

   if (ATATGameState* gs = ATATGameState::GetTATGameState(this))
   {
      gs->OnGameFrozenChanged.AddUniqueDynamic(this, &ATATCharacterAIBase::_OnIsGameFrozenChanged);
      _OnIsGameFrozenChanged(gs->IsGameFrozen());
   }

   if (UTATAIStateWorldSubsystem* aiStateWorldSubsystem = GetWorld()->GetSubsystem<UTATAIStateWorldSubsystem>())
   {
      aiStateWorldSubsystem->RegisterAICharacter(this);
   }

   if (UOSEVoiceOverParticipantSubsystem* voSubsystem = GetWorld()->GetSubsystem<UOSEVoiceOverParticipantSubsystem>())
   {
      voSubsystem->RegisterParticipant(this);
   }

   _detectionComponent->OnLocalPlayerDetectionValueChanged.BindUObject(this, &ThisClass::_OnLocalPlayerDetectionValueChanged);

   // Trigger initial escalation state delegates to catch initially replicated variables
   _BroadcastEscalationStateChanged();
   
   UTATSignificanceBasedTickConfig::RegisterComponent(GetCharacterMovement(), _movementSignificanceTickConfig);
   
   if (HasAuthority())
   {
      if (_ShouldCheckIfOverlappingWorldStaticGeometry)
      {
         GetWorld()->GetTimerManager().SetTimer(
            _CheckOverlappingWorldStaticGeometryHandle,
            this,
            &ThisClass::_HandleCheckOverlappingWorldStaticGeometry,
            _TimeBetweenOverlappingWorldStaticGeometryChecks,
            true);
      }
   }
   
   // If either a listen server or standalone or a client of a dedicated server.
   if (HasAuthority() == false || (GetNetMode() != NM_DedicatedServer))
   {
      if (UCharacterMovementComponent* characterMovementComp = Cast<UCharacterMovementComponent>(GetCharacterMovement()))
      {
         if (characterMovementComp->DefaultLandMovementMode != MOVE_Flying)
         {
            characterMovementComp->DefaultLandMovementMode = MOVE_Walking;
         }
      }
   }
}

void ATATCharacterAIBase::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   UTATSignificanceBasedTickConfig::Unregister(GetCharacterMovement());
   if (_CheckOverlappingWorldStaticGeometryHandle.IsValid())
   {
      _CheckOverlappingWorldStaticGeometryHandle.Invalidate();
   }
   if (UOSEVoiceOverParticipantSubsystem* voSubsystem = GetWorld()->GetSubsystem<UOSEVoiceOverParticipantSubsystem>())
   {
      voSubsystem->UnregisterParticipant(this);
   }

   Super::EndPlay(endPlayReason);
}

void ATATCharacterAIBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATCharacterAIBase, _unconsciousEffectInfo);
   DOREPLIFETIME(ATATCharacterAIBase, _escalationState);

   DOREPLIFETIME(ATATCharacterAIBase, _pickpocketableComponent);
}

#if WITH_EDITOR
EDataValidationResult ATATCharacterAIBase::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (_characterAnimationMapping != nullptr)
   {
      const int32 numErrors = _characterAnimationMapping->ValidateForCharacter(this, context);
      if (numErrors > 0)
      {
         result = EDataValidationResult::Invalid;
      }
   }

   if (_clueComponentClass.IsNull())
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no clue component class set!"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif

void ATATCharacterAIBase::InitializeAbilities(UOSEAbilitySystemComponent* inComponent, UAttributeBaseSet* inAttributeSet)
{
   if (bAreAbilitiesInitialized)
      return;

   Super::InitializeAbilities(inComponent, inAttributeSet);
}

void ATATCharacterAIBase::_OnLocalPlayerDetectionValueChanged(float oldDetectionValue, float newDetectionValue) const
{
   if (oldDetectionValue <= 0.0f && newDetectionValue > 0.0f)
   {
      OnLocalTATCharacterFirstDetected.Broadcast();
   }
}

void ATATCharacterAIBase::PossessedBy(AController* newController)
{
   Super::PossessedBy(newController);

   _possessedController = newController;

   if(const ATATAIController* aiController = Cast<ATATAIController>(newController))
   {
      if(UTATEscalationComponent* escalationComponent = aiController->GetTATEscalationComponent())
      {
         escalationComponent->OnEscalationStateChanged.AddUniqueDynamic(this, &ATATCharacterAIBase::_AuthorityHandleEscalationStateChanged);
         _AuthorityHandleEscalationStateChanged(escalationComponent->GetCurrentState());
      }
   }
}

void ATATCharacterAIBase::UnPossessed()
{
   _possessedController = nullptr;

   Super::UnPossessed();
}

UOSEIndividualAttitudeComponent* ATATCharacterAIBase::GetAttitudeComponent() const
{
   return _individualAttitudeComponent;
}

uint8 ATATCharacterAIBase::GetTeam() const
{
   if (HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_HOSTILEINTRUDER))
   {
      return UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Intruder);
   }
   return GetOriginalTeam();
}

uint8 ATATCharacterAIBase::GetOriginalTeam() const
{   
   return UTATProjectSettings::GetTeamAssignmentForCharacterType(TeamCharacter);
}

TSoftObjectPtr<UAnimMontage> ATATCharacterAIBase::GetIdleAnimMontage() const
{
   //TODO: Maybe we want to ensure we don't play the same idle anim repeatedly? Should we start to track which was last
   //played? If so, we can extend this by taking into account the last anim index played, removing it from the "valid" indexes
   //for now, this should be good enough for a proof of concept. We may also want to take into account the random steam from the level
   //but that seems like overkill right now - DG

   // Should we be using a map to escalation states? Or instead have a struct with animation montages + valid gameplay tag containers
   // then gather all valid animations and randomize off of that?
   const FTATCharacterAIIdleAnimations* idleAnimations = _idleMontages.Find(GetCurrentEscalationState());
   if(idleAnimations == nullptr)
      return nullptr;
   const int animIndexToPlay = FMath::RandRange(0,idleAnimations->Animations.Num()-1);
   if(idleAnimations->Animations.IsValidIndex(animIndexToPlay) == false)
      return nullptr;
   return idleAnimations->Animations[animIndexToPlay];
}

TSoftObjectPtr<UAnimMontage> ATATCharacterAIBase::GetSearchAnimMontage() const
{
   return GetSearchAnimMontageFromType(ETATCharacterAISearchAnimType::SameLevel);
}

TSoftObjectPtr<UAnimMontage> ATATCharacterAIBase::GetSearchAnimMontageFromType(const ETATCharacterAISearchAnimType animType) const
{
   const FTATCharacterAISearchAnimationSet* searchAnimation = _searchMontages.Find(GetCurrentEscalationState());
   if(searchAnimation == nullptr)
      return nullptr;
   const FTATCharacterAISearchAnimations* animsToUse = searchAnimation->AnimationMap.Find(animType);
   if(animsToUse == nullptr)
      return nullptr;
   const int animIndexToPlay = FMath::RandRange(0,animsToUse->Animations.Num()-1);
   if(animsToUse->Animations.IsValidIndex(animIndexToPlay) == false)
      return nullptr;
   return animsToUse->Animations[animIndexToPlay];
}

void ATATCharacterAIBase::SprintRequest()
{
   // Block additional sprint requests.
   if(SprintRequestCounter > 0)
      return;
   Super::SprintRequest();
}

void ATATCharacterAIBase::SprintCancel()
{
   Super::SprintCancel();
}

void ATATCharacterAIBase::GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const
{
   Super::GetOwnedGameplayTags(tagContainer);
   // Smart objects may need to know if the owner is allowed in a certain private area when doing their queries.
   if(_privateSpaceCharacterComponent)
   {
      const FGameplayTagContainer& allowedPrivateZones = _privateSpaceCharacterComponent->AuthorityGetAllAllowedPrivateZone();
      tagContainer.AppendTags(allowedPrivateZones);
   }
}

void ATATCharacterAIBase::CreateNPCClueComponent(const UTATNPCClueSpawnerComponent* spawner)
{
   check(spawner != nullptr);

   if (!ensureMsgf(_clueComponent == nullptr, TEXT("[%s] CreateNPCClueComponent was called when AI already had a clue component!"),
      *GetName()))
   {
      return;
   }

   if (!ensureMsgf(!_clueComponentClass.IsNull(), TEXT("[%s] CreateNPCClueComponent was called when no clue component class was set!"),
      *GetName()))
   {
      return;
   }
   
   UAssetManager::GetStreamableManager().RequestAsyncLoad(_clueComponentClass.ToSoftObjectPath(), 
      [weakThis = MakeWeakObjectPtr(this), weakSpawner = MakeWeakObjectPtr(spawner)]()
   {
      ATATCharacterAIBase* character = weakThis.Get();
      const UTATNPCClueSpawnerComponent* spawner = weakSpawner.Get();
      if (character == nullptr || spawner == nullptr)
      {
         return;
      }

      constexpr bool manualAttachment = false;
      const FTransform relativeTransform = FTransform::Identity;
      constexpr bool deferredFinish = false;
      character->_clueComponent = CastChecked<UTATNPCClueComponent>(character->AddComponentByClass(
         character->_clueComponentClass.Get(),
         manualAttachment,
         relativeTransform,
         deferredFinish
      ));
      character->_clueComponent->AuthorityInitFromSpawner(spawner);
   });
}

void ATATCharacterAIBase::OnHealthChanged_Implementation(float newValue, float oldValue)
{
   Super::OnHealthChanged_Implementation(newValue, oldValue);

   // we have lost all of our health!
   UAbilitySystemComponent* asc = GetAbilitySystemComponent();
   if (asc && newValue == 0.0f && oldValue > 0.0f)
   {
      FGameplayEffectSpecHandle specHandle = asc->MakeOutgoingSpec(_InfiniteUnconsciousEffect, UGameplayEffect::INVALID_LEVEL, FGameplayEffectContextHandle());
      if (specHandle.IsValid())
      {
         asc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
      }

      const ETATDifficulty currentDifficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());

      // Note: Intentionally increment difficulty by one, the FScalableFloat is indexed at 1 but our enum is indexed at 0.
      const int difficultyAsInteger = static_cast<int>(currentDifficulty) + 1;
      static const FString contextString = TEXT("ATATCharacterAIBase::OnHealthChanged_Implementation");
      _FiniteUnconsciousEffect.ApplyEffectWithMagnitude(
         asc, _FiniteUnconsciousEffectDuration.GetValueAtLevel(difficultyAsInteger, &contextString));
   }
}

void ATATCharacterAIBase::_OnDamageChanged(const FOnAttributeChangeData& data)
{
   Super::_OnDamageChanged(data);
   if(const FGameplayEffectModCallbackData* modData = data.GEModData)
   {
      const FGameplayEffectContextHandle context = modData->EffectSpec.GetContext();
      const FVector origin = context.GetOrigin();
      if(AActor* instigator = context.GetInstigator())
      {
         UAISense_Damage::ReportDamageEvent(this, this, instigator, data.NewValue, origin, origin);
         UAISense_VisualEvent::ReportVisualEvent(this, instigator, TEXT("Damage"));
      }
      // Force escalation to vigilant IF we are taking damage.
      if(const ATATAIController* aiController = GetController<ATATAIController>())
      {
         if(UTATEscalationComponent* escalationComponent = aiController->GetTATEscalationComponent())
         {
            escalationComponent->SetState(ETATEscalationState::Vigilant);
         }
      }
   }
}

void ATATCharacterAIBase::_OnHealthChanged(const FOnAttributeChangeData& data)
{
   // THIS IS COPY / PASTED FROM TATCharacter.cpp
   // I don't like the fact that we have the split like this BUT it's a small code change for now.
   // Pass through health attribute change events to the player's inventory component
   // first so it can handle things early.
   // The primary reason for this is that if we let the inventory component register
   // for health change events on its own, it gets called _after_ other handlers like
   // the toolset, which can themselves trigger inventory changes (eg. unequipping the
   // major loot tool causes the item to be dropped).
   // We need to know _why_ loot items are dropped, so let the inventory know first.
   if (HasAuthority())
   {
      if (UTATLootInventoryComponent* inventory = GetLootInventoryComponent())
      {
         inventory->AuthorityOnHealthChanged(data);
      }
      if (data.NewValue <= 0.0f && data.OldValue > 0)
      {
         _DefaultMovementGroupUID = GetCharacterMovement()->GetRVOAvoidanceUID();
         const UTATProjectSettings* projectSettings = GetDefault<UTATProjectSettings>();
         if(UCharacterMovementComponent* movementComponent = GetCharacterMovement())
         {
            movementComponent->SetRVOAvoidanceUID(projectSettings->DownedNPCAvoidanceGroup);
         }
      }
      else if(data.NewValue > 0.f && data.OldValue <= 0.f)
      {
         if(UCharacterMovementComponent* movementComponent = GetCharacterMovement())
         {
            movementComponent->SetRVOAvoidanceUID(_DefaultMovementGroupUID);
         }
      }
   }
   Super::_OnHealthChanged(data);
}

bool ATATCharacterAIBase::IsDebugHUDShowing() const
{
   const UOSEAISettings& settings = UOSEAISettings::Get();
   return settings.ShowingAIDebugHUD;
}


void ATATCharacterAIBase::_OnRep_EscalationState()
{
   _BroadcastEscalationStateChanged();
}

void ATATCharacterAIBase::_BroadcastEscalationStateChanged()
{
   OnEscalationStateNameChanged.Broadcast(_escalationState);
}
void ATATCharacterAIBase::_OnIsGameFrozenChanged(bool isFrozen)
{
   // freeze our AI when the game state is frozen
   if (CanBeFrozen)
   {
      if (isFrozen)
         CustomTimeDilation = 0;
      else
         CustomTimeDilation = _defaultTimeDilation;
   }
}

void ATATCharacterAIBase::_OnAlertnessLevelChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel)
{   
   // when we are leaving neutral to any other state, mark off the time
   if (oldAlertnessLevel == EAlertnessLevel::Neutral)
   {
      _worldTimeLeavingNeutralState = GetWorld()->GetTimeSeconds();
   }
}

void ATATCharacterAIBase::OnLyingDownChanged_Implementation(bool isLyingDown)
{
   Super::OnLyingDownChanged_Implementation(isLyingDown);

   TATCollisionUtils::SetOverlayForLyingDown(this, isLyingDown);
}

void ATATCharacterAIBase::OnUnconsciousChanged_Implementation(bool isUnconscious)
{
   Super::OnUnconsciousChanged_Implementation(isUnconscious);

   if (HasAuthority())
   {
      if (isUnconscious)
      {
         // Spawn a KO glyph on our body
         _AuthoritySpawnKOGlyph();
      }
      else
      {
         // Reset others knowledge of me only after I've gotten up
         // to prevent AI from reviving each other immediately after they've knocked them out
         _AuthorityResetAIKnowledgeOfMyself();
      }
   }
}

void ATATCharacterAIBase::InitializeItemInventory(UItemInventoryComponent* itemInventoryComponent)
{
   if (bIsInventoryInitialized)
   {
      return;
   }

   Super::InitializeItemInventory(itemInventoryComponent);

   if (_tatItemInventory && HasAuthority())
   {
      _tatItemInventory->AuthorityOverrideSize(InventorySize);

      // Wait for ability system to be initialized to add default items.
      CallOrRegisterAbilitiesInitializedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ATATCharacterAIBase::_AuthorityOnAbilitiesInitialized));
   }
}

void ATATCharacterAIBase::_OnGameplayTagChanged(const FGameplayTag gameplayTag, const int32 count)
{
   const UTATAISettings& settings = UTATAISettings::Get();
   
   if(settings.ForcedStateTags.HasTag(gameplayTag) == false)
      return;
   
   const ATATAIController* aiController = GetController<ATATAIController>();
   if(aiController == nullptr)
      return;

   FTATAITargetingEvent_GameplayTagChanged gameplayTagChangedEvent;
   gameplayTagChangedEvent.Exists = count > 0;
   gameplayTagChangedEvent.Tag = gameplayTag;
   
   FStateTreeEvent stateTreeEvent;
   stateTreeEvent.Tag = TAG_StateTreeEvent_GameplayTagChange;
   stateTreeEvent.Payload = FInstancedStruct::Make(gameplayTagChangedEvent);
   
   aiController->SendStateTreeEvent(stateTreeEvent);
}

void ATATCharacterAIBase::_AuthorityOnAbilitiesInitialized()
{
   check(HasAuthority());
   _AuthorityAddDefaultItems();
   _AuthorityTryAddPickpocketableComp();

   if (_tatItemInventory)
   {
      _tatItemInventory->AuthoritySetToolset(GetToolSetInterface());
   }
   if(_tatLootInventory)
   {
      _tatLootInventory->AuthoritySetToolset(GetToolSetInterface());
   }

   // Listen for effects that are added or removed so we can potentially update our unconscious effect state
   AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &ThisClass::_AuthorityOnGameplayEffectWithDurationAdded);
   AbilitySystemComponent->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::_AuthorityOnAnyGameplayEffectAdded);
   AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &ThisClass::_AuthorityOnGameplayEffectRemoved);
   AbilitySystemComponent->RegisterGenericGameplayTagEvent().AddUObject(this, &ATATCharacterAIBase::_OnGameplayTagChanged);
}

void ATATCharacterAIBase::_AuthorityAddDefaultItems()
{
   check(HasAuthority());

   if (_tatItemInventory)
   {
      FTATInventoryNotifyScope inventoryBatch(_tatItemInventory);
      for (const FTATInventorySlot& item : DefaultItems)
      {
         _tatItemInventory->AuthorityAddItemMultiple(item.ItemInfo, item.StackCount);
      }
   }
}

void ATATCharacterAIBase::_AuthorityTryAddPickpocketableComp()
{
   check(HasAuthority());

   if (_tatItemInventory && PickpocketableComponentClass)
   {
      bool hasPickpocketableItem = false;
      _tatItemInventory->ForEachInventorySlot(
         [&hasPickpocketableItem](const FTATInventorySlot& slot, bool& done)
         {
            if (slot.ItemInfo.GetDefaultObject()->IsPickpocketable)
            {
               hasPickpocketableItem = true;
               done = true;
            }
         }
      );

      if (hasPickpocketableItem)
      {
         _pickpocketableComponent = NewObject<UTATPickpocketableComponent>(this, PickpocketableComponentClass);
         if (_pickpocketableComponent)
         {
            _pickpocketableComponent->OnPickpocketableItemAdded.AddUniqueDynamic(this, &ATATCharacterAIBase::OnPickpocketableItemAdded);
            _pickpocketableComponent->OnPickpocketableItemRemoved.AddUniqueDynamic(this, &ATATCharacterAIBase::OnPickpocketableItemRemoved);

            _pickpocketableComponent->RegisterComponent();
         }
      }
   }
}

void ATATCharacterAIBase::OnPickpocketableItemAdded_Implementation(const UTATItemInfo* item)
{
   OnPickpocketableItemAddedEvent.Broadcast(item);
}

void ATATCharacterAIBase::OnPickpocketableItemRemoved_Implementation(const UTATItemInfo* item)
{
   OnPickpocketableItemRemovedEvent.Broadcast(item);
}

void ATATCharacterAIBase::_OnRep_PickpocketableComponent()
{
   if (_pickpocketableComponent)
   {
      _pickpocketableComponent->OnPickpocketableItemAdded.AddUniqueDynamic(this, &ATATCharacterAIBase::OnPickpocketableItemAdded);
      _pickpocketableComponent->OnPickpocketableItemRemoved.AddUniqueDynamic(this, &ATATCharacterAIBase::OnPickpocketableItemRemoved);
   }
}

void ATATCharacterAIBase::GetPickpocketableItems(TArray<UTATItemInfo*>& items) const
{
   if (_pickpocketableComponent)
   {
      _pickpocketableComponent->GetAttachedItems(items);
   }
   else
   {
      items.Empty();
   }
}

void ATATCharacterAIBase::_HandleCheckOverlappingWorldStaticGeometry()
{
   FCollisionQueryParams params;
   params.AddIgnoredActor(this);

   const UCapsuleComponent* const capsule = GetCapsuleComponent();
   const float halfCapsuleHeight = capsule->GetScaledCapsuleHalfHeight() * _CheckOverlappingWorldStaticHeightMultiplier;
   const float halfCapsuleRadius = capsule->GetScaledCapsuleRadius() * _CheckOverlappingWorldStaticHeightRadius;
   const auto shape = FCollisionShape::MakeCapsule(halfCapsuleRadius, halfCapsuleHeight);
   
   FHitResult hitResult;
  
   FCollisionObjectQueryParams queryParams;
   queryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
   
   const bool didHit = GetWorld()->SweepSingleByObjectType(
      hitResult, 
      GetActorLocation(),
      GetActorLocation(), 
      FQuat::Identity,
      queryParams, 
      shape,
      params);
#if WITH_EDITORONLY_DATA
   if (_DebugCheckOverlapWorldStaticGeometry)
   {
      DrawDebugCapsule(
         GetWorld(),
         GetActorLocation(),
         halfCapsuleHeight,
         halfCapsuleRadius,
         FQuat::Identity,
         didHit ? FColor::Red : FColor::Green);
   }
#endif
   _IsCurrentlyOverlappingWorldStaticGeometry = true;
}

UTATKnowledgeComponent* ATATCharacterAIBase::_GetTATKnowledgeComponent() const
{
   if (ATATAIController* controller = Cast<ATATAIController>(GetController()))
      return controller->GetTATKnowledgeComponent();
   return nullptr;
}

void ATATCharacterAIBase::_AuthoritySpawnKOGlyph()
{
   check(HasAuthority());

   if (!KnockedOutGlyphIndicatorType.IsValid())
   {
      // early out if the actor class is null, keeping this code path incase we want some NPC's to have the glyphs
      return;
   }
   
   UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>();
   check(IsValid(thiefVisionSubsystem));

   const FTransform spawnTransform(GetActorLocation());

   if (KnockedOutGlyphSpawnDelaySeconds <= 0)
   {
      // No delay, spawn the KO glyph immediately
      thiefVisionSubsystem->AuthoritySpawnThiefVisionIndicator(KnockedOutGlyphIndicatorType, spawnTransform);
   }
   else
   {
      // Spawn the KO glyph after a delay
      const FGameplayTag glyphIndicatorType = KnockedOutGlyphIndicatorType;
      constexpr bool loop = false;
      FTimerHandle handle;
      GetWorldTimerManager().SetTimer(handle, [spawnTransform, weakThiefVisionSubsystem = MakeWeakObjectPtr(thiefVisionSubsystem), glyphIndicatorType]()
         {
            if (UTATThiefVisionSubsystem* thiefVisionSubsystem = weakThiefVisionSubsystem.Get())
            {
               thiefVisionSubsystem->AuthoritySpawnThiefVisionIndicator(glyphIndicatorType, spawnTransform);
            }
         }, KnockedOutGlyphSpawnDelaySeconds, loop);
   }
}

void ATATCharacterAIBase::_AuthorityOnGameplayEffectWithDurationAdded(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle handle)
{
   _AuthorityRecomputeUnconsciousEffectState();
}

void ATATCharacterAIBase::_AuthorityOnAnyGameplayEffectAdded(
   UAbilitySystemComponent* abilitySystemComponent,
   const FGameplayEffectSpec& gameplayEffectSpec,
   FActiveGameplayEffectHandle activeGameplayEffectHandle)
{
   ensure(HasAuthority());

   FGameplayTagContainer assetTags;
   gameplayEffectSpec.GetAllGrantedTags(assetTags);

   if(assetTags.HasAny(_GameplayTagsThatTriggerEscalationToVigilant))
   {
      if(const ATATAIController* aiController = Cast<ATATAIController>(GetController()))
      {
         if(UTATEscalationComponent* escalationComponent = aiController->GetTATEscalationComponent())
         {
            escalationComponent->SetState(ETATEscalationState::Vigilant);
         }
      }
   }
}

void ATATCharacterAIBase::_AuthorityOnGameplayEffectRemoved(const FActiveGameplayEffect& activeEffect)
{
   _AuthorityRecomputeUnconsciousEffectState();
}

void ATATCharacterAIBase::_AuthorityRecomputeUnconsciousEffectState()
{
   DECLARE_SCOPE_CYCLE_COUNTER(TEXT("[Server] AI Recompute Unconscious Effect Stats"), STAT_TATCharacterAIBase_AuthorityRecomputeUnconsciousEffectState, STATGROUP_TATCharacterAIBase);

   ensure(HasAuthority());

   // NB: _isUnconscious is based on our tags, which will be updated by the time we get the callbacks for when an effect
   // is added or removed, meaning this will be up to date including the newly-added effect
   if (_isUnconscious)
   {
      const FGameplayTag effectDurationTag = UTATProjectSettings::Get().UnconsciousEffectDurationTag;
      const FGameplayEffectQuery query = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(effectDurationTag.GetSingleTagContainer());

      float endTime = 0.0f;
      float totalDuration = 0.0f;
      if (AbilitySystemComponent->GetActiveEffectsEndTimeAndDuration(query, endTime, totalDuration))
      {
         if (ensure(totalDuration > 0))
         {
            _unconsciousEffectInfo.ExpectedServerEndTime = endTime;
            _unconsciousEffectInfo.TotalEffectDuration = totalDuration;
            _unconsciousEffectInfo.UnconsciousState = ETATCharacterUnconsciousState::DurationUnconscious;
         }
         else
         {
            // This shouldn't happen, since we should be looking for only duration-based unconscious effects,
            // but if it does happen it indicates that there is an infinite unconscious effect marked with the duration tag
            _unconsciousEffectInfo.UnconsciousState = ETATCharacterUnconsciousState::IndefiniteUnconscious;
         }
      }
      else
      {
         // We don't have an duration-based unconscious effects but still are unconscious, so we are indefinitely unconscious
         _unconsciousEffectInfo.UnconsciousState = ETATCharacterUnconsciousState::IndefiniteUnconscious;
      }
   }
   else
   {
      _unconsciousEffectInfo.UnconsciousState = ETATCharacterUnconsciousState::Conscious;
   }
}

UOSEStimDatabase* ATATCharacterAIBase::AuthorityGetStimDatabase() const
{
   // fwd to controller
   if (const IOSEStimDatabaseInterface* controllerInterface = Cast<IOSEStimDatabaseInterface>(GetController()))
   {
      return controllerInterface->AuthorityGetStimDatabase();
   }
   return nullptr;
}

UOSEIndividualKnowledgeComponent* ATATCharacterAIBase::GetIndividualKnowledgeComponent() const
{
   if (const IOSEIndividualKnowledgeInterface* knowledgeInterface = Cast<IOSEIndividualKnowledgeInterface>(GetController()))
   {
      return knowledgeInterface->GetIndividualKnowledgeComponent();
   }
   return nullptr;
}

UOSEVoiceLineKnowledgeComponent* ATATCharacterAIBase::GetVoiceLineKnowledgeComponent() const
{
   if (const IOSEVoiceLineKnowledgeInterface* voiceLineKnowledgeInterface = Cast<IOSEVoiceLineKnowledgeInterface>(GetController()))
   {
      return voiceLineKnowledgeInterface->GetVoiceLineKnowledgeComponent();
   }
   return nullptr;
}

bool ATATCharacterAIBase::CanBeSneakAttackedByActor(AActor* actor) const
{
   return BP_CanBeSneakAttackedByActor(actor);
}

bool ATATCharacterAIBase::BP_CanBeSneakAttackedByActor_Implementation(AActor* actor) const
{
   if (actor)
   {
      return UTATCombatFunctionLibrary::CanBeSneakAttackedFromPosition(this, actor->GetActorLocation());
   }
   else
   {
      return false;
   }
}

void ATATCharacterAIBase::OnSneakAttackedByActor(AActor* actor)
{
   UE_LOG(LogTATCharacterAIBase, Verbose, TEXT("AI '%s' was sneak attacked by '%s'"), *GetName(), *GetNameSafe(actor));
}

bool ATATCharacterAIBase::CanBeCounterAttackedByActor(AActor* actor) const
{
   return UTATCombatFunctionLibrary::IsDefenderVulnerableToCounterByAttacker(actor, this);
}

void ATATCharacterAIBase::OnCounterAttackedByActor(AActor* actor)
{
   UE_LOG(LogTATCharacterAIBase, Verbose, TEXT("AI '%s' was counter attacked by '%s'"), *GetName(), *GetNameSafe(actor));

   const UTATCombatSettings& combatSettings = UTATCombatSettings::Get();
   AbilitySystemComponent->RemoveActiveEffectsWithAppliedTags(combatSettings.IsVulnerableToCounterTag.GetSingleTagContainer());
}

bool ATATCharacterAIBase::IsAllowedToSeeActor(const AActor* actor) const
{
   return true;
}

void ATATCharacterAIBase::ModifySightRangeForSpecificActor(const AActor* actor, float& outSightRadius) const
{
   if(UTATProjectSettings::ShouldUseLightDetection() == false)
      return;

   if(const ATATAIController* aiController = GetController<ATATAIController>())
   {
      // If the current target for the state tree is the same we're checking the sight range on, then don't modify the vision value.
      if(aiController->IsCurrentStateTreeTarget(actor))
      {
         return;
      }
   }
   if(const ITATStealthScoreInterface* stealthScoreInterface = Cast<ITATStealthScoreInterface>(actor))
   {        
      outSightRadius *= UOSELightDetectionFunctionLibrary::GetRangeMultiplierFromLightIntensity(
         1.f - stealthScoreInterface->GetStealthScore(),
         _LightIntensityToRangeMultiplierCurve
      );
   }

}

FString ATATCharacterAIBase::DescribeSightRangeModificationForSpecificActor(const AActor* actor) const
{
   if (!UTATProjectSettings::ShouldUseLightDetection())
   {
      return TEXT("Light detection is disabled, no modifiers to apply.");
   }

   if (const IOSELightDetectionInterface* lightDetectionInterface = Cast<IOSELightDetectionInterface>(actor))
   {
      const float lightActualIntensity = lightDetectionInterface->GetActualLightIntensityFromLightSources();
      const float lightCurrentIntensity = lightDetectionInterface->GetCurrentLightIntensityPlusMinimumValue();
      const float lightCurrentModifier = UOSELightDetectionFunctionLibrary::GetRangeMultiplierFromLightIntensity(
         lightCurrentIntensity,
         _LightIntensityToRangeMultiplierCurve
      );

      return FString::Printf(TEXT("Light Intensity: Actual (%.2f), Current (%.2f), Current Modifier (%.2f)"), 
         lightActualIntensity, lightCurrentIntensity, lightCurrentModifier);
   }
   else
   {
      return TEXT("No modifiers to apply.");
   }
}

void ATATCharacterAIBase::_AuthorityHandleEscalationStateChanged(const ETATEscalationState newEscalationState)
{
   _escalationState = newEscalationState;
   _BroadcastEscalationStateChanged();
}

UOSEAlertnessComponent* ATATCharacterAIBase::GetAlertnessComponent() const
{
   return _alertnessComponent;
}

EAlertnessLevel ATATCharacterAIBase::GetAlertnessLevel() const
{
   if (_alertnessComponent)
      return _alertnessComponent->GetAlertnessLevel();
   return EAlertnessLevel::Neutral;
}

void ATATCharacterAIBase::AuthorityOnEnterVisibleByActor(AActor* viewingActor)
{
}

void ATATCharacterAIBase::AuthorityOnExitVisibleByActor(AActor* viewingActor)
{
}

UAnimMontage* ATATCharacterAIBase::GetCharacterMontage(const FGameplayTag& animationTag) const
{
   if (_characterAnimationMapping)
   {
      return _characterAnimationMapping->LookupMontageByTag(animationTag);
   }
   else
   {
      UE_LOG(LogTATCharacterAIBase, Warning, TEXT("TATCharacterAIBase '%s' is missing _characterAnimationMapping, despite GetCharacterMontage called on it"), *GetName());
      return nullptr;
   }
}

USkeletalMesh* ATATCharacterAIBase::GetDisguiseTargetMesh(ETATDisguiseMeshType meshType, TArray<UMaterialInterface*>& outOverrideMaterials) const
{
   switch (meshType)
   {
   case ETATDisguiseMeshType::Skeleton:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(GetMesh(), outOverrideMaterials);
   case ETATDisguiseMeshType::ThirdPersonBody:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(GetMesh3P_Body(), outOverrideMaterials);
   case ETATDisguiseMeshType::ThirdPersonHead:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(GetMesh3P_Head(), outOverrideMaterials);
   case ETATDisguiseMeshType::FirstPersonLowerBody:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(_firstPersonLowerBodyMeshForDisguise, outOverrideMaterials);
   case ETATDisguiseMeshType::FirstPersonUpperBody:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(_firstPersonUpperBodyMeshForDisguise, outOverrideMaterials);
   default:
      checkNoEntry();
      break;
   }
   return nullptr;
}

TSubclassOf<UAnimInstance> ATATCharacterAIBase::GetDisguiseTargetThirdPersonAnimClass() const
{
   if (USkeletalMeshComponent* bodyMeshComp = GetMesh3P())
   {
      return bodyMeshComp->GetAnimClass();
   }
   return nullptr;
}

TSubclassOf<UAnimInstance> ATATCharacterAIBase::GetDisguiseTargetAnimSetLayer() const
{
   return _GetAnimSetForTag(TAG_AnimSet_Disguise);
}

FTATDisguiseMovementParams ATATCharacterAIBase::GetDisguiseTargetMovementParams() const
{
   FTATDisguiseMovementParams result{};
   if (UTATCharacterMovement* moveComp = GetCharacterMovement<UTATCharacterMovement>())
   {
      result.MaxWalkSpeed = moveComp->MaxWalkSpeed;
      result.MaxAcceleration = moveComp->MaxAcceleration;
   }
   return result;
}

ETATDisguiseTargetType ATATCharacterAIBase::GetDisguiseTargetType() const
{
   return DisguiseTargetType;
}
