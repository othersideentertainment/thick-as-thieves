// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Disguise/TATDisguiseComponent.h"

// tat
#include "Character/TATTeams.h"
#include "Player/TATCharacter.h"
#include "Character/TATCharacterAIBase.h"
#include "Disguise/TATDisguisableCharacterInterface.h"
#include "Items/Disguise/TATDisguiseToolComponent.h"
#include "Animation/TATCharacterAnimationMapping.h"
#include "Character/TATCharacterMovement.h"
#include "Tools/TATToolFunctionLibrary.h"
#include "Animation/TATAnimInstance.h"
#include "Developer/TATProjectSettings.h"
#include "Developer/TATDevToolSubsystem.h"
#include "Developer/TATImGuiHelpers.h"
#include "Player/TATPlayerState.h"

// ose
#include "OSECommon.h"
#include "Character/OSECharacterBase.h"
#include "Character/OSETeamInterface.h"
#include "Items/ToolSetComponent.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "EngineUtils.h"
#include "GameplayEffectExtension.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraWorldManager.h"
#include "Abilities/Attributes/AttributeBaseSet.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDisguiseComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATDisguiseComponent, Log, All)

// The duration that "instant" disguise events (eg. not per second) show up in the dev tools disguise window
static constexpr float kTATDisguiseMessageDurationSec = 10.0f;

#if TAT_ENABLE_DEV_TOOLS
// Constructs an instance of FTATDisguiseReductionEventDebugInfo in a way that gets compiled out when dev tools are not enabled.
#define TAT_DISGUISE_EVENT_DEBUG_INFO(SOURCE, DESC, VALUE, EVENT_IS_PER_FRAME) \
   FTATDisguiseReductionEventDebugInfo{ \
      .Source = (SOURCE), \
      .Desc = (DESC), \
      .Value = (VALUE), \
      .IsPerFrameEvent = (EVENT_IS_PER_FRAME), \
      .RemoveAtWorldTime = 0.0, \
   }
#else
#define TAT_DISGUISE_EVENT_DEBUG_INFO(SOURCE, DESC, VALUE, EVENT_IS_PER_FRAME) FTATDisguiseReductionEventDebugInfo{}
#endif

namespace DisguiseHelpers
{
   void SetAllSkeletalMeshComponentMaterials(USkeletalMeshComponent* comp, const TArray<UMaterialInterface*>& materials)
   {
      check(comp != nullptr);
      comp->EmptyOverrideMaterials();
      for (int32 i = 0; i < FMath::Max(materials.Num(), comp->GetNumOverrideMaterials()); i++)
      {
         comp->SetMaterial(i, materials.IsValidIndex(i) ? materials[i] : nullptr);
      }
   }

   bool AreCharactersInRange(ACharacter* a, ACharacter* b, float distance)
   {
      check(IsValid(a));
      check(IsValid(b));
      const float aOffset = a->GetCapsuleComponent() ? a->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
      const float bOffset = b->GetCapsuleComponent() ? b->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
      return FVector::Dist(a->GetActorLocation(), b->GetActorLocation()) < (distance + ((aOffset + bOffset) * 0.5f));
   };
}

void FTATDisguiseMeshParams::CopyFrom(ACharacter* character, ACharacter* fallbackCharacter, bool isDisguise)
{
   Reset();

   IsDisguise = isDisguise;

   ITATDisguiseTargetInterface* disguiseTargetInterface = Cast<ITATDisguiseTargetInterface>(character);
   if (disguiseTargetInterface == nullptr)
   {
      return;
   }

   // Third person meshes are required for a valid disguise
   SkeletonMesh.Mesh = disguiseTargetInterface->GetDisguiseTargetMesh(ETATDisguiseMeshType::Skeleton, SkeletonMesh.Materials);
   ThirdPersonBody.Mesh = disguiseTargetInterface->GetDisguiseTargetMesh(ETATDisguiseMeshType::ThirdPersonBody, ThirdPersonBody.Materials);
   ThirdPersonHead.Mesh = disguiseTargetInterface->GetDisguiseTargetMesh(ETATDisguiseMeshType::ThirdPersonHead, ThirdPersonHead.Materials);
   if (!ThirdPersonHead.IsValid())
   {
      UE_LOG(LogTATDisguiseComponent, Error, TEXT("Failed to get valid third person body mesh from disguise target character %s"), *character->GetName());
      ThirdPersonHead.Reset();
      return;
   }

   ITATDisguiseTargetInterface* fallbackTargetInterface = Cast<ITATDisguiseTargetInterface>(fallbackCharacter);
   auto getDisguiseMesh = [disguiseTargetInterface, fallbackTargetInterface](ETATDisguiseMeshType type, FTATDisguiseMeshData& outMeshData)
   {
      outMeshData.Mesh = disguiseTargetInterface->GetDisguiseTargetMesh(type, outMeshData.Materials);
      if (outMeshData.Mesh == nullptr && fallbackTargetInterface != nullptr)
      {
         outMeshData.Mesh = fallbackTargetInterface->GetDisguiseTargetMesh(type, outMeshData.Materials);
      }
   };
   getDisguiseMesh(ETATDisguiseMeshType::FirstPersonUpperBody, FirstPersonUpperBody);
   getDisguiseMesh(ETATDisguiseMeshType::FirstPersonLowerBody, FirstPersonLowerBody);

   AnimInstanceClass = disguiseTargetInterface->GetDisguiseTargetThirdPersonAnimClass();

   if (AnimInstanceClass == nullptr && fallbackTargetInterface != nullptr)
   {
      AnimInstanceClass = fallbackTargetInterface->GetDisguiseTargetThirdPersonAnimClass();
   }

   if (isDisguise)
   {
      AnimLayer = disguiseTargetInterface->GetDisguiseTargetAnimSetLayer();

      if (AnimLayer == nullptr)
      {
         if (fallbackTargetInterface)
         {
            AnimLayer = fallbackTargetInterface->GetDisguiseTargetAnimSetLayer();
         }
      }
   }
}

void FTATDisguiseMeshParams::Apply(ACharacter* character) const
{
   if (!IsValid() || character == nullptr)
   {
      return;
   }


   static constexpr auto applyMeshSimple = [](USkeletalMeshComponent* meshComp, const FTATDisguiseMeshData& mesh)
   {
      if(meshComp)
      {
         meshComp->SetSkeletalMeshAsset(mesh.Mesh);
         DisguiseHelpers::SetAllSkeletalMeshComponentMaterials(meshComp, mesh.Materials);
      }
   };
   auto applyFirstPersonMesh = [this](USkeletalMeshComponent* firstPersonMeshComp, const FTATDisguiseMeshData& mesh, UClass* animInstanceClass, USkeletalMeshComponent* thirdPersonMeshComp)
   {
      if(firstPersonMeshComp == nullptr)
      {
         return;
      }
      constexpr bool forceUpdate = true;

      if (mesh.Mesh == nullptr)
      {
         return;
      }

      if (IsDisguise)
      {
         firstPersonMeshComp->SetLeaderPoseComponent(nullptr, forceUpdate);
         firstPersonMeshComp->SetAnimInstanceClass(animInstanceClass);
         if (AnimLayer)
         {
            firstPersonMeshComp->LinkAnimClassLayers(AnimLayer);
         }

         // When disguised, we need a separate ABP on the first person mesh, but we don't want it to fire anim notifies
         // because that causes double events for things like footsteps.
         if (UTATAnimInstance* animInstance = Cast<UTATAnimInstance>(firstPersonMeshComp->GetAnimInstance()))
         {
            animInstance->SetAnimNotifiesEnabled(false);
         }
      }
      else
      {
         firstPersonMeshComp->SetLeaderPoseComponent(thirdPersonMeshComp, forceUpdate);
         firstPersonMeshComp->SetAnimInstanceClass(nullptr);
         if (AnimLayer)
         {
            firstPersonMeshComp->UnlinkAnimClassLayers(AnimLayer);
         }
      }
      applyMeshSimple(firstPersonMeshComp, mesh);
   };

   USkeletalMeshComponent* thirdPersonMeshComp = character->GetMesh();
   check(thirdPersonMeshComp != nullptr);
   UClass* origAnimInstanceClass = thirdPersonMeshComp->GetAnimClass();

   ATATCharacterBase* tatCharacter = Cast<ATATCharacterBase>(character);
   if (tatCharacter)
   {
      applyFirstPersonMesh(tatCharacter->GetMesh1P_UpperBody(), FirstPersonUpperBody, origAnimInstanceClass, thirdPersonMeshComp);
      applyMeshSimple(tatCharacter->GetMesh1P_LowerBody(), FirstPersonLowerBody);
   }

   if (ensure(SkeletonMesh.Mesh != nullptr))
   {
      constexpr bool reinitPose = true;
      thirdPersonMeshComp->SetSkeletalMesh(SkeletonMesh.Mesh, reinitPose);
      thirdPersonMeshComp->SetAnimInstanceClass(AnimInstanceClass);

      if (tatCharacter)
      {
         tatCharacter->SuppressAnimSets(IsDisguise);
      }
      if (AnimLayer)
      {
         thirdPersonMeshComp->LinkAnimClassLayers(AnimLayer);
      }
      DisguiseHelpers::SetAllSkeletalMeshComponentMaterials(thirdPersonMeshComp, SkeletonMesh.Materials);
   }

   if (tatCharacter)
   {
      applyMeshSimple(tatCharacter->GetMesh3P_Body(), ThirdPersonBody);
      applyMeshSimple(tatCharacter->GetMesh3P_Head(), ThirdPersonHead);
   }
}

void FTATDisguiseMeshParams::Unapply(ACharacter* character) const
{
   if (character == nullptr)
   {
      return;
   }

   USkeletalMeshComponent* mainMeshComp = character->GetMesh();
   check(mainMeshComp != nullptr);

   if (AnimLayer)
   {
      mainMeshComp->UnlinkAnimClassLayers(AnimLayer);
   }

   mainMeshComp->EmptyOverrideMaterials();

   auto tryClearOverrides = [] (USkeletalMeshComponent* meshComp)
   {
      if(meshComp)
      {
         meshComp->EmptyOverrideMaterials();
      }
   };

   if (ATATCharacterBase* tatCharacter = Cast<ATATCharacterBase>(character))
   {
      tryClearOverrides(tatCharacter->GetMesh3P_Head());
      tryClearOverrides(tatCharacter->GetMesh3P_Body());
   }
}

void FTATDisguiseLocalClientActiveEffectContainer::DestroyAllEffects()
{
   if (IsValid(RadiusEffectParticles))
   {
      RadiusEffectParticles->ReleaseToPool();
      RadiusEffectParticles = nullptr;
   }
}

