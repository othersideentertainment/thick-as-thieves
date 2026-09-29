// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Character/TATCharacterBase.h"

// tat
#include "Abilities/TATGameplayTags.h"
#include "AI/TATAIController.h"
#include "AI/Perception/TATAIPerceptionSystem.h"
#include "Animation/TATAnimSetMapping.h"
#include "Animation/TATAnimSetTagTriggers.h"
#include "Environment/TATAreaMarkupVolume.h"
#include "Environment/TATWeatherSubsystem.h"
#include "Character/TATCharacterBaseChangeNotifyInterface.h"
#include "Combat/CombatComponent.h"
#include "Character/TATCharacterLandedOnActorInterface.h"
#include "Developer/TATProjectSettings.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Interactables/InteractHoldAbilityInterface.h"
#include "Interactables/OSEInteractionHelpers.h"
#include "Items/ItemInventoryComponent.h"
#include "Player/OSEPlayerStats.h"

// ue
#include "SignificanceManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterBase)

namespace CharacterCVars {
   static TAutoConsoleVariable<int32> CVarAutoLODMode(
      TEXT("tat.Character.AutoLOD.Mode"),
      0,
      TEXT("Mode for AutoLODs (switching to/from leaderpose based on LOD)\n")\
      TEXT("0 = default\n")\
      TEXT("1 = force leader pose\n")\
      TEXT("2 = force non-leader pose\n"),
      ECVF_Default | ECVF_Scalability
   );

   static TAutoConsoleVariable<int32> CVarAutoLODBuffer(
      TEXT("tat.Character.AutoLOD.Buffer"),
      1,
      TEXT("Extra LOD count before switching to leader pose"),
      ECVF_Default
   );

   static TAutoConsoleVariable<int32> CVarAutoLODMax(
      TEXT("tat.Character.AutoLOD.Max"),
      2,
      TEXT("Max LOD level to switch to leader pose at"),
      ECVF_Default
   );

   static TAutoConsoleVariable<int32> CVarTickAnimThreshold(
      TEXT("tat.Character.AutoLOD.TickAnimThreshold"),
      2,
      TEXT("Max LOD that it will tick the animation after re-enabling the animation"),
      ECVF_Default | ECVF_Scalability
   );
}

namespace TATCharacterBase
{
   const static FName SignificanceGroupName = FName(TEXT("TATCharacter"));

   static float SignificanceFunction(const USignificanceManager::FManagedObjectInfo* objectInfo,
      const FTransform& viewport)
   {
      if (objectInfo->GetTag() == SignificanceGroupName)
      {
         const ATATCharacterBase* character = CastChecked<ATATCharacterBase>(objectInfo->GetObject());
         const float distance = (character->GetActorLocation() - viewport.GetLocation()).Size();

         return character->GetSignificanceByDistance(distance);
      }

      return 0.f;
   }

   static void PostSignificanceFunction(const USignificanceManager::FManagedObjectInfo* objectInfo,
      float oldSignificance,
      float significanceValue,
      bool bFinal)
   {
      if (objectInfo->GetTag() == SignificanceGroupName)
      {
         const ATATCharacterBase* character = CastChecked<ATATCharacterBase>(objectInfo->GetObject());
         UCharacterMovementComponent* characterMovementComponent = character->GetCharacterMovement();

         // Current low-risk stab: Don't try to change movement mode, since that is more likely to have potential
         // side effects, just attenuate cost of nav-walking

         characterMovementComponent->bSweepWhileNavWalking = (significanceValue >= 1.0f);
      }
   }

