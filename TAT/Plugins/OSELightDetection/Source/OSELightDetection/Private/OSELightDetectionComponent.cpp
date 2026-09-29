// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


// ose
#include "OSELightDetectionComponent.h"
#include "OSELightDetectionInterface.h"
#include "OSELightDetectionWorldSubsystem.h"

// ue
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSELightDetectionComponent)

namespace LightDetectionCVars
{
   static int32 DebugLightDetection = 0;
   FAutoConsoleVariableRef CVarDebugLightDetection(
      TEXT("tat.AI.DebugLightDetection"),
      DebugLightDetection,
      TEXT("Should we debug the light detection?"),
      ECVF_Default);
}

UOSELightDetectionComponent::UOSELightDetectionComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   bWantsInitializeComponent = true;
}

void UOSELightDetectionComponent::InitializeComponent()
{
   Super::InitializeComponent();
  
   if(const UWorld* world = GetWorld())
   {
      if(UOSELightDetectionWorldSubsystem* lightDetectionWorldSubsystem = world->GetSubsystem<UOSELightDetectionWorldSubsystem>())
      {
         lightDetectionWorldSubsystem->RegisterLightDetectionComponent(this);
      }
   }
   _CharacterOwner = Cast<ACharacter>(GetOwner());
   _LightDetectionInterface = TScriptInterface<IOSELightDetectionInterface>(_CharacterOwner);
}

void UOSELightDetectionComponent::UninitializeComponent()
{
   if(const UWorld* world = GetWorld())
   {
      if(UOSELightDetectionWorldSubsystem* lightDetectionWorldSubsystem = world->GetSubsystem<UOSELightDetectionWorldSubsystem>())
      {
         lightDetectionWorldSubsystem->UnRegisterLightDetectionComponent(this);
      }
   }   
   Super::UninitializeComponent();
}

const UPrimitiveComponent* UOSELightDetectionComponent::GetPrimitiveComponent() const
{
   return _CharacterOwner ? _CharacterOwner->GetCapsuleComponent() : nullptr;
}

IOSELightDetectionInterface* UOSELightDetectionComponent::GetLightDetectionInterface() const
{
   return _LightDetectionInterface.GetInterface();
}

void UOSELightDetectionComponent::SetLightDetectionValues(const float actualLightIntensity,
                                                          const float lightIntensityPlusMinimum,
                                                          const FLinearColor color)
{
   _ActualLightIntensityFromLightSources = actualLightIntensity;
   _CurrentLightIntensityPlusMinimumValue = lightIntensityPlusMinimum;
   _CurrentLightColor = color * _CurrentLightIntensityPlusMinimumValue;
   if(LightDetectionCVars::DebugLightDetection != 0)
   {
      DrawDebugSphere(
         GetWorld(),
         _CharacterOwner->GetActorLocation(),
         _CharacterOwner->GetSimpleCollisionRadius() * 2.f,
         4,
         _CurrentLightColor.ToFColor(false),
         false,
         0.1f);
      
      GEngine->AddOnScreenDebugMessage(
         9998,
         1.f,
         FColor::Red,
         FString::Printf(TEXT("Actual Light Value: %f"), _ActualLightIntensityFromLightSources));
      
      GEngine->AddOnScreenDebugMessage(
         9999,
         1.f,
         FColor::Red,
         FString::Printf(TEXT("Calculated Light Value: %f"), _CurrentLightIntensityPlusMinimumValue));
   }
}

bool UOSELightDetectionComponent::CanHandleCalculation() const
{
   if(_CharacterOwner == nullptr)
      return false;
   return _CharacterOwner->GetLocalRole() >= ROLE_AutonomousProxy;
}

FTransform UOSELightDetectionComponent::GetLightDetectionTransform() const
{
   return _CharacterOwner->GetTransform();
}