UTATDisguiseComponent::UTATDisguiseComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;

   SetIsReplicatedByDefault(true);
}

void UTATDisguiseComponent::BeginPlay()
{
   Super::BeginPlay();

   _character = Cast<ATATCharacter>(GetOwner());

   // Cache the max disguise integrity value from the disguise tool
   if (UTATDisguiseToolComponent* disguiseTool = _GetDisguiseToolComponent())
   {
      _maxDisguiseIntegrity = disguiseTool->MaxDisguiseIntegrity;
   }
   else
   {
      // failsafe default value
      _maxDisguiseIntegrity = 100.0f;
   }

   // Get a reference to the component we should use to show coop partners that we're disguised
   if (_character)
   {
      _disguisedPlayerCoopIndicatorComponent = _character->GetDisguisedPlayerCoopIndicatorComponent();

      // Always default to inactive
      if (_disguisedPlayerCoopIndicatorComponent != nullptr)
      {
         _disguisedPlayerCoopIndicatorComponent->SetActive(false);
      }
   }

   if (UTATDevToolSubsystem* devToolSubsystem = GetWorld()->GetSubsystem<UTATDevToolSubsystem>())
   {
      static const FName devToolName = FName(TEXT("Disguise Component"));
      devToolSubsystem->RegisterDevToolObject(this, devToolName);
   }
}

void UTATDisguiseComponent::EndPlay(const EEndPlayReason::Type reason)
{
   if (UTATDevToolSubsystem* devToolSubsystem = GetWorld()->GetSubsystem<UTATDevToolSubsystem>())
   {
      devToolSubsystem->UnregisterDevToolObject(this);
   }

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      for (const auto& pair : _gameplayTagChangeEventInfos)
      {
         asc->UnregisterGameplayTagEvent(pair.Value.Handle, pair.Key, EGameplayTagEventType::NewOrRemoved);
      }
      _gameplayTagChangeEventInfos.Empty();

      for (const auto& pair : _gameplayEventInfos)
      {
         asc->GenericGameplayEventCallbacks.FindOrAdd(pair.Key).Remove(pair.Value.Handle);
      }
      _gameplayEventInfos.Empty();

      for (const auto& pair : _gameplayAttributeDecreaseEventInfos)
      {
         asc->GetGameplayAttributeValueChangeDelegate(pair.Key).Remove(pair.Value.Handle);
      }
      _gameplayAttributeDecreaseEventInfos.Empty();

      if (_abilityActivatedDelegate.IsValid())
      {
         asc->AbilityActivatedCallbacks.Remove(_abilityActivatedDelegate);
         _abilityActivatedDelegate.Reset();
      }
   }

   if (GetOwner()->HasAuthority())
   {
      _AuthorityRemoveDisguiseEffects();
   }
   
   if (_IsLocalClient())
   {
      _LocalClientEndPlay();
   }

   if (_IsCoopPartner() && _disguisedPlayerCoopIndicatorComponent != nullptr)
   {
      _disguisedPlayerCoopIndicatorComponent->SetActive(false);
   }

   Super::EndPlay(reason);
}

void UTATDisguiseComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);
   
   if (_IsLocalClient())
   {
      _LocalClientTickComponent(deltaTime);
   }
   else if (_IsCoopPartner())
   {
      _CoopPartnerTickComponent(deltaTime);
   }

   if (!_currentDisguiseState.IsDisguiseActive)
   {
      return;
   }

   // tick should only be enabled on servers and local clients (and local client tick was already handled above)
   if (!GetOwner()->HasAuthority())
   {
      return;
   }

   const FTATDisguiseIntegrityResult integrityDeltaTime{ deltaTime };

   float integrity = _currentDisguiseIntegrity;

   // Apply any "events" that don't depend on general state
   for (const FDisguiseEventInfo& reduceOverTimeEvt : _alwaysReduceOverTimeEvents)
   {
      _HandleIntegrityReductionEventInternal(
         TAT_DISGUISE_EVENT_DEBUG_INFO(TEXT("AlwaysReduceOverTime"), TEXT(""), reduceOverTimeEvt.Result.ToDebugValue(), true),
         integrity, reduceOverTimeEvt.Result * integrityDeltaTime, reduceOverTimeEvt);
   }

   // Check for any gameplay tags that have reduce-over-time effects
   if (!_gameplayTagReduceOverTimeEventInfos.IsEmpty())
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         for (const FDisguiseEventInfo_GameplayTag& info : _gameplayTagReduceOverTimeEventInfos)
         {
            if (asc->HasMatchingGameplayTag(info.Tag))
            {
               _HandleIntegrityReductionEventInternal(
                  TAT_DISGUISE_EVENT_DEBUG_INFO(TEXT("GameplayTagReduceOverTime"), info.Tag.ToString(), info.EventInfo.Result.ToDebugValue(), true),
                  integrity, info.EventInfo.Result * integrityDeltaTime, info.EventInfo);
            }
         }
      }
   }

   // Find nearby characters if we need to check proximity
   if (!_characterProximityEventInfos.IsEmpty() || !_characterViewConeEventInfos.IsEmpty())
   {
      // Determine the search radius of nearby characters we care about
      float searchRadius = 0.0f;
      for (const FDisguiseEventInfo_Character& info : _characterProximityEventInfos)
      {
         // early out if we don't meet the stealth score requirement
         if (!_AuthorityMeetsStealthScoreRequirements(info.EventInfo.StealthScoreRange))
         {
            continue;
         }
         searchRadius = FMath::Max(searchRadius, info.Radius);
      }
      for (const FDisguiseEventInfo_Character& info : _characterViewConeEventInfos)
      {
         // early out if we don't meet the stealth score requirement
         if (!_AuthorityMeetsStealthScoreRequirements(info.EventInfo.StealthScoreRange))
         {
            continue;
         }
         searchRadius = FMath::Max(searchRadius, info.Radius);
      }
      searchRadius = (searchRadius > 0) ? FMath::Max(1000.0f, searchRadius * 2.0f) : 0.0f;

      _UpdateNearbyCharacters(_authorityNearbyCharacters, searchRadius);

      if (!_authorityNearbyCharacters.IsEmpty())
      {
         ACharacter* ownerCharacter = Cast<ACharacter>(GetOwner());
         if (ensure(ownerCharacter != nullptr))
         {
            for (ACharacter* nearbyCharacter : _authorityNearbyCharacters)
            {
               // Do proximity checks
               for (const FDisguiseEventInfo_Character& info : _characterProximityEventInfos)
               {
                  if (!_ShouldHandleNearbyCharacter(nearbyCharacter, info.CharacterType))
                  {
                     continue;
                  }
#if TAT_ENABLE_DEV_TOOLS
                  if (_devToolState.DebugDraw)
                  {
                     const FVector yAxis{ 0, 1, 0 };
                     const FVector zAxis{ 1, 0, 0 };
                     constexpr bool drawAxis = true;
                     DrawDebugCircle(GetWorld(), nearbyCharacter->GetActorLocation(), info.Radius, 24, FColor::White,
                        false, -1, 0, 0, yAxis, zAxis, drawAxis);
                  }
#endif
                  if (DisguiseHelpers::AreCharactersInRange(ownerCharacter, nearbyCharacter, info.Radius))
                  {
                     _HandleIntegrityReductionEventInternal(
                        TAT_DISGUISE_EVENT_DEBUG_INFO(TEXT("CharacterProximity"), nearbyCharacter->GetName(), info.EventInfo.Result.ToDebugValue(), true),
                        integrity, info.EventInfo.Result * integrityDeltaTime, info.EventInfo);
                  }
               }

               // Do view cone checks
               for (const FDisguiseEventInfo_Character& info : _characterViewConeEventInfos)
               {
                  if (!_ShouldHandleNearbyCharacter(nearbyCharacter, info.CharacterType))
                  {
                     continue;
                  }
                  if (!ensure(info.ViewConeAngleDegrees.IsSet()))
                  {
                     continue;
                  }
                  // Line of sight check
                  FTATLineOfSightTraceParams params{};
                  params.MaxDistance = info.Radius;
                  params.RequireSourceActorConscious = true;
                  params.UseAISightToLimitMaxDistance = info.UseAISightToLimitMaxViewConeDistance;
                  params.MaxAngleDeg = info.ViewConeAngleDegrees.GetValue();
                  params.UseFrustumCheckForViewAngle = true;
                  params.TraceProfile.Name = UCollisionProfile::BlockAll_ProfileName;
                  FHitResult hitResult{};
#if TAT_ENABLE_DEV_TOOLS
                  const bool debugDraw = _devToolState.DebugDraw;
#else
                  constexpr bool debugDraw = false;
#endif
                  const bool inViewCone = UTATToolFunctionLibrary::PerformLineOfSightTrace(hitResult, nearbyCharacter, ownerCharacter, params, debugDraw);
                  if (inViewCone)
                  {
                     _HandleIntegrityReductionEventInternal(
                        TAT_DISGUISE_EVENT_DEBUG_INFO(TEXT("CharacterViewCone"), nearbyCharacter->GetName(), info.EventInfo.Result.ToDebugValue(), true),
                        integrity, info.EventInfo.Result * integrityDeltaTime, info.EventInfo);
                  }
               }
            }
         }
      }
   }

   if (!FMath::IsNearlyEqual(_currentDisguiseIntegrity, integrity))
   {
      AuthoritySetDisguiseIntegrity(integrity);
   }
}

void UTATDisguiseComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UTATDisguiseComponent, _currentDisguiseState)
   // TODO: Can this be owner-only?
   DOREPLIFETIME(UTATDisguiseComponent, _currentDisguiseIntegrity)
}