   static void UpdateLeaderMeshes(USkeletalMeshComponent* mainMesh, TConstArrayView<USkeletalMeshComponent*> followers)
   {
      TRACE_CPUPROFILER_EVENT_SCOPE(TATCharacterBase::UpdateLeaderMeshes)
      QUICK_SCOPE_CYCLE_COUNTER(STAT_TATCharacterBase_UpdateLeaderMeshes);
      check(mainMesh);
      const int predictedLod = mainMesh->GetPredictedLODLevel();

      const int mode = CharacterCVars::CVarAutoLODMode.GetValueOnGameThread();
      const int lodBuffer = CharacterCVars::CVarAutoLODBuffer.GetValueOnGameThread();
      const int maxLodThreshold = CharacterCVars::CVarAutoLODMax.GetValueOnGameThread();
      constexpr int kModeForceLeader = 1;
      constexpr int kModeForceAbp = 2;
      
      for (USkeletalMeshComponent* follower : followers)
      {
         if(follower == nullptr) continue;

         const USkeletalMesh* mesh = follower->GetSkeletalMeshAsset();
         if(mesh == nullptr) continue;

         // Switching from leader-pose for meshes with post-proc ABPs within some buffered range of the LODs
         // that they would be used at

         // NOTE: Not using ShouldEvaluatePostProcessAnimBP so that -1 means never rather than always
         const int lodThreshold = (mode == kModeForceAbp) ? 10 : FMath::Min(mesh->GetPostProcessAnimBPLODThreshold() + lodBuffer, maxLodThreshold);

         const bool shouldUseAbp = (mode != kModeForceLeader)
            && follower->GetAnimClass() != nullptr
            && mesh->GetPostProcessAnimBlueprint() != nullptr
            && predictedLod <= lodThreshold;
         bool hasChanged = shouldUseAbp != (!follower->LeaderPoseComponent.IsValid());
         if (!hasChanged)
         {
            continue;
         }

         if (shouldUseAbp)
         {
            follower->SetLeaderPoseComponent(nullptr);
            follower->ResetAnimInstanceDynamics();
            if (follower->bRecentlyRendered && follower->GetPredictedLODLevel() <= CharacterCVars::CVarTickAnimThreshold.GetValueOnGameThread())
            {
               // This avoids some artifacts caused by the delay in updating it next, but has some cost
               // ~20us with just copy-pose
               follower->TickAnimation(0, false);
            }
         }
         else
         {
            follower->SetLeaderPoseComponent(mainMesh);
         }
      }
   }

   static void UpdateLeaderMeshes(ATATCharacterBase* character)
   {
      UpdateLeaderMeshes(character->GetMesh(), {character->GetMesh3P_Body(), character->GetMesh3P_Head()});
   }
}

void FTATCharacterLeaderMeshTickFunction::ExecuteTick(float deltaTime, ELevelTick tickType, ENamedThreads::Type currentThread, const FGraphEventRef& myCompletionGraphEvent)
{
   TATCharacterBase::UpdateLeaderMeshes(Target);
}

FString FTATCharacterLeaderMeshTickFunction::DiagnosticMessage()
{
   return Target->GetFullName() + TEXT("[TATCharacterLeaderMeshTick]");
}

FName FTATCharacterLeaderMeshTickFunction::DiagnosticContext(bool bDetailed)
{
   if (bDetailed)
   {
      return FName(*FString::Printf(TEXT("TATCharacterLeaderMeshTick/%s"), *GetFullNameSafe(Target)));
   }

   return FName(TEXT("TATCharacterLeaderMeshTick"));
}

ATATCharacterBase::ATATCharacterBase(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
   _voiceLineTraitsToTrack.AddTag(TAG_Status_Indoors);
   _combatPositioningComponent = CreateDefaultSubobject<UTATCombatPositioningComponent>(TEXT("CombatPositioningComponent"));

   Mesh3P_Body = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("Mesh3P_Body");
   if(Mesh3P_Body)
   {
      Mesh3P_Body->SetupAttachment(GetMesh());
      Mesh3P_Body->SetRelativeRotation(FRotator::ZeroRotator);
      Mesh3P_Body->SetRelativeLocation(FVector::ZeroVector);
      Mesh3P_Body->SetRelativeScale3D(FVector::OneVector);
      Mesh3P_Body->LeaderPoseComponent = GetMesh();
      
      Mesh3P_Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
      Mesh3P_Body->bEnableUpdateRateOptimizations = true;
      Mesh3P_Body->bUseAttachParentBound = true;
      Mesh3P_Body->SetOwnerNoSee(true);
      Mesh3P_Body->SetOnlyOwnerSee(false);
      Mesh3P_Body->SetReceivesDecals(true);
      Mesh3P_Body->SetGenerateOverlapEvents(false);

      // Lighting
      Mesh3P_Body->SetCastShadow(true);
      Mesh3P_Body->bCastHiddenShadow = true;
   }

   ItemInventoryComponent = CreateOptionalDefaultSubobject<UItemInventoryComponent>(AOSECharacterBase::ItemInventoryComponentName);
   if (ItemInventoryComponent)
   {
      bCreatedStaticItemInventoryComponent = true;
   }

   _leaderMeshTickFunction.bAllowTickOnDedicatedServer = false;
   _leaderMeshTickFunction.bCanEverTick = true;
   _leaderMeshTickFunction.bStartWithTickEnabled = true;
   _leaderMeshTickFunction.SetTickFunctionEnable(true);
   _leaderMeshTickFunction.TickGroup = TG_PrePhysics;
}

