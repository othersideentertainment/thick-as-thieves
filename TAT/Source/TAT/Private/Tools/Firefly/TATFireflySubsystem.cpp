// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/Firefly/TATFireflySubsystem.h"

// tat
#include "Player/TATCharacter.h"
#include "Developer/TATProjectSettings.h"
#include "Developer/TATDevToolSubsystem.h"
#include "Character/TATTeams.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Tools/Firefly/TATFireflyBeaconComponent.h"

// ose
#include "OSECommon.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATFireflySubsystem)

bool UTATFireflySubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   if (const UWorld* world = outer->GetWorld())
   {
      // Game world only
      if (world->IsGameWorld())
      {
         return true;
      }
   }

   return false;
}

void UTATFireflySubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
}

void UTATFireflySubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);

   if (UTATThiefVisionSubsystem* thiefVisionSubsystem = inWorld.GetSubsystem<UTATThiefVisionSubsystem>())
   {
      _thiefVisionEnabled = thiefVisionSubsystem->IsThiefVisionEnabledForLocalPlayer();
      thiefVisionSubsystem->OnLocalPlayerThiefVisionStatusChanged.AddDynamic(this, &UTATFireflySubsystem::_OnThiefVisionStatusChanged);

      if (APlayerController* pc = UOSECommon::GetLocalPlayerController<APlayerController>(this))
      {
         _UpdateFireflyBeaconVisibility(Cast<ATATCharacter>(pc->GetPawn()), _thiefVisionEnabled);
      }
   }

#if TAT_ENABLE_DEV_TOOLS
   if (UTATDevToolSubsystem* devToolSubsystem = inWorld.GetSubsystem<UTATDevToolSubsystem>())
   {
      devToolSubsystem->RegisterDevToolFunction(TEXT("Firefly Subsystem"), [this](const FTATDevToolContext& ctx, FTATDevToolState& state)
      {
         _DrawDevTools(ctx.DeltaSeconds, state);
      }, this);
   }
#endif // TAT_ENABLE_DEV_TOOLS
}

ETickableTickType UTATFireflySubsystem::GetTickableTickType() const
{
   // Don't tick on dedicated server
   const UWorld* world = GetWorld();
   if (world != nullptr && world->IsNetMode(NM_DedicatedServer))
   {
      return ETickableTickType::Never;
   }
   return Super::GetTickableTickType();
}

void UTATFireflySubsystem::Tick(float deltaTime)
{
   Super::Tick(deltaTime);
   
   auto getCharacter = [this]() -> ATATCharacter*
   {
      APlayerController* pc = UOSECommon::GetLocalPlayerController<APlayerController>(this);
      return (pc != nullptr) ? Cast<ATATCharacter>(pc->GetPawn()) : nullptr;
   };

   if (_thiefVisionEnabled)
   {
      _UpdateFireflyBeaconVisibility(getCharacter(), _thiefVisionEnabled);
   }
   else if (!_allBeaconsHidden)
   {
      // hide all beacons if needed
      _UpdateFireflyBeaconVisibility(getCharacter(), false);
   }
}

void UTATFireflySubsystem::RegisterBeacon(UTATFireflyBeaconComponent* beacon)
{
   _beacons.Add(beacon);
}

void UTATFireflySubsystem::UnregisterBeacon(UTATFireflyBeaconComponent* beacon)
{
   _beacons.RemoveSingleSwap(beacon);
}

bool UTATFireflySubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

void UTATFireflySubsystem::_OnThiefVisionStatusChanged(APlayerController* controller, bool thiefVisionEnabled)
{
   _thiefVisionEnabled = thiefVisionEnabled;
}

void UTATFireflySubsystem::_UpdateFireflyBeaconVisibility(ATATCharacter* character, bool abilityIsActive)
{
   if (!IsValid(character))
   {
      // unsure of beacon state, so don't assume all are hidden
      _allBeaconsHidden = false;
      return;
   }

   if (!abilityIsActive)
   {
      for (UTATFireflyBeaconComponent* beacon : _beacons)
      {
         if (IsValid(beacon))
         {
            beacon->SetShown(false);
         }
      }
      _allBeaconsHidden = true;
      return;
   }

   const FVector sourceLocation = character->GetActorLocation();

   int32 numBeaconsEnabled = 0;

   for (UTATFireflyBeaconComponent* beacon : _beacons)
   {
      if (!IsValid(beacon))
      {
         continue;
      }

      const AActor* beaconOwner = beacon->GetOwner();
      if (!ensure(IsValid(beaconOwner)))
      {
         continue;
      }

      if (beaconOwner == character)
      {
         continue;
      }

      if (!beacon->IsAllowed())
      {
         beacon->SetShown(false);
         continue;
      }

      const EOSETeamAttitude teamAttitude = UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(
         character,
         beaconOwner,
         ETATTeamDisguiseHandling::UseOriginalTeam
      );

      // always show friendly fireflies
      bool shouldShow = true;

      if (teamAttitude != EOSETeamAttitude::Friendly)
      {
         const float distSquared = FVector::DistSquared(sourceLocation, beaconOwner->GetActorLocation());
         const float boost = beacon->IsShown() ? UTATProjectSettings::Get().ThiefVisionFireflyShownRangeBoost : 0.0f;
         const float rangeSquared = FMath::Square(UTATProjectSettings::Get().ThiefVisionFireflySearchRadius + beacon->GetRadius() + boost);
         shouldShow = distSquared < rangeSquared;
      }

      beacon->SetShown(shouldShow);

      if (shouldShow)
      {
         ++numBeaconsEnabled;
      }
   }

   _allBeaconsHidden = numBeaconsEnabled == 0;
}

#if TAT_ENABLE_DEV_TOOLS
void UTATFireflySubsystem::_DrawDevTools(float deltaSeconds, FTATDevToolState& state)
{
   if (ImGui::Begin(TATImGui::ConvertString(state.Label), &state.IsOpen))
   {
      if (ImGui::BeginTable("##components", 3, ImGuiTableFlags_Resizable))
      {
         ImGui::TableSetupColumn("Owner");
         ImGui::TableSetupColumn("Component");
         ImGui::TableSetupColumn("Visible");
         ImGui::TableHeadersRow();

         for (int32 i = 0; i < _beacons.Num(); i++)
         {
            UTATFireflyBeaconComponent* beacon = _beacons[i];
            if (!IsValid(beacon))
            {
               continue;
            }

            TATImGui::FScopedID _id{ i };
            if (ImGui::TableNextColumn())
            {
               if (AActor* owner = beacon->GetOwner())
               {
                  TATImGui::TextUnformatted(owner->GetName());
               }
            }
            if (ImGui::TableNextColumn())
            {
               TATImGui::TextUnformatted(beacon->GetName());
            }
            if (ImGui::TableNextColumn())
            {
               bool shown = beacon->IsShown();
               ImGui::Checkbox("##shown", &shown);
            }
         }

         ImGui::EndTable();
      }
   }
   ImGui::End();
}
#endif // TAT_ENABLE_DEV_TOOLS