void UTATDisguiseComponent::DrawDevToolObjectEditor(float deltaSeconds)
{
#if TAT_ENABLE_DEV_TOOLS
   TATImGui::Text(TEXT("Disguise Active: %s"), _currentDisguiseState.IsDisguiseActive ? TEXT("true") : TEXT("false"));
   ImGui::SameLine();
   TATImGui::Text(TEXT("ComponentTick: %s"), IsComponentTickEnabled() ? TEXT("enabled") : TEXT("disabled"));
   ImGui::SameLine();
   ImGui::Checkbox("DebugDraw", &_devToolState.DebugDraw);
   ImGui::SameLine();
   ImGui::Checkbox("Show Stealth Score", &_devToolState.ShowStealthScore);

   TATImGui::TextUnformatted(TEXT("Disguise Integrity"));
   ImGui::SameLine();
   TATImGui::ProgressBar(
      GetNormalizedRemainingDisguiseIntegrity(),
      FVector2f(ImGui::GetContentRegionAvail().x, 0.0f),
      FString::Printf(TEXT("%.2f"),
         _currentDisguiseIntegrity));
   
   if (_devToolState.ShowStealthScore)
   {
      TATImGui::TextUnformatted(TEXT("Stealth Score"));
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(40, 80, 200, 255));
      ATATCharacter* character = Cast<ATATCharacter>(GetOwner());
      const float stealthScore = (character != nullptr) ? character->GetStealthScore() : 0.0f;
      TATImGui::ProgressBar(stealthScore, FVector2f(ImGui::GetContentRegionAvail().x, 0.0f), FString::Printf(TEXT("%.2f"), stealthScore));
      ImGui::PopStyleColor();
   }
   
   if (ImGui::BeginTabBar("##tab-bar"))
   {
      if (ImGui::BeginTabItem("Disguise Events (Server-Only)"))
      {
         if (ImGui::BeginTable("##EventTable", 4, ImGuiTableFlags_Resizable))
         {
            ImGui::TableSetupColumn("Source");
            ImGui::TableSetupColumn("Desc");
            ImGui::TableSetupColumn("Amount");
            ImGui::TableSetupColumn("Message Timer");

            ImGui::TableHeadersRow();

            for (const FTATDisguiseReductionEventDebugInfo& info : _devToolState.IntegrityReductionEvents)
            {
               if (ImGui::TableNextColumn())
               {
                  TATImGui::TextUnformatted(info.Source);
               }
               if (ImGui::TableNextColumn())
               {
                  TATImGui::TextUnformatted(info.Desc);
               }
               if (ImGui::TableNextColumn())
               {
                  if (info.IsPerFrameEvent)
                  {
                     ImGui::Text("%.2f / sec", info.Value);
                  }
                  else
                  {
                     ImGui::Text("%.2f", info.Value);
                  }
               }
               if (ImGui::TableNextColumn())
               {
                  const float timeRemaining = FMath::Max(0.0f, info.RemoveAtWorldTime - GetWorld()->GetTimeSeconds());
                  const float timeRemainingNormalized = FMath::Clamp(timeRemaining / kTATDisguiseMessageDurationSec, 0.0f, 1.0f);
                  if (timeRemainingNormalized > 0)
                  {
                     ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(40, 80, 200, 127));
                     ImGui::ProgressBar(timeRemainingNormalized, { 0, 0 }, "");
                     ImGui::PopStyleColor();
                  }
                  else
                  {
                     ImGui::Dummy({ 0, 0 });
                  }
               }
            }
            ImGui::EndTable();
         }
         ImGui::EndTabItem();
      }

      // Show local client debugging data
      if (_IsLocalClient())
      {
         if (ImGui::BeginTabItem("Local Client State"))
         {
            ImGui::BeginDisabled(true);
            ImGui::Checkbox("_hasRunLocalClientInit", &_hasRunLocalClientInit);
            ImGui::EndDisabled();

            ImGui::Text("Configured VFX Hooks: %i", _characterProximityVFXInfos.Num());
            ImGui::Text("Tracked Nearby Characters: %i", _localPlayerNearbyCharacters.Num());

            if (ImGui::BeginTable("##nearby-characters", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
            {
               for (int32 i = 0; i < _localPlayerNearbyCharacters.Num(); i++)
               {
                  TATImGui::FScopedID _id{ i };
                  if (!_localPlayerNearbyCharacters[i])
                  {
                     if (ImGui::TableNextColumn())
                     {
                        ImGui::Text("NULL");
                     }
                     ImGui::TableNextColumn();
                  }
                  else
                  {
                     if (ImGui::TableNextColumn())
                     {
                        TATImGui::TextUnformatted(_localPlayerNearbyCharacters[i]->GetName());
                     }
                     if (ImGui::TableNextColumn())
                     {
                        if (FTATDisguiseLocalClientActiveEffectContainer* activeEffects = _nearbyCharacterEffects.Find(_localPlayerNearbyCharacters[i]))
                        {
                           if (activeEffects->RadiusEffectParticles != nullptr)
                           {
                              TATImGui::TextUnformatted(activeEffects->RadiusEffectParticles->GetName());
                           }
                        }
                     }
                  }
               }
               ImGui::EndTable();
            }

            ImGui::EndTabItem();
         }
      }

      const UTATDisguiseToolComponent* disguiseTool = _GetDisguiseToolComponent();
      if (disguiseTool != nullptr && disguiseTool->DisguiseIntegrityReductionTable)
      {
         if (ImGui::BeginTabItem("Integrity Reduction Data Table"))
         {
            ImGui::Checkbox("Show All", &_devToolState.ShowAllDataTableRows);
            if (ImGui::BeginTable("##data-table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
            {
               ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.1f);
               ImGui::TableSetupColumn("Event Type", ImGuiTableColumnFlags_WidthStretch, 0.25f);
               ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, 32.0f);
               ImGui::TableSetupColumn("Integrity Reduction", ImGuiTableColumnFlags_WidthFixed, 50.0);
               ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.5f);
               ImGui::TableHeadersRow();

               int32 rowIndex = 0;
               disguiseTool->DisguiseIntegrityReductionTable->ForeachRow<FTATDisguiseIntegrityReductionDataRow>(
                  TEXT("DisguiseIntegrityReduction_DevToolDisplay"),
                  [&](const FName& key, const FTATDisguiseIntegrityReductionDataRow& dataRow)
               {
                  if (!_devToolState.ShowAllDataTableRows && !dataRow.Enabled)
                  {
                     return;
                  }

                  TATImGui::FScopedID _id{ rowIndex++ };
                  ImGui::TableNextRow();
                  if (ImGui::TableNextColumn())
                  {
                     TATImGui::TextUnformatted(key.ToString());
                  }
                  if (ImGui::TableNextColumn())
                  {
                     TATImGui::TextUnformatted(StaticEnum<ETATDisguiseIntegrityEvent>()->GetNameStringByValue(static_cast<int64>(dataRow.EventType)));
                  }
                  if (ImGui::TableNextColumn())
                  {
                     ImGui::BeginDisabled(true);
                     ImGui::Checkbox("##enabled", const_cast<bool*>(&dataRow.Enabled));
                     ImGui::EndDisabled();
                  }
                  if (ImGui::TableNextColumn())
                  {
                     TATImGui::TextUnformatted(dataRow.GetIntegrityReductionDebugValue());
                  }
                  if (ImGui::TableNextColumn())
                  {
                     TATImGui::TextUnformatted(dataRow.GetDebugDescription());
                  }
               });

               ImGui::EndTable();
            }
            ImGui::EndTabItem();
         }
      }

      ImGui::EndTabBar();
   }

   const double worldTimeSeconds = GetWorld()->GetTimeSeconds();
   _devToolState.IntegrityReductionEvents.RemoveAll([worldTimeSeconds](const FTATDisguiseReductionEventDebugInfo& info)
   {
      return info.RemoveAtWorldTime != 0 && info.RemoveAtWorldTime <= worldTimeSeconds;
   });
#endif // TAT_ENABLE_DEV_TOOLS
}

void UTATDisguiseComponent::_AuthorityBindToGameplayEvents()
{
   check(GetOwner()->HasAuthority());

   if (_authorityHasBoundToGameplayEvents)
   {
      return;
   }

   _disableAbilitiesTag = UTATProjectSettings::Get().DisableAbilitiesTag;

   const UTATDisguiseToolComponent* disguiseTool = _GetDisguiseToolComponent();
   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
   if (disguiseTool == nullptr || asc == nullptr)
   {
      return;
   }

   bool bindAbilityEvents = false;

   if (disguiseTool->DisguiseIntegrityReductionTable == nullptr)
   {
      _abilityActivatedDelegate = asc->AbilityActivatedCallbacks.AddUObject(this, &UTATDisguiseComponent::_OnAbilityActivated);
      return;
   }

   disguiseTool->DisguiseIntegrityReductionTable->ForeachRow<FTATDisguiseIntegrityReductionDataRow>(TEXT("DisguiseIntegrityReduction"),
      [&](const FName& key, const FTATDisguiseIntegrityReductionDataRow& dataRow)
   {
      if (!dataRow.Enabled)
      {
         return;
      }

      switch (dataRow.EventType)
      {
      case ETATDisguiseIntegrityEvent::AlwaysReduceOverTime:
         {
            FDisguiseEventInfo eventInfo = _MakeDisguiseEventInfo(dataRow);
            if (eventInfo.Result)
            {
               _alwaysReduceOverTimeEvents.Add(MoveTemp(eventInfo));
            }
         }
         break;
      case ETATDisguiseIntegrityEvent::TagPresenceReduceOverTime:
         if (dataRow.GameplayTag.IsValid() && dataRow.IntegrityReduction != 0)
         {
            FDisguiseEventInfo eventInfo = _MakeDisguiseEventInfo(dataRow);
            // In case an integrity reduction curve results in a reduction of zero, only add the info to the array if we have a reduction to apply.
            // Because these are checked on tick, the less gameplay tags to check, the better.
            if (eventInfo.Result)
            {
               _gameplayTagReduceOverTimeEventInfos.Add({MoveTemp(eventInfo), dataRow.GameplayTag});
            }
         }
         break;
      case ETATDisguiseIntegrityEvent::TagAdded:
         // fallthrough
      case ETATDisguiseIntegrityEvent::TagRemoved:
         if (dataRow.GameplayTag.IsValid())
         {
            FDisguiseGameplayTagChangeInfo tagEventInfo;
            tagEventInfo.Handle = asc->RegisterGameplayTagEvent(dataRow.GameplayTag, EGameplayTagEventType::NewOrRemoved)
               .AddUObject(this, &UTATDisguiseComponent::_OnReductionGameplayTagNewOrRemoved);
            tagEventInfo.TriggerOnTagAdded = dataRow.EventType == ETATDisguiseIntegrityEvent::TagAdded;
            tagEventInfo.EventInfo = _MakeDisguiseEventInfo(dataRow);
            _gameplayTagChangeEventInfos.Add(dataRow.GameplayTag, tagEventInfo);
         }
         break;

      case ETATDisguiseIntegrityEvent::GameplayEvent:
         if (dataRow.GameplayEvent.IsValid())
         {
            if (!_gameplayEventInfos.Contains(dataRow.GameplayEvent))
            {
               FDelegateHandle eventHandle = asc->GenericGameplayEventCallbacks.FindOrAdd(dataRow.GameplayEvent)
                  .AddUObject(this, &UTATDisguiseComponent::_OnReductionGameplayEvent);
               _gameplayEventInfos.Add(dataRow.GameplayEvent, { _MakeDisguiseEventInfo(dataRow), eventHandle});
            }
            else
            {
               UE_LOG(LogTATDisguiseComponent, Error, TEXT("Failed to register disguise event: got duplicate gameplay event tag '%s' in data table '%s'"),
                  *dataRow.GameplayEvent.ToString(), *disguiseTool->DisguiseIntegrityReductionTable->GetName());
            }
         }
         break;

      case ETATDisguiseIntegrityEvent::AttributeDecrease:
         if (dataRow.AttributeDecrease.GameplayAttribute.IsValid())
         {
            FDisguiseAttributeChangeInfo attrChangeInfo;
            attrChangeInfo.EventInfo = _MakeDisguiseEventInfo(dataRow);
            attrChangeInfo.Handle = asc->GetGameplayAttributeValueChangeDelegate(dataRow.AttributeDecrease.GameplayAttribute)
                  .AddUObject(this, &UTATDisguiseComponent::_OnReductionAttributeChange);
            attrChangeInfo.AttributeDeltaMultiplier = dataRow.AttributeDecrease.AttributeDeltaMultiplier;
            attrChangeInfo.MaxExtraReductionFromAttributeDelta = dataRow.AttributeDecrease.MaxExtraReductionFromAttributeDelta;
            _gameplayAttributeDecreaseEventInfos.Add(dataRow.AttributeDecrease.GameplayAttribute, attrChangeInfo);
         }
         break;

      case ETATDisguiseIntegrityEvent::AbilityStarted:
         bindAbilityEvents = true;
         _abilityEventInfos.Add(FDisguiseAbilityInfo{ _MakeDisguiseEventInfo(dataRow), dataRow.AbilityStarted });
         break;

      case ETATDisguiseIntegrityEvent::NearbyCharacterState:
         switch (dataRow.NearbyCharacterState.Mode)
         {
         case ETATDisguiseIntegrityNearbyCharacterMode::CharacterProximity:
            _characterProximityEventInfos.Add(FDisguiseEventInfo_Character{
               .CharacterType = dataRow.NearbyCharacterState.CharacterType,
               .EventInfo = _MakeDisguiseEventInfo(dataRow),
               .Radius = FMath::Max(0.0f, dataRow.NearbyCharacterState.CharacterRadius),
               .ViewConeAngleDegrees = {},
               .UseAISightToLimitMaxViewConeDistance = false,
            });
            break;
         case ETATDisguiseIntegrityNearbyCharacterMode::CharacterViewCone:
            _characterViewConeEventInfos.Add(FDisguiseEventInfo_Character{
               .CharacterType = dataRow.NearbyCharacterState.CharacterType,
               .EventInfo = _MakeDisguiseEventInfo(dataRow),
               .Radius = FMath::Max(0.0f, dataRow.NearbyCharacterState.CharacterRadius),
               .ViewConeAngleDegrees = FMath::Clamp(dataRow.NearbyCharacterState.CharacterViewConeAngleDegrees, 0.0f, 360.0f),
               .UseAISightToLimitMaxViewConeDistance = dataRow.NearbyCharacterState.UseAISightToLimitMaxViewConeDistance,
            });
            break;
         //TODO: Implement this
         // case ETATDisguiseIntegrityNearbyCharacterMode::CharacterInvestigating:
         //    break;
         default:
            checkNoEntry();
         }
         break;
      default:
         checkNoEntry();
         break;
      }
   });

   // If we care about ability events, listen for them
   if (bindAbilityEvents)
   {
      _abilityActivatedDelegate = asc->AbilityActivatedCallbacks.AddUObject(this, &UTATDisguiseComponent::_OnAbilityActivated);
   }

   _authorityHasBoundToGameplayEvents = true;
}