void ATATCharacterBase::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      if (UTATWeatherSubsystem* weatherSubsystem = GetWorld()->GetSubsystem<UTATWeatherSubsystem>())
      {
         constexpr bool enablePolling = true;
         weatherSubsystem->RegisterActor(this, enablePolling);
      }
   }

   if (GetMesh())
   {
      TATCharacterBase::UpdateLeaderMeshes(this);
   }

   if (bCreatedStaticItemInventoryComponent)
   {
      InitializeItemInventory(ItemInventoryComponent);
   }
   
   if (_significanceSettings.SignificanceThresholds.Num() > 0)
   {
      if (USignificanceManager* significanceManager = USignificanceManager::Get(GetWorld()))
      {
         significanceManager->RegisterObject(this, TATCharacterBase::SignificanceGroupName, TATCharacterBase::SignificanceFunction, USignificanceManager::EPostSignificanceType::Sequential, TATCharacterBase::PostSignificanceFunction);
      }
   }
   _RefreshAnimSet();
}

void ATATCharacterBase::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (HasAuthority())
   {
      if (UTATWeatherSubsystem* weatherSubsystem = GetWorld()->GetSubsystem<UTATWeatherSubsystem>())
      {
         weatherSubsystem->UnregisterActor(this);
      }
   }

   if (USignificanceManager* significanceManager = USignificanceManager::Get(GetWorld()))
   {
      significanceManager->UnregisterObject(this);
   }

   Super::EndPlay(endPlayReason);
}

void ATATCharacterBase::SetBase(UPrimitiveComponent* newBase, const FName boneName, bool notifyActor)
{
   UPrimitiveComponent* oldBase = BasedMovement.MovementBase;

   Super::SetBase(newBase, boneName, notifyActor);

   // If the actor we're about to start basing on implements ITATCharacterBaseChangeNotifyInterface, notify it
   if (newBase != nullptr)
   {
      AActor* owner = newBase->GetOwner();
      if (newBase != oldBase && owner != nullptr && owner->Implements<UTATCharacterBaseChangeNotifyInterface>())
      {
         ITATCharacterBaseChangeNotifyInterface::Execute_OnCharacterBeginBasing(owner, this);
      }
   }
}

void ATATCharacterBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   if (!bCreatedStaticItemInventoryComponent)
   {
      DOREPLIFETIME(ThisClass, ItemInventoryComponent);
   }
   else
   {
      DISABLE_REPLICATED_PROPERTY(ThisClass, ItemInventoryComponent);
   }
   DOREPLIFETIME_CONDITION(ThisClass, _individualAttitudes, COND_OwnerOnly);
}

void ATATCharacterBase::InitializeAbilities(UOSEAbilitySystemComponent* inComponent, UAttributeBaseSet* inAttributeSet)
{
   Super::InitializeAbilities(inComponent, inAttributeSet);

   // player characters don't have ASC set until this method is called
   // as its provided by the player state
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      asc->AddLooseGameplayTags(_characterTraits);

      asc->SetLooseGameplayTagCount(TAG_Status_Health_Full, GetHealth() >= GetHealthMax() ? 1 : 0);

      for (const FGameplayTag& trackedTag : _voiceLineTraitsToTrack)
      {
         _OnVoiceLineAppliedTraitTagsChanged(trackedTag, asc->GetGameplayTagCount(trackedTag));
         asc->RegisterGameplayTagEvent(trackedTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ATATCharacterBase::_OnVoiceLineAppliedTraitTagsChanged);
      }

      if(_animSetTagTriggers)
      {
         for(const FTATAnimSetTagTrigger& trigger : _animSetTagTriggers->AnimSetTriggers)
         {
            asc->RegisterGameplayTagEvent(trigger.RequiredTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::_OnAnimSetTriggerChanged);
         }
      }
   }
}

