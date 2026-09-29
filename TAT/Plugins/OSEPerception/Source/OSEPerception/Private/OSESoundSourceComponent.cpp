// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESoundSourceComponent.h"

#include "OSESense_HearingContinuous.h"

//UE
#include "OSEPerceptionSystem.h"


void UOSESoundSourceComponent::RegisterSense()
{
   TSubclassOf<UOSESense> SenseClass = UOSESense_HearingContinuous::StaticClass();
   AActor* OwnerActor = GetOwner();
   if (OwnerActor == nullptr)
   {
      return;
   }

   UWorld* World = OwnerActor->GetWorld();
   if (World)
   {
      UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(World);
      if (PerceptionSystem)
      {
         PerceptionSystem->RegisterSourceForSenseClass(SenseClass, *OwnerActor);

         bSuccessfullyRegistered = true;
      }
   }
}

void UOSESoundSourceComponent::UnregisterSense()
{
   TSubclassOf<UOSESense> SenseClass = UOSESense_HearingContinuous::StaticClass();

   AActor* OwnerActor = GetOwner();
   if (OwnerActor == nullptr)
   {
      return;
   }

   UWorld* World = OwnerActor->GetWorld();
   if (World)
   {
      UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(World);
      if (PerceptionSystem)
      {
         PerceptionSystem->UnregisterSource(*OwnerActor, SenseClass);
         bSuccessfullyRegistered = false;
      }
   }
}


// Sets default values for this component's properties
UOSESoundSourceComponent::UOSESoundSourceComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UOSESoundSourceComponent::BeginPlay()
{
	Super::BeginPlay();

   if (!VelocityDriven)
   {
      SetComponentTickEnabled(false);
      RegisterSense();
   }
}


// Called every frame
void UOSESoundSourceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

   if(VelocityDriven)
   {
      const bool bIsMoving = GetOwner()->GetVelocity().Size() > MinimumSpeedTrigger;
      if (bIsMoving && !bSuccessfullyRegistered)
      {
         RegisterSense();
      }
      else if(!bIsMoving && bSuccessfullyRegistered)
      {
         UnregisterSense();
      }
   }
}

void UOSESoundSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
   Super::EndPlay(EndPlayReason);
 
   if (bSuccessfullyRegistered)
   {
      UnregisterSense();
   }
}