void UTATDisguiseComponent::_AuthorityUnbindToGameplayEvents()
{
   check(GetOwner()->HasAuthority());

#if 0
   const UTATDisguiseToolComponent* disguiseTool = _GetDisguiseToolComponent();
   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
   if (disguiseTool == nullptr || asc == nullptr )
   {
      return;
   }

   if (_abilityActivatedDelegate.IsValid())
   {
      asc->AbilityActivatedCallbacks.Remove(_abilityActivatedDelegate);
      _abilityActivatedDelegate.Reset();
   }
#endif
}

bool UTATDisguiseComponent::AuthorityActivateDisguise(const FDisguiseSnapshot& disguiseSnapshot, AActor* caster)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogTATDisguiseComponent, Warning, TEXT("AuthorityActivateDisguise called on UTATDisguiseComponent on '%s' despite not being an authority"),
         *GetOwner()->GetName());
      return false;
   }

   _currentDisguiseState.CurrentDisguiseData = disguiseSnapshot;
   _currentDisguiseState.OriginalTeam = GetOwner<IOSETeamInterface>()->GetOriginalTeam();
   _currentDisguiseState.IsDisguiseActive = true;
   _currentDisguiseState.ServerStartTimeSeconds = GetWorld()->GetTimeSeconds();
   _currentDisguiseState.MaxDurationSeconds = _disguiseMaxDuration;

   _currentDisguiseIntegrity = _maxDisguiseIntegrity;

   _AuthorityApplyDisguiseEffects(caster);
   
   _OnDisguiseBegin();

   _AuthorityBindToGameplayEvents();
   
   SetComponentTickEnabled(true);

   return true;
}

void UTATDisguiseComponent::AuthorityReduceDisguiseIntegrity(float reduction)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogTATDisguiseComponent, Warning, TEXT("AuthorityReduceDisguiseIntegrity called on UTATDisguiseComponent on '%s' despite not being an authority"),
         *GetOwner()->GetName());
      return;
   }

   if (reduction == 0)
   {
      return;
   }
   AuthoritySetDisguiseIntegrity(_currentDisguiseIntegrity - reduction);
}

void UTATDisguiseComponent::AuthoritySetDisguiseIntegrity(float integrity)
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogTATDisguiseComponent, Warning, TEXT("AuthoritySetDisguiseIntegrity called on UTATDisguiseComponent on '%s' despite not being an authority"),
         *GetOwner()->GetName());
      return;
   }
   _currentDisguiseIntegrity = FMath::Clamp(integrity, 0.0f, _maxDisguiseIntegrity);
   OnDisguiseIntegrityChange.Broadcast(_currentDisguiseIntegrity, GetNormalizedRemainingDisguiseIntegrity());
   if (_currentDisguiseIntegrity <= 0.0f)
   {
      AuthorityEndDisguise();
   }
}

bool UTATDisguiseComponent::IsDisguiseActive() const
{
   return _currentDisguiseState.IsDisguiseActive;
}

const FDisguiseSnapshot& UTATDisguiseComponent::GetDisguiseSnapshot() const
{
   return _currentDisguiseState.CurrentDisguiseData;
}

void UTATDisguiseComponent::AuthorityEndDisguise()
{
   if (!GetOwner()->HasAuthority())
   {
      UE_LOG(LogTATDisguiseComponent, Warning, TEXT("AuthorityEndDisguise called on UTATDisguiseComponent on '%s' despite not being an authority"),
         *GetOwner()->GetName());
      return;
   }

   if (!_currentDisguiseState.IsDisguiseActive)
   {
      UE_LOG(LogTATDisguiseComponent, Warning, TEXT("AuthorityEndDisguise called on UTATDisguiseComponent on '%s' but a disguise was not active"),
         *GetOwner()->GetName());
      return;
   }

   SetComponentTickEnabled(false);

   _currentDisguiseIntegrity = 0.0f;
   _currentDisguiseState.IsDisguiseActive = false;

   _AuthorityRemoveDisguiseEffects();

   _AuthorityUnbindToGameplayEvents();
   
   _OnDisguiseBroken();
}

float UTATDisguiseComponent::GetRemainingDisguiseIntegrity() const
{
   return _currentDisguiseIntegrity;
}

float UTATDisguiseComponent::GetNormalizedRemainingDisguiseIntegrity() const
{
   return _currentDisguiseIntegrity / FMath::Max(0.01f, _maxDisguiseIntegrity);
}

uint8 UTATDisguiseComponent::GetTeamForDisguise() const
{
   return UTATTeamAttitudeSolver::MakeDisguiseTeam(_currentDisguiseState.CurrentDisguiseData.Team, _currentDisguiseState.OriginalTeam);
}

UAnimMontage* UTATDisguiseComponent::GetDisguisedCharacterMontage(const FGameplayTag& animationTag) const
{
   if (_disguisedCharacterAnimationMapping != nullptr)
   {
      return _disguisedCharacterAnimationMapping->LookupMontageByTag(animationTag);
   }
   return nullptr;
}

bool UTATDisguiseComponent::HandleToolLinkAnimClassLayers(UTATToolComponent* toolComponent, const TSubclassOf<UAnimInstance>& animClassLayer)
{
   if (!_currentDisguiseState.IsDisguiseActive)
   {
      return false;
   }

   AOSEPlayerCharacter1P* osePlayerCharacter = Cast<AOSEPlayerCharacter1P>(GetOwner());
   if (osePlayerCharacter == nullptr || osePlayerCharacter->GetMesh3P() == nullptr)
   {
      return false;
   }

   // For the disguise tool specifically, if we have an anim layer for the character we're disguised as, use that instead of the tool's anim layer.
   // This lets us treat the disguise tool as a "behave exactly like the character" mode
   check(toolComponent != nullptr);
   if (toolComponent->IsA<UTATDisguiseToolComponent>() && _disguisedMeshParams.AnimLayer)
   {
      osePlayerCharacter->GetMesh3P()->LinkAnimClassLayers(_disguisedMeshParams.AnimLayer);
      _lastAnimLinkedToolComponent = toolComponent;
      _lastAnimLinkedToolComponentAnimClassLayer = _disguisedMeshParams.AnimLayer;
      return true;
   }

   if (animClassLayer && osePlayerCharacter->GetMesh1P_UpperBody() && osePlayerCharacter->GetMesh1P_UpperBody()->LeaderPoseComponent == nullptr)
   {
      osePlayerCharacter->GetMesh3P()->LinkAnimClassLayers(animClassLayer);
      osePlayerCharacter->GetMesh1P_UpperBody()->LinkAnimClassLayers(animClassLayer);
      _lastAnimLinkedToolComponent = toolComponent;
      _lastAnimLinkedToolComponentAnimClassLayer = animClassLayer;
      return true;
   }

   return false;
}