void ATATCharacterBase::ResetAbilities()
{
   Super::ResetAbilities();

   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      asc->RemoveLooseGameplayTags(_characterTraits);

      for (const FGameplayTag& trackedTag : _voiceLineTraitsToTrack)
      {
         asc->RegisterGameplayTagEvent(trackedTag).RemoveAll(this);
      }

      if(_animSetTagTriggers)
      {
         for(const FTATAnimSetTagTrigger& trigger : _animSetTagTriggers->AnimSetTriggers)
         {
            asc->RegisterGameplayTagEvent(trigger.RequiredTag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
         }
         RemoveAnimSetBySource(_animSetTagTriggers);
      }
   }
}

//-------------------------------------------------------------------------------------------------
// IInteractableInterface
//-------------------------------------------------------------------------------------------------

bool ATATCharacterBase::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   // by default, look for matching abilities. Should we have a flag to opt out, or just override the method in BPs?
   return UOSEInteractionHelpers::HasCharacterInteractionAbility(interactingCharacter, this);
}

FInteractStartResult ATATCharacterBase::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   // kFallbackInteractionTime used to be a property, but everything should override it in practice
   constexpr float kFallbackInteractionTime = 2.f;
   const FCharacterInteractHoldInfo holdInfo = UOSEInteractionHelpers::GetHoldInfoForCharacterInteraction(
      interactingCharacter,
      this,
      kFallbackInteractionTime
   );
   const float delay = holdInfo.bHasHold ? holdInfo.HoldDuration : 0;

   FInteractStartResult result;
   result.bWaitForDelay = delay > 0.0f;
   result.Delay = delay;
   result.HoldAnimationTag = holdInfo.HoldAnimation;
   result.HoldTargetEffect = holdInfo.HoldTargetEffect;
   result.HoldGameplayAbility = holdInfo.Ability;

   const FCharacterInteractInstantAnimationInfo animationInfo = UOSEInteractionHelpers::GetInstantAnimationForCharacterInteraction(interactingCharacter, this);
   if (animationInfo.bHasInstantAnimation)
   {
      result.InstantAnimationTag = animationInfo.InstantAnimation;
   }

   if (holdInfo.Ability && holdInfo.Ability->Implements<UInteractHoldAbilityInterface>())
   {
      result.IsCharacterAllowedToMoveDuringInteraction = IInteractHoldAbilityInterface::Execute_IsCharacterAllowedToMoveDuringHoldInteraction(
         holdInfo.Ability,
         interactingCharacter,
         this
      );
   }

   // if it is a press, just do it
   if (!result.bWaitForDelay)
   {
      UOSEInteractionHelpers::TriggerAbilityForCharacterInteraction(interactingCharacter, this, false);
   }

   return result;
}

bool ATATCharacterBase::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (context.IsComplete())
   {
      return UOSEInteractionHelpers::TriggerAbilityForCharacterInteraction(interactingCharacter, this, true);
   }
   else if (context.IsProbablyInstant())
   {
      return UOSEInteractionHelpers::TriggerAbilityForCharacterInteraction(interactingCharacter, this, false);
   }
   return false;
}

void ATATCharacterBase::GetInteractPrompt_Implementation(ACharacter* InteractingCharacter, FInteractPrompt& outPrompt)
{
   UOSEInteractionHelpers::GetPromptForCharacterInteraction(InteractingCharacter, this, outPrompt);
}

void ATATCharacterBase::AuthorityOnKnockedOutByOtherCharacter_Implementation(AOSECharacterBase* otherCharacter)
{
   Super::AuthorityOnKnockedOutByOtherCharacter_Implementation(otherCharacter);

   // increment stat for knocking-out player
   UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(otherCharacter, _knockOutStatForAttackingPlayer);
}