bool UTATDisguiseComponent::HandleToolUnlinkAnimClassLayers(UTATToolComponent* toolComponent, const TSubclassOf<UAnimInstance>& animClassLayer)
{
   if (!_currentDisguiseState.IsDisguiseActive)
   {
      return false;
   }

   AOSEPlayerCharacter1P* osePlayerCharacter = Cast<AOSEPlayerCharacter1P>(GetOwner());
   if (osePlayerCharacter == nullptr)
   {
      return false;
   }

   if (animClassLayer && osePlayerCharacter->GetMesh1P_UpperBody() && osePlayerCharacter->GetMesh1P_UpperBody()->LeaderPoseComponent == nullptr)
   {
      if (osePlayerCharacter->GetMesh3P())
      {
         osePlayerCharacter->GetMesh3P()->UnlinkAnimClassLayers(animClassLayer);
      }
      osePlayerCharacter->GetMesh1P_UpperBody()->UnlinkAnimClassLayers(animClassLayer);
      _lastAnimLinkedToolComponent.Reset();
      _lastAnimLinkedToolComponentAnimClassLayer = nullptr;
      return true;
   }

   return false;
}

bool UTATDisguiseComponent::_IsCoopPartner() const
{
   if (GetNetMode() == NM_DedicatedServer)
   {
      return false;
   }

   APlayerController* localController = UOSECommon::GetLocalPlayerController<APlayerController>(this);
   if (localController == nullptr)
   {
      return false;
   }

   APawn* disguiseOwnerPawn = Cast<APawn>(GetOwner());
   if (disguiseOwnerPawn == nullptr || disguiseOwnerPawn->IsLocallyControlled() || disguiseOwnerPawn->GetController() == localController)
   {
      return false;
   }
   
   APawn* localPawn = localController->GetPawn();
   if (localPawn == nullptr)
   {
      return false;
   }

   return UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(disguiseOwnerPawn, localPawn, ETATTeamDisguiseHandling::UseOriginalTeam) == EOSETeamAttitude::Friendly;
}

void UTATDisguiseComponent::_CoopPartnerTickComponent(float deltaTime)
{
   check(_IsCoopPartner());

   if (_disguisedPlayerCoopIndicatorComponent == nullptr)
   {
      return;
   }

   if (_currentDisguiseState.IsDisguiseActive != _disguisedPlayerCoopIndicatorComponent->IsActive())
   {
      _disguisedPlayerCoopIndicatorComponent->SetActive(_currentDisguiseState.IsDisguiseActive);
   }
}

bool UTATDisguiseComponent::_IsLocalClient() const
{
   if (GetNetMode() == NM_DedicatedServer)
   {
      return false;
   }
   APawn* owner = Cast<APawn>(GetOwner());
   return (owner != nullptr) ? owner->IsLocallyControlled() : false;
}

void UTATDisguiseComponent::_LocalClientInit()
{
   check(_IsLocalClient());

   if (_hasRunLocalClientInit)
   {
      return;
   }
   
   const UTATDisguiseToolComponent* disguiseTool = _GetDisguiseToolComponent();
   if (disguiseTool == nullptr)
   {
      return;
   }

   disguiseTool->DisguiseIntegrityReductionTable->ForeachRow<FTATDisguiseIntegrityReductionDataRow>(TEXT("DisguiseIntegrityReduction"),
      [&](const FName& key, const FTATDisguiseIntegrityReductionDataRow& dataRow)
      {
         if (!dataRow.Enabled)
         {
            return;
         }
         switch (dataRow.EventType)
         {
         case ETATDisguiseIntegrityEvent::NearbyCharacterState:
            if (dataRow.NearbyCharacterState.Mode == ETATDisguiseIntegrityNearbyCharacterMode::CharacterProximity)
            {
               _characterProximityVFXInfos.Add(FDisguiseVFXInfo_Character{
                  .CharacterType = dataRow.NearbyCharacterState.CharacterType,
                  .Radius = dataRow.NearbyCharacterState.CharacterRadius,
                  .ViewConeAngleDeg = NullOpt,
               });
            }
            else if (dataRow.NearbyCharacterState.Mode == ETATDisguiseIntegrityNearbyCharacterMode::CharacterViewCone)
            {
               _characterProximityVFXInfos.Add(FDisguiseVFXInfo_Character{
                  .CharacterType = dataRow.NearbyCharacterState.CharacterType,
                  .Radius = dataRow.NearbyCharacterState.CharacterRadius,
                  .ViewConeAngleDeg = dataRow.NearbyCharacterState.CharacterViewConeAngleDegrees,
               });
            }
            break;
         default:
            break;
         }
      });

   _hasRunLocalClientInit = true;
}

void UTATDisguiseComponent::_LocalClientEndPlay()
{
   check(_IsLocalClient());
   _RemoveAllLocalClientEffects();
}

void UTATDisguiseComponent::_LocalClientTickComponent(float deltaTime)
{
   check(_IsLocalClient());

   // We rely on the disguise tool component being available to configure VFX.
   // Rather than running _LocalClientInit on BeginPlay, attempt to run it on the first tick.
   // If it's not available yet (eg. because the player is still spawning), keep trying until it becomes available.
   //
   // Note that a disguise component won't have tick enabled unless the disguise is active, so in practice this should run exactly once,
   // on the first tick after disguise was enabled.
   if (!_hasRunLocalClientInit)
   {
      _LocalClientInit();
   }
   if (!_hasRunLocalClientInit)
   {
      return;
   }

   if (!_currentDisguiseState.IsDisguiseActive)
   {
      // Make sure there's no active VFX
      _RemoveAllLocalClientEffects();
      return;
   }

   auto spawnVFX = [this](ACharacter* character, UNiagaraSystem* vfxAsset) -> UNiagaraComponent*
   {
      if (IsValid(character) && IsValid(vfxAsset))
      {
         FFXSystemSpawnParameters params{};
         params.WorldContextObject = this;
         params.SystemTemplate = vfxAsset;
         params.AttachToComponent = character->GetCapsuleComponent();
         params.LocationType = EAttachLocation::SnapToTarget;
         params.bAutoDestroy = false;
         params.bAutoActivate = true;
         params.PoolingMethod = EPSCPoolMethod::ManualRelease;
         params.bIsPlayerEffect = true;
         return UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(params);
      }
      return nullptr;
   };
   
   auto destroyVFX = [this](TObjectPtr<UNiagaraComponent>& component)
   {
      if (IsValid(component))
      {
         component->ReleaseToPool();
      }
      component = nullptr;
   };
   
   auto characterIsConscious = [](ACharacter* character) -> bool
   {
      if (!IsValid(character))
      {
         return false;
      }
      if (AOSECharacterBase* oseChar = Cast<AOSECharacterBase>(character))
      {
         return !oseChar->IsUnconscious();
      }
      return true;
   };
   
   auto getCharacterViewDirection = [](ACharacter* character) -> FVector
   {
      if (!IsValid(character))
      {
         return FVector::ForwardVector;
      }
      FVector loc;
      FRotator rot;
      if (!UOSEAbilityFunctionLibrary::OffsetCameraAimToAvatarAim(character, FGameplayAbilityTargetingLocationInfo(), loc, rot))
      {
         // fallback
         character->GetActorEyesViewPoint(loc, rot);
      }
      return rot.Vector();
   };

   ACharacter* ownerCharacter = Cast<ACharacter>(GetOwner());
   UTATDisguiseToolComponent* toolComp = _GetDisguiseToolComponent();
   if (!ensure(ownerCharacter != nullptr) || !ensure(toolComp != nullptr))
   {
      return;
   }

   UNiagaraSystem* effectAsset = toolComp->NearbyCharacterStateRadiusEffect;
   if (!_characterProximityVFXInfos.IsEmpty() && effectAsset != nullptr)
   {
      float searchRadius = 0.0f;

      // proximity emitter
      float vfxProximityRadius = 0.0f;
      // view cone emitter
      float vfxViewConeAngleDeg = 0.0f;
      float vfxViewConeRadius = 0.0f;

      for (const FDisguiseVFXInfo_Character& info : _characterProximityVFXInfos)
      {
         // character search radius
         searchRadius = FMath::Max(searchRadius, info.Radius);

         if (info.ViewConeAngleDeg)
         {
            // view cone emitter params
            vfxViewConeAngleDeg = FMath::Max(vfxViewConeAngleDeg, *info.ViewConeAngleDeg);
            vfxViewConeRadius = FMath::Max(vfxViewConeRadius, info.Radius);
         }
         else
         {
            // proximity emitter params
            vfxProximityRadius = FMath::Max(vfxProximityRadius, info.Radius);
         }
      }

      if (searchRadius > 0)
      {
         auto shouldApplyRadiusEffectToCharacter = [this](ACharacter* character)
         {
            // apply the effect if any character type matches
            for (const FDisguiseVFXInfo_Character& info : _characterProximityVFXInfos)
            {
               if (_ShouldHandleNearbyCharacter(character, info.CharacterType))
               {
                  return true;
               }
            }
            return false;
         };
         
         // We apply VFX to characters that are within double the range of the gameplay effect's radius (or the radius + 1000, whichever is smaller)
         searchRadius = FMath::Min(searchRadius * 2.0f, searchRadius + 1000.0f);

         // Spawn VFX on any character within double the radius that would actually damage the disguise
         _UpdateNearbyCharacters(_localPlayerNearbyCharacters, searchRadius);

         for (ACharacter* nearbyCharacter : _localPlayerNearbyCharacters)
         {
#if TAT_ENABLE_DEV_TOOLS && UE_ENABLE_DEBUG_DRAWING
            if (_devToolState.DebugDraw)
            {
               // small offset so you can tell the difference between client and server debug drawing
               constexpr float clientDebugDrawOffset = 1.0f;
               constexpr int32 resolution = 24;
               static const FColor color = FColor::Magenta;

               for (const FDisguiseVFXInfo_Character& info : _characterProximityVFXInfos)
               {
                  if (info.ViewConeAngleDeg)
                  {
                     const float angleRad = FMath::DegreesToRadians(*info.ViewConeAngleDeg * 0.5f);
                     DrawDebugCone(GetWorld(), nearbyCharacter->GetActorLocation() + FVector(0, 0, clientDebugDrawOffset),
                        getCharacterViewDirection(nearbyCharacter), info.Radius + clientDebugDrawOffset, angleRad, angleRad, 
                        resolution, color);
                  }
                  else
                  {
                     const FVector yAxis{ 0, 1, 0 };
                     const FVector zAxis{ 1, 0, 0 };
                     constexpr bool drawAxis = true;
                     DrawDebugCircle(GetWorld(), nearbyCharacter->GetActorLocation() + FVector(0, 0, clientDebugDrawOffset),
                        info.Radius + clientDebugDrawOffset, resolution, color, false, -1, 0, 0,
                        yAxis, zAxis, drawAxis);
                  }
               }
            }
#endif // TAT_ENABLE_DEV_TOOLS && UE_ENABLE_DEBUG_DRAWING

            if (!shouldApplyRadiusEffectToCharacter(nearbyCharacter))
            {
               continue;
            }

            FTATDisguiseLocalClientActiveEffectContainer& clientEffects = _nearbyCharacterEffects.FindOrAdd(nearbyCharacter);

            // Effect already active?
            if (IsValid(clientEffects.RadiusEffectParticles) && clientEffects.RadiusEffectParticles->IsActive())
            {
               continue;
            }

            // Clear out existing inactive effect
            if (clientEffects.RadiusEffectParticles != nullptr)
            {
               destroyVFX(clientEffects.RadiusEffectParticles);
            }

            // Spawn new VFX
            clientEffects.RadiusEffectParticles = spawnVFX(nearbyCharacter, effectAsset);
            if (clientEffects.RadiusEffectParticles != nullptr)
            {
               // set up proximity emitter
               static const FName proximityEmitterName = FName(TEXT("ProximityEmitter"));
               static const FName proximityRadiusParam = FName(TEXT("EffectRadius"));

               const bool proximityEnabled = vfxProximityRadius > 0;
               clientEffects.RadiusEffectParticles->SetEmitterEnable(proximityEmitterName, proximityEnabled);
               if (proximityEnabled)
               {
                  clientEffects.RadiusEffectParticles->SetFloatParameter(proximityRadiusParam, vfxProximityRadius);
               }

               // set up view cone emitter
               static const FName viewConeEmitterName = FName(TEXT("ViewConeEmitter"));
               static const FName viewConeRadiusParam = FName(TEXT("FrontAngleRadius"));
               static const FName viewConeAngleDegParam = FName(TEXT("FrontAngleDeg"));

               const bool viewConeEnabled = vfxViewConeRadius > 0;
               clientEffects.RadiusEffectParticles->SetEmitterEnable(viewConeEmitterName, viewConeEnabled);
               if (viewConeEnabled)
               {
                  clientEffects.RadiusEffectParticles->SetFloatParameter(viewConeRadiusParam, vfxViewConeRadius);
                  clientEffects.RadiusEffectParticles->SetFloatParameter(viewConeAngleDegParam, vfxViewConeAngleDeg);
               }
            }
         }

         // Remove any effects we applied in the past but are now too far away (or no longer conscious)
         for (auto& pair : _nearbyCharacterEffects)
         {
            if (!IsValid(pair.Key)
               || !characterIsConscious(pair.Key)
               || FVector::DistSquared(pair.Key->GetActorLocation(), ownerCharacter->GetActorLocation()) > FMath::Square(searchRadius))
            {
               destroyVFX(pair.Value.RadiusEffectParticles);
            }
         }
      }
   }
}

void UTATDisguiseComponent::_RemoveAllLocalClientEffects()
{
   for (auto& pair : _nearbyCharacterEffects)
   {
      pair.Value.DestroyAllEffects();
   }
   _nearbyCharacterEffects.Reset();
}

bool UTATDisguiseComponent::_ShouldHandleNearbyCharacter(ACharacter* nearbyCharacter, ETATTeamCharacterType matchCharacterType) const
{
   ACharacter* ownerCharacter = Cast<ACharacter>(GetOwner());
   check(IsValid(ownerCharacter));
   if (!IsValid(nearbyCharacter) || nearbyCharacter == ownerCharacter)
   {
      return false;
   }
   ATATCharacterAIBase* npcChar = Cast<ATATCharacterAIBase>(nearbyCharacter);
   if (npcChar != nullptr && npcChar->TeamCharacter != matchCharacterType)
   {
      return false;
   }
   ATATCharacter* playerChar = Cast<ATATCharacter>(nearbyCharacter);
   if (playerChar != nullptr && matchCharacterType != ETATTeamCharacterType::Player)
   {
      return false;
   }
   return true;
}

UTATDisguiseToolComponent* UTATDisguiseComponent::_GetDisguiseToolComponent() const
{
   if (const UToolSetComponent* toolSetComponent = UTATToolFunctionLibrary::GetToolSetComponentFromActor(GetOwner()))
   {
      constexpr bool matchesExact = false;
      return Cast<UTATDisguiseToolComponent>(toolSetComponent->GetToolByClass(UTATDisguiseToolComponent::StaticClass(), matchesExact));
   }
   return nullptr;
}

void UTATDisguiseComponent::_OnDamageChanged(const FOnAttributeChangeData& onAttributeChangeData)
{
   const FGameplayEffectModCallbackData* modData = onAttributeChangeData.GEModData;
   if(modData == nullptr)
      return;

   const FGameplayTagContainer& specTags = modData->EffectSpec.CapturedSourceTags.GetSpecTags();
   
   const ATATCharacter* instigatorCharacter = Cast<ATATCharacter>(modData->EffectSpec.GetEffectContext().GetInstigator());
   const ATATPlayerState* instigatorPlayerState = instigatorCharacter ? instigatorCharacter->GetPlayerState<ATATPlayerState>() : nullptr;
   const ATATCharacter* ownerCharacter = Cast<ATATCharacter>(GetOwner());
   const ATATPlayerState* ownerPlayerState = ownerCharacter ? ownerCharacter->GetPlayerState<ATATPlayerState>() : nullptr;
   
   if(ownerPlayerState != nullptr && instigatorPlayerState != nullptr)
   {
      if(ownerPlayerState->GetTeam() == instigatorPlayerState->GetTeam())
      {
         return;
      }
   }
   if(_damageQueryToBreakDisguise.Matches(specTags))
   {
      AuthorityEndDisguise();
   }
}

void UTATDisguiseComponent::_OnDisguiseBegin()
{
   if (_IsLocalClient() || _IsCoopPartner())
   {
      SetComponentTickEnabled(true);
   }

   _RecomputeDisguiseVisibility();

   OnDisguiseBegin.Broadcast();
   OnDisguiseNativeBegin.Broadcast();
}

void UTATDisguiseComponent::_OnDisguiseBroken()
{
   _RecomputeDisguiseVisibility();

   OnDisguiseEnd.Broadcast();
   OnDisguiseNativeEnd.Broadcast();

   bool shouldDisableTick = false;

   // Destroy any VFX spawned while disguise was active
   if (_IsLocalClient())
   {
      shouldDisableTick = true;
      _RemoveAllLocalClientEffects();
   }
   else if (_IsCoopPartner())
   {
      shouldDisableTick = true;
      if (_disguisedPlayerCoopIndicatorComponent != nullptr && _disguisedPlayerCoopIndicatorComponent->IsActive())
      {
         _disguisedPlayerCoopIndicatorComponent->SetActive(false);
      }
   }

   if (shouldDisableTick)
   {
      SetComponentTickEnabled(false);
   }
}

void UTATDisguiseComponent::_OnReductionGameplayTagNewOrRemoved(FGameplayTag tag, int32 newCount)
{
   if (!_currentDisguiseState.IsDisguiseActive)
   {
      return;
   }

   const FDisguiseGameplayTagChangeInfo* info = _gameplayTagChangeEventInfos.Find(tag);
   if (info == nullptr)
   {
      UE_LOG(LogTATDisguiseComponent, Warning, TEXT("UTATDisguiseComponent attached to '%s' received unexpected gameplay tag change event for '%s'"),
         *GetOwner()->GetName(), *tag.ToString());
      return;
   }

   const bool tagAdded = newCount > 0;

   if (info->TriggerOnTagAdded == tagAdded)
   {
      _OnReductionEventOccur(
         TAT_DISGUISE_EVENT_DEBUG_INFO(tagAdded ? TEXT("GameplayTagAdded") : TEXT("GameplayTagRemoved"), tag.ToString(), info->EventInfo.Result.ToDebugValue(), false),
         info->EventInfo.Result, info->EventInfo);
   }
}

void UTATDisguiseComponent::_OnReductionGameplayEvent(const FGameplayEventData* payload)
{
   if (!_currentDisguiseState.IsDisguiseActive)
   {
      return;
   }

   if (const FDisguiseEventInfo_DelegateHandle* bindInfo = _gameplayEventInfos.Find(payload->EventTag))
   {
      TArray<const AActor*, TInlineAllocator<2>> relatedActors;
      if (payload->Instigator)
      {
         relatedActors.Add(payload->Instigator);
      }

      if (payload->Target)
      {
         relatedActors.Add(payload->Target);
      }

      _OnReductionEventOccur(
         TAT_DISGUISE_EVENT_DEBUG_INFO(TEXT("GameplayEvent"), payload->EventTag.ToString(), bindInfo->EventInfo.Result.ToDebugValue(), false),
         bindInfo->EventInfo.Result, bindInfo->EventInfo, relatedActors);
   }
   else
   {
      UE_LOG(LogTATDisguiseComponent, Warning, TEXT("UTATDisguiseComponent attached to '%s' received unexpected gameplay event '%s'"),
         *GetOwner()->GetName(), *payload->EventTag.ToString());
   }
}

void UTATDisguiseComponent::_OnReductionAttributeChange(const FOnAttributeChangeData& data)
{
   if (!_currentDisguiseState.IsDisguiseActive)
   {
      return;
   }

   const float attributeDelta = data.OldValue - data.NewValue;
   if (attributeDelta <= 0)
   {
      return;
   }

   if (const FDisguiseAttributeChangeInfo* bindInfo = _gameplayAttributeDecreaseEventInfos.Find(data.Attribute))
   {
      FTATDisguiseIntegrityResult result = bindInfo->EventInfo.Result;

      // Factor in the change in attribute value if needed
      if (!result.InstantCancel && bindInfo->AttributeDeltaMultiplier != 0)
      {
         if (bindInfo->MaxExtraReductionFromAttributeDelta > 0)
         {
            result += FTATDisguiseIntegrityResult(FMath::Clamp(attributeDelta * bindInfo->AttributeDeltaMultiplier, 0.0f, bindInfo->MaxExtraReductionFromAttributeDelta));
         }
         else
         {
            result += FTATDisguiseIntegrityResult(FMath::Max(attributeDelta * bindInfo->AttributeDeltaMultiplier, 0.0f));
         }
      }

      _OnReductionEventOccur(
         TAT_DISGUISE_EVENT_DEBUG_INFO(TEXT("AttributeChange"), data.Attribute.GetName(), result.ToDebugValue(), false),
         result, bindInfo->EventInfo);
   }
   else
   {
      UE_LOG(LogTATDisguiseComponent, Warning, TEXT("UTATDisguiseComponent attached to '%s' received unexpected attribute changed event '%s'"),
         *GetOwner()->GetName(), *data.Attribute.GetName());
   }
}