bool ATATCharacterBase::_InterceptLanded(const FHitResult& hit)
{
   // If we're playing a synced animation, intercept any landing events.
   if (HasMatchingGameplayTag(UTATProjectSettings::Get().PlayingSyncedAnimationTag))
   {
      return true;
   }
   
   bool interceptLandedEvent = false;

   struct FLandedInfo
   {
      AActor* Owner = nullptr;
      UPrimitiveComponent* Comp = nullptr;
      FVector Location = FVector::ZeroVector;
   };
   TArray<FLandedInfo, TInlineAllocator<16>> landedInfo;

   auto actorWantsCharacterLandedEvents = [this](AActor* actor)
   {
      ITATCharacterLandedOnActorInterface* iface = Cast<ITATCharacterLandedOnActorInterface>(actor);
      return (iface != nullptr) ? iface->WantsToHandleCharacterLandedEvents(this) : false;
   };

   if (UCapsuleComponent* capsule = GetCapsuleComponent())
   {
      TSet<UPrimitiveComponent*, DefaultKeyFuncs<UPrimitiveComponent*>, TInlineSetAllocator<16>> visitedComponents;
      for (const FOverlapInfo& overlapInfo : capsule->GetOverlapInfos())
      {
         UPrimitiveComponent* comp = overlapInfo.OverlapInfo.Component.Get();
         if (comp && actorWantsCharacterLandedEvents(comp->GetOwner()) && !visitedComponents.Contains(comp))
         {
            visitedComponents.Add(comp);
            landedInfo.Add(FLandedInfo{ comp->GetOwner(), comp, overlapInfo.OverlapInfo.ImpactPoint });
         }
      }
   }

   if (actorWantsCharacterLandedEvents(hit.GetActor()))
   {
      landedInfo.Add(FLandedInfo{ hit.GetActor(), nullptr, hit.Location });
   }

   if (landedInfo.Num() > 0)
   {
      // Sort the actors we hit by distance to the ground, picking the one farthest away
      // (we'll assume we hit that overlap first on the way towards the ground)
      landedInfo.Sort([&hit](const FLandedInfo& a, const FLandedInfo& b)
      {
         return FVector::DistSquared(a.Location, hit.Location) < FVector::DistSquared(b.Location, hit.Location);
      });

      // for (int32 i = 0; i < landedInfo.Num(); i++)
      // {
      //    const FLandedInfo& info = landedInfo[i];
      //    DrawDebugPoint(GetWorld(), info.Location, 2.0f, FColor::Emerald, false, 10.0f);
      //    DrawDebugString(GetWorld(), info.Location, FString::Printf(TEXT("[%i] %s . %s"), i, *GetNameSafe(info.Owner), *GetNameSafe(info.Comp)), nullptr, FColor::White, 10.0f);
      // }

      CastChecked<ITATCharacterLandedOnActorInterface>(landedInfo[0].Owner)->OnCharacterLandedOnThisActor(this, hit, landedInfo[0].Comp, interceptLandedEvent);
   }

   return interceptLandedEvent;
}

void ATATCharacterBase::GetTraits(FGameplayTagContainer& tagContainer) const
{
   if(const UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      asc->GetOwnedGameplayTags(tagContainer);
   }
}

void ATATCharacterBase::AddArea(const TObjectPtr<ATATAreaMarkupVolume> area)
{
   _areaMarkupVolumes.Add(area);
   _areaMarkupTags.AppendTags(area->GetAreaMarkupTags());
}

void ATATCharacterBase::RemoveArea(const TObjectPtr<ATATAreaMarkupVolume> area)
{
   _areaMarkupVolumes.Remove(area);
   _areaMarkupTags.RemoveTags(area->GetAreaMarkupTags());
}

const TArray<TWeakObjectPtr<ATATAreaMarkupVolume>>& ATATCharacterBase::GetAreasCurrentlyContainingActor() const
{
   return _areaMarkupVolumes;
}

void ATATCharacterBase::AttitudeChangedFromActor(AActor* sourceOfChange, EOSEIndividualAttitude attitude)
{
   FIndividualAttitude* individualAttitude = FindAttitude(sourceOfChange);
   if(individualAttitude == nullptr)
   {
      individualAttitude = &_individualAttitudes.AddDefaulted_GetRef();
      individualAttitude->SetTarget(sourceOfChange);
   }
   individualAttitude->SetAttitude(attitude);
}