void UTATDisguiseComponent::_HandleIntegrityReductionEventInternal(
   FTATDisguiseReductionEventDebugInfo&& debugInfo,
   float& inOutIntegrityValue,
   const FTATDisguiseIntegrityResult& integrityResult,
   const FDisguiseEventInfo& eventInfo,
   TArrayView<const AActor*> relatedActors) const
{
   if (!_currentDisguiseState.IsDisguiseActive
      || !_AuthorityMeetsVisibilityRequirements(eventInfo.VisibilityRequirement, relatedActors)
      || !_AuthorityMeetsStealthScoreRequirements(eventInfo.StealthScoreRange))
   {
      return;
   }

   if (integrityResult.InstantCancel)
   {
      inOutIntegrityValue = 0.0f;
   }
   else if (integrityResult.IntegrityReduction)
   {
      inOutIntegrityValue = FMath::Max(0.0f, inOutIntegrityValue - *integrityResult.IntegrityReduction);
   }
   
#if TAT_ENABLE_DEV_TOOLS
   // Compute a reasonable value for RemoveAtWorldTime if one was not passed in
   if (debugInfo.RemoveAtWorldTime == 0)
   {
      // Keep the message around if it's not a per-frame event or the disguise would immediately cancel
      debugInfo.RemoveAtWorldTime = (!debugInfo.IsPerFrameEvent || integrityResult.InstantCancel || inOutIntegrityValue <= 0)
         ? (GetWorld()->GetTimeSeconds() + kTATDisguiseMessageDurationSec)
         : GetWorld()->GetTimeSeconds();
   }
   const_cast<UTATDisguiseComponent*>(this)->_devToolState.IntegrityReductionEvents.Add(MoveTemp(debugInfo));
#endif
}

void UTATDisguiseComponent::_OnReductionEventOccur(
   FTATDisguiseReductionEventDebugInfo&& debugInfo,
   const FTATDisguiseIntegrityResult& integrityResult,
   const FDisguiseEventInfo& eventInfo,
   TArrayView<const AActor*> relatedActors)
{
   float newIntegrityValue = _currentDisguiseIntegrity;

   _HandleIntegrityReductionEventInternal(MoveTemp(debugInfo), newIntegrityValue, integrityResult, eventInfo, relatedActors);

   if (newIntegrityValue <= 0.0f)
   {
      AuthorityEndDisguise();
   }
   else if (!FMath::IsNearlyEqual(newIntegrityValue, _currentDisguiseIntegrity))
   {
      AuthoritySetDisguiseIntegrity(newIntegrityValue);
   }
}

bool UTATDisguiseComponent::_AuthorityMeetsStealthScoreRequirements(const FFloatRange& stealthScoreRange) const
{
   check(GetOwner()->HasAuthority());
   if (stealthScoreRange == FFloatRange(FFloatRangeBound::Open(), FFloatRangeBound::Open()))
   {
      return true;
   }
   if (ATATCharacter* character = Cast<ATATCharacter>(GetOwner()))
   {
      return !stealthScoreRange.Contains(character->GetStealthScore());
   }
   return false;
}

bool UTATDisguiseComponent::_AuthorityMeetsVisibilityRequirements(ETATDisguiseVisibilityRequirement visRequirement, TArrayView<const AActor*> relatedActors) const
{
   check(GetOwner()->HasAuthority());
   
   if (visRequirement == ETATDisguiseVisibilityRequirement::Always)
   {
      return true;
   }

   ATATCharacter* character = Cast<ATATCharacter>(GetOwner());
   if (character == nullptr)
   {
      UE_LOG(LogTATDisguiseComponent, Warning,
         TEXT("UTATDisguiseComponent attached to '%s' which is not a TATCharacter. ")
         TEXT("Trying to use visibility requirement that's not Always, which currently is not supported"),
         *GetOwner()->GetName());
      return false;
   }

   if (visRequirement == ETATDisguiseVisibilityRequirement::Always)
   {
      return true;
   }

   if (visRequirement == ETATDisguiseVisibilityRequirement::WhenSeenByAnyNPC)
   {
      return character->AuthorityIsBeingViewed();
   }

   if (visRequirement == ETATDisguiseVisibilityRequirement::WhenSeenByUnrelatedNPC)
   {
      const bool isSeenByUnrelatedNPC = character->AuthorityGetViewingActors().ContainsByPredicate([&](const AActor* viewingActor)
      {
         if (IsValid(viewingActor))
         {
            // See if any of the actors observing us are not in the relatedActors list
            return !relatedActors.Contains(viewingActor);
         }
         return false;
      });
      return isSeenByUnrelatedNPC;
   }

   checkNoEntry();
   return false;
}

void UTATDisguiseComponent::_AuthorityApplyDisguiseEffects(AActor* caster)
{
   check(GetOwner()->HasAuthority());

   // Add the "is disguised" effect to ourselves
   if (IsDisguisedEffect != nullptr)
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         const UGameplayEffect* disguiseEffectCDO = IsDisguisedEffect.GetDefaultObject();
         check(disguiseEffectCDO != nullptr);

         FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
         _disguiseEffectHandle = asc->ApplyGameplayEffectToSelf(disguiseEffectCDO, 0.0f, effectContext, asc->GetPredictionKeyForNewAction());
         _appliedDisguiseEffects.Add(GetOwner(), _disguiseEffectHandle);
      }
   }

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      asc->GetGameplayAttributeValueChangeDelegate(UAttributeBaseSet::GetHealthDamageAttribute()).AddUObject(this, &ThisClass::_OnDamageChanged);
   }

   // If we're disguising ourselves, add the "self disguise" effect to ourself as well
   if (caster == GetOwner())
   {
      if (IsSelfDisguisedEffect != nullptr)
      {
         if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
         {
            FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
            FActiveGameplayEffectHandle effectHandle = asc->ApplyGameplayEffectToSelf(IsSelfDisguisedEffect.GetDefaultObject(), 0.0f, effectContext, asc->GetPredictionKeyForNewAction());
            _appliedDisguiseEffects.Add(GetOwner(), effectHandle);
         }
      }
   }

   _authorityDisguiseCaster = caster;

   // Add the "granting disguise" effect to the caster
   if (caster != nullptr)
   {
      if (IsGrantingDisguiseEffect)
      {
         if (UAbilitySystemComponent* casterAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(caster))
         {
            FGameplayEffectContextHandle effectContext = casterAsc->MakeEffectContext();
            effectContext.AddInstigator(GetOwner(), GetOwner());
            FActiveGameplayEffectHandle effectHandle = casterAsc->ApplyGameplayEffectToSelf(IsGrantingDisguiseEffect.GetDefaultObject(), 0.0f, effectContext, casterAsc->GetPredictionKeyForNewAction());
            _appliedDisguiseEffects.Add(caster, effectHandle);
         }
      }
   }
   else
   {
      UE_LOG(LogTATDisguiseComponent, Warning, TEXT("Applying a disguise to '%s' without specifying who the caster is"), *GetOwner()->GetName());
   }
}

void UTATDisguiseComponent::_AuthorityRemoveDisguiseEffects()
{
   check(GetOwner()->HasAuthority());

   // Remove our disguised effect, and the caster's casting effect
   _appliedDisguiseEffects.CancelAll();
   _disguiseEffectHandle.Invalidate();

   // Add the cooldown effect to our caster
   if (AActor* caster = _authorityDisguiseCaster.Get())
   {
      if (UAbilitySystemComponent* casterAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(caster))
      {
         if (EffectAfterDisguiseOver != nullptr)
         {
            FGameplayEffectContextHandle effectContext = casterAsc->MakeEffectContext();
            casterAsc->ApplyGameplayEffectToSelf(EffectAfterDisguiseOver.GetDefaultObject(), 0.0f, effectContext, casterAsc->GetPredictionKeyForNewAction());
         }
      }
   }

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
   {
      asc->GetGameplayAttributeValueChangeDelegate(UAttributeBaseSet::GetHealthDamageAttribute()).RemoveAll(this);
   }
   
   if (EffectOnObserversAfterDisguiseOver != nullptr)
   {
      if (auto* character = Cast<ATATCharacter>(GetOwner()))
      {
         for (AActor* viewingActor : character->AuthorityGetViewingActors())
         {
            if (IsValid(viewingActor))
            {
               if (UAbilitySystemComponent* viewerAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(viewingActor))
               {
                  FGameplayEffectContextHandle effectContext = viewerAsc->MakeEffectContext();
                  viewerAsc->ApplyGameplayEffectToSelf(EffectOnObserversAfterDisguiseOver.GetDefaultObject(), 0.0f, effectContext, viewerAsc->GetPredictionKeyForNewAction());
               }
            }
         }
      }
   }

   _authorityDisguiseCaster = nullptr;
}

void UTATDisguiseComponent::_OnAbilityActivated(UGameplayAbility* gameplayAbility)
{
   if (!GetOwner()->HasAuthority() || !_currentDisguiseState.IsDisguiseActive || gameplayAbility == nullptr)
   {
      return;
   }

   UTATDisguiseToolComponent* disguiseTool = _GetDisguiseToolComponent();
   if (!ensure(disguiseTool != nullptr))
   {
      return;
   }

   if (disguiseTool->AbilityQueryToDisableDisguise.Matches(gameplayAbility->GetAssetTags()))
   {
      AuthoritySetDisguiseIntegrity(0.f);
   }

   if (!ensureMsgf(_disableAbilitiesTag.IsValid(), TEXT("Failed to get a valid DisableAbilitiesTag. See [TAT] Project Settings -> DisableAbilitiesTag")))
   {
      return;
   }

   auto isAbilityInList = [](TSubclassOf<UGameplayAbility> abilityClass, const TArray<TSoftClassPtr<UGameplayAbility>>& abilityList) -> bool
   {
      check(abilityClass != nullptr);
      for (const TSoftClassPtr<UGameplayAbility>& softClassPtr : abilityList)
      {
         TSubclassOf<UGameplayAbility> cls = softClassPtr.Get();
         // No need to sync/async load the soft class pointer - in case of a match, we know its already loaded.
         if (cls && abilityClass->IsChildOf(cls))
         {
            return true;
         }
      }
      return false;
   };

   TOptional<bool> _isActivatedAbilityCached;
   auto isActivatedAbility = [this, &_isActivatedAbilityCached](UGameplayAbility* ability) -> bool
   {
      // Perform lazy evaluation of the ActivationBlockedTags check, because for a given ability we might check it more than once, but we might never check it.
      if (!_isActivatedAbilityCached)
      {
         UOSEGameplayAbility* oseAbility = Cast<UOSEGameplayAbility>(ability);
         _isActivatedAbilityCached = (oseAbility != nullptr)
            ? oseAbility->GetActivationBlockedTags().HasTagExact(_disableAbilitiesTag)
            : false;
      }
      return *_isActivatedAbilityCached;
   };
   
   for (const FDisguiseAbilityInfo& info : _abilityEventInfos)
   {
      // If we only care about activated abilities (which we define as an ability with a certain tag in "ActivationBlockedTags"), make sure this ability is one.
      if (info.Config.OnlyActivatedAbilities && !isActivatedAbility(gameplayAbility))
      {
         continue;
      }

      // Check against the ability allow list (if we have one)
      if (!info.Config.AbilityAllowList.IsEmpty() && !isAbilityInList(gameplayAbility->GetClass(), info.Config.AbilityAllowList))
      {
         continue;
      }

      // Check against the ability deny list (if we have one)
      if (!info.Config.AbilityDenyList.IsEmpty() && isAbilityInList(gameplayAbility->GetClass(), info.Config.AbilityDenyList))
      {
         continue;
      }

      // Check against the ability tag query (if we have one)
      if (!info.Config.AbilityTagQuery.IsEmpty() && !info.Config.AbilityTagQuery.Matches(gameplayAbility->GetAssetTags()))
      {
         continue;
      }

      _OnReductionEventOccur(
         TAT_DISGUISE_EVENT_DEBUG_INFO(TEXT("AbilityActivated"), gameplayAbility->GetName(), info.EventInfo.Result.ToDebugValue(), false),
         info.EventInfo.Result, info.EventInfo);
   }
}

namespace DisguiseHelpers
{
   UTATCharacterAnimationMappingAsset* GetAnimMapping(const FDisguiseSnapshot& snapshot)
   {
      if (snapshot.ActorClass)
      {
         if (ITATDisguiseTargetInterface* disguiseTargetInterfaceCDO = Cast<ITATDisguiseTargetInterface>(snapshot.ActorClass.GetDefaultObject()))
         {
            return disguiseTargetInterfaceCDO->GetDisguiseTargetCharacterAnimationMapping();
         }
      }
      return nullptr;
   }

   FTATDisguiseMovementParams GetMovementParams(const FDisguiseSnapshot& snapshot)
   {
      if (snapshot.ActorClass)
      {
         if (ITATDisguiseTargetInterface* disguiseTargetInterfaceCDO = Cast<ITATDisguiseTargetInterface>(snapshot.ActorClass.GetDefaultObject()))
         {
            return disguiseTargetInterfaceCDO->GetDisguiseTargetMovementParams();
         }
      }
      return FTATDisguiseMovementParams{};
   }

   ETATDisguiseTargetType GetDisguiseTargetType(const FDisguiseSnapshot& snapshot)
   {
      if(snapshot.ActorClass)
      {
         if (ITATDisguiseTargetInterface* disguiseTargetInterfaceCDO = Cast<ITATDisguiseTargetInterface>(snapshot.ActorClass.GetDefaultObject()))
         {
            return disguiseTargetInterfaceCDO->GetDisguiseTargetType();
         }
      }
      return ETATDisguiseTargetType::None;
   }
}

ETATDisguiseTargetType UTATDisguiseComponent::GetDisguiseTargetType() const
{
   if(IsDisguiseActive())
   {
      return DisguiseHelpers::GetDisguiseTargetType(_currentDisguiseState.CurrentDisguiseData);
   }
   return ETATDisguiseTargetType::None;
}

void UTATDisguiseComponent::_SetDisguise(bool isDisguiseVisible)
{
   ACharacter* ownerChar = Cast<ACharacter>(GetOwner());
   if (ownerChar == nullptr)
   {
      return;
   }

   // Apply movement speed constraints
   if (UTATCharacterMovement* cmc = ownerChar->GetCharacterMovement<UTATCharacterMovement>())
   {
      if (isDisguiseVisible)
      {
         const FTATDisguiseMovementParams movementParams = DisguiseHelpers::GetMovementParams(_currentDisguiseState.CurrentDisguiseData);
         if (movementParams.IsValid())
         {
            cmc->AddMovementModifier(UTATDisguiseComponent::StaticClass()->GetFName(),
               ETATCharacterMovementModifierStat::WalkSpeed | ETATCharacterMovementModifierStat::CrouchSpeed,
               ETATCharacterMovementModifierOp::ClampMax,
               movementParams.MaxWalkSpeed);

            cmc->AddMovementModifier(UTATDisguiseComponent::StaticClass()->GetFName(),
               ETATCharacterMovementModifierStat::Acceleration,
               ETATCharacterMovementModifierOp::ClampMax,
               movementParams.MaxAcceleration);
         }
      }
      else
      {
         cmc->RemoveMovementModifier(UTATDisguiseComponent::StaticClass()->GetFName());
      }
   }

   // Notify the disguise tool
   if (UTATDisguiseToolComponent* disguiseTool = _GetDisguiseToolComponent())
   {
      disguiseTool->OnDisguiseStateChanged(isDisguiseVisible, _currentDisguiseState.CurrentDisguiseData);
   }

   // Need to assign this before the early out because the server needs the animation mapping in order to properly replicate animations to clients.
   if (isDisguiseVisible)
   {
      _disguisedCharacterAnimationMapping = DisguiseHelpers::GetAnimMapping(_currentDisguiseState.CurrentDisguiseData);
   }

   // The rest of this function only handles cosmetic mesh changes
   if (IsNetMode(NM_DedicatedServer))
   {
      return;
   }

   if (isDisguiseVisible)
   {
      if (!_origMeshParams.IsValid())
      {
         constexpr bool isDisguise = false;
         _origMeshParams.CopyFrom(ownerChar, nullptr, isDisguise);
      }

      if (ACharacter* targetCharCDO = _currentDisguiseState.CurrentDisguiseData.ActorClass.GetDefaultObject())
      {
         _origMeshParams.Unapply(ownerChar);

         constexpr bool isDisguise = true;
         _disguisedMeshParams.CopyFrom(targetCharCDO, ownerChar, isDisguise);
         _disguisedMeshParams.Apply(ownerChar);
      }

      ITATDisguisableCharacterInterface::Execute_ApplyDisguiseVisuals(GetOwner(), _currentDisguiseState.CurrentDisguiseData);
   }
   else
   {
      _disguisedMeshParams.Unapply(ownerChar);

      _origMeshParams.Apply(ownerChar);

      // If we managed the anim class layers for the currently equipped tool, unlink them now, then restore the correct anim layer for that tool
      if (UTATToolComponent* lastLinkedToolComponent = _lastAnimLinkedToolComponent.Get())
      {
         check(_lastAnimLinkedToolComponentAnimClassLayer != nullptr);
         HandleToolUnlinkAnimClassLayers(lastLinkedToolComponent, _lastAnimLinkedToolComponentAnimClassLayer);

         // Link the tool's original anim class layer
         USkeletalMeshComponent* meshComponent = ownerChar->GetMesh();
         if (meshComponent != nullptr && lastLinkedToolComponent->GetToolAnimations().AnimClassLayer)
         {
            meshComponent->LinkAnimClassLayers(lastLinkedToolComponent->GetToolAnimations().AnimClassLayer);
         }
      }

      ITATDisguisableCharacterInterface::Execute_RevertDisguiseVisuals(GetOwner());

      _disguisedCharacterAnimationMapping = nullptr;
   }
}


bool UTATDisguiseComponent::_ShouldDisguiseBeVisible() const
{
   return _currentDisguiseState.IsDisguiseActive;
}

void UTATDisguiseComponent::_RecomputeDisguiseVisibility()
{
   bool shouldDisguiseBeVisible = _ShouldDisguiseBeVisible();
   if (shouldDisguiseBeVisible != _isDisguiseCurrentlyVisible)
   {
      _SetDisguise(shouldDisguiseBeVisible);

      _isDisguiseCurrentlyVisible = shouldDisguiseBeVisible;
   }
}

void UTATDisguiseComponent::_OnRep_CurrentDisguiseState(const FTATDisguiseState& oldState)
{
   if (oldState.IsDisguiseActive != _currentDisguiseState.IsDisguiseActive)
   {
      if (_currentDisguiseState.IsDisguiseActive)
      {
         _OnDisguiseBegin();
      }
      else
      {
         _OnDisguiseBroken();
      }
   }
}

void UTATDisguiseComponent::_OnRep_CurrentDisguiseIntegrity()
{
   OnDisguiseIntegrityChange.Broadcast(_currentDisguiseIntegrity, GetNormalizedRemainingDisguiseIntegrity());
}

void UTATDisguiseComponent::_UpdateNearbyCharacters(TArray<TObjectPtr<ACharacter>>& outCharacters, float searchRadius, bool requireConscious) const
{
   outCharacters.Reset();

   if (searchRadius <= 0)
   {
      return;
   }

   AActor* owner = GetOwner();
   check(IsValid(owner));
   const FVector ownerLocation = owner->GetActorLocation();

   const float radiusSquared = FMath::Square(searchRadius);

   for (TActorIterator<ACharacter> it(GetWorld()); it; ++it)
   {
      if (ACharacter* character = (*it))
      {
         if (character == owner)
         {
            continue;
         }

         if (requireConscious)
         {
            AOSECharacterBase* oseChar = Cast<AOSECharacterBase>(character);
            if (oseChar != nullptr && oseChar->IsUnconscious())
            {
               continue;
            }
         }
         
         const FVector charLocation = character->GetActorLocation();
         if (FVector::DistSquared(ownerLocation, charLocation) <= radiusSquared)
         {
            outCharacters.Add(character);
         }
      }
   }
}

UTATDisguiseComponent::FDisguiseEventInfo UTATDisguiseComponent::_MakeDisguiseEventInfo(const FTATDisguiseIntegrityReductionDataRow& dataRow) const
{
   FTATDisguiseIntegrityResult integrityResult;
   integrityResult.InstantCancel = dataRow.IsInstantCancelEvent();
   if (!integrityResult.InstantCancel)
   {
      // Eval the upgrade value at init time to avoid evaluating it for every application
      const float integrityReduction = _EvalUpgradeCurve(dataRow.IntegrityReduction, dataRow.IntegrityReductionUpgradeTag);
      if (integrityReduction != 0)
      {
         integrityResult.IntegrityReduction = integrityReduction;
      }
   }
   return FDisguiseEventInfo{ integrityResult, dataRow.StealthScoreRange, dataRow.VisibilityRequirement };
}

float UTATDisguiseComponent::_EvalUpgradeCurve(const FScalableFloat& upgradeCurve, FGameplayTag upgradeTag) const
{
   static const FString contextString = TEXT("UTATDisguiseComponent::_EvalUpgradeCurve");
   if (upgradeTag.IsValid())
   {
      UOSEAbilitySystemComponent* asc = Cast<UOSEAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
      if (ensure(asc != nullptr))
      {
         constexpr int32 fallbackLevel = 0;
         return upgradeCurve.GetValueAtLevel(static_cast<float>(asc->GetUpgradeValue(upgradeTag, fallbackLevel)), &contextString);
      }
   }
   return upgradeCurve.GetValue(&contextString);
}