EOSEIndividualAttitude ATATCharacterBase::GetAttitudeFromActor(const AActor* sourceOfChange) const
{
   const FIndividualAttitude* attitude = FindAttitude(sourceOfChange);
   if(attitude == nullptr)
      return EOSEIndividualAttitude::Unknown;
   return attitude->GetAttitude();
}

const FIndividualAttitude* ATATCharacterBase::FindAttitude(const AActor* sourceOfChange) const
{
   const FIndividualAttitude* returnValue = _individualAttitudes.FindByPredicate(
      [sourceOfChange](const FIndividualAttitude& attitude)
      {
         return attitude.MatchesTarget(sourceOfChange);
      });
   return returnValue;
}

FIndividualAttitude* ATATCharacterBase::FindAttitude(const AActor* sourceOfChange)
{
   FIndividualAttitude* returnValue = _individualAttitudes.FindByPredicate(
      [sourceOfChange](const FIndividualAttitude& attitude)
      {
         return attitude.MatchesTarget(sourceOfChange);
      });
   return returnValue;
}

void ATATCharacterBase::RegisterActorTickFunctions(bool shouldRegister)
{
   Super::RegisterActorTickFunctions(shouldRegister);

   if (shouldRegister)
   {

      if (_leaderMeshTickFunction.bCanEverTick)
      {
         _leaderMeshTickFunction.Target = this;
         _leaderMeshTickFunction.SetTickFunctionEnable(_leaderMeshTickFunction.bStartWithTickEnabled || _leaderMeshTickFunction.IsTickFunctionEnabled());
         _leaderMeshTickFunction.RegisterTickFunction(GetLevel());

         // This tick function is sandwiched between the main mesh, and the follower meshes that would be dynamically switched
         // to and from leader pose
         _leaderMeshTickFunction.AddPrerequisite(GetMesh(), GetMesh()->PrimaryComponentTick);
         auto tryAddFollowerDependency = [this](UActorComponent* component)
            {
               if (component)
               {
                  component->PrimaryComponentTick.AddPrerequisite(this, _leaderMeshTickFunction);
               }
            };
         tryAddFollowerDependency(GetMesh3P_Body());
         tryAddFollowerDependency(GetMesh3P_Head());
      }
   }
   else
   {
      if (_leaderMeshTickFunction.IsTickFunctionRegistered())
      {
         _leaderMeshTickFunction.UnRegisterTickFunction();
      }
   }
}

TSubclassOf<UAnimInstance> ATATCharacterBase::_GetAnimSetForTag(FGameplayTag tag) const
{
   return _animSetMapping ? _animSetMapping->GetAnimSetByTag(tag) : nullptr;
}

void ATATCharacterBase::SuppressAnimSets(bool isSuppressed)
{
   if(_suppressAnimSets == isSuppressed)
   {
      return;
   }

   _suppressAnimSets = isSuppressed;
   if(isSuppressed)
   {
      _SetAnimSet(nullptr);
   }
   else
   {
      _RefreshAnimSet();
   }
}

void ATATCharacterBase::AddAnimSetRequest(const FTATAnimSetRequest& request)
{
   _animSetOverrides.AddRequest(request);
   _ScheduleAnimSetRefresh();
}

void ATATCharacterBase::RemoveAnimSetRequest(const FTATAnimSetRequest& request)
{
   _animSetOverrides.RemoveRequest(request);
   _ScheduleAnimSetRefresh();
}

void ATATCharacterBase::RemoveAnimSetBySource(FObjectKey source)
{
   _animSetOverrides.RemoveBySource(source);
   _ScheduleAnimSetRefresh();
}

void ATATCharacterBase::_ScheduleAnimSetRefresh()
{
   if(!_refreshAnimSetTimer.IsValid())
   {
      _refreshAnimSetTimer = GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::_RefreshAnimSet));
   }
}

void ATATCharacterBase::_RefreshAnimSet()
{
   _refreshAnimSetTimer.Invalidate();
   if(_suppressAnimSets)
   {
      return;
   }

   FGameplayTag animSetTag = _animSetOverrides.GetCurrentOverride();
   _SetAnimSet(_GetAnimSetForTag(animSetTag));
}

void ATATCharacterBase::_OnAnimSetTriggerChanged(const FGameplayTag tag, int count)
{
   if(_animSetTagTriggers)
   {
      _animSetTagTriggers->ApplyTagCountChange(tag, count, _animSetOverrides);
      _ScheduleAnimSetRefresh();
   }
}

void ATATCharacterBase::_SetAnimSet(TSubclassOf<UAnimInstance> animSetClass)
{
   if(_currentAnimSet != animSetClass)
   {
      if(_currentAnimSet)
      {
         GetMesh()->UnlinkAnimClassLayers(_currentAnimSet);
      }
      _currentAnimSet = animSetClass;
      if(animSetClass)
      {
         GetMesh()->LinkAnimClassLayers(animSetClass);
      }
   }
}

void ATATCharacterBase::_OnHealthChanged(const FOnAttributeChangeData& data)
{
   Super::_OnHealthChanged(data);

   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      asc->SetLooseGameplayTagCount(TAG_Status_Health_Full, GetHealth() >= GetHealthMax() ? 1 : 0);
   }
}

void ATATCharacterBase::InitializeItemInventory(UItemInventoryComponent* itemInventoryComponent)
{
   if (bIsInventoryInitialized)
   {
      return;
   }

   if (!ItemInventoryComponent)
   {
      ItemInventoryComponent = itemInventoryComponent;
   }
}

float ATATCharacterBase::GetSignificanceByDistance(const float distance) const
{
   const int32 numThresholds = _significanceSettings.SignificanceThresholds.Num();
   if(numThresholds == 0)
      return 0.f;
   if (distance >= _significanceSettings.SignificanceThresholds[numThresholds - 1].MaxDistance)
      return _significanceSettings.SignificanceThresholds[numThresholds - 1].Significance;
   for (int32 id = 0; id < numThresholds; id++)
   {
      const FSignificanceThresholds& thresholds = _significanceSettings.SignificanceThresholds[id];
      if (distance <= thresholds.MaxDistance)
         return thresholds.Significance;
   }
   return 0.f;
}

void ATATCharacterBase::OnEnterTargetedByStateTree(AOSECharacterBase* aiCharacter)
{
   _combatPositioningComponent->AddTargetingCharacter(aiCharacter);
   if (CombatComponent)
   {
      CombatComponent->AddTargetingCharacter(aiCharacter);
   }
}

void ATATCharacterBase::OnExitTargetedByStateTree(AOSECharacterBase* aiCharacter)
{
   _combatPositioningComponent->RemoveTargetingCharacter(aiCharacter);
   if (CombatComponent)
   {
      CombatComponent->RemoveTargetingCharacter(aiCharacter);
   }
}

void ATATCharacterBase::GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const
{
   if(const ATATAIController* const aiController = GetController<ATATAIController>())
   {
      aiController->GetActorTraitsForVoiceLines(tagContainer);
   }
   tagContainer.AppendTags(_areaMarkupTags);
   tagContainer.AppendTags(_voiceLineTraitsApplied);
}

void ATATCharacterBase::_OnVoiceLineAppliedTraitTagsChanged(const FGameplayTag tag, int32 newTagCount)
{
   if (newTagCount > 0)
   {
      _voiceLineTraitsApplied.AddTag(tag);
   }
   else
   {
      _voiceLineTraitsApplied.RemoveTag(tag);
   }
}

void ATATCharacterBase::_OnMoveInput(const FInputActionValue& value)
{
   if (!HasMatchingGameplayTag(UTATProjectSettings::Get().DisableDirectMoveInputTag))
   {
      Super::_OnMoveInput(value);
   }
}

void ATATCharacterBase::_OnLookInput(const FInputActionValue& value)
{
   if (!HasMatchingGameplayTag(UTATProjectSettings::Get().DisableDirectLookInputTag))
   {
      Super::_OnLookInput(value);
   }
}

void ATATCharacterBase::_AuthorityResetAIKnowledgeOfMyself()
{
   check(HasAuthority());

   // Reset AI knowledge of the actor after KO.
   UTATAIPerceptionSystem::ResetKnowledgeOfActor(GetWorld(), this);
}
