// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSEBugSubmitter.h"

#include "OSEBugReporterSettings.h"
#include "OSEBugData.h"

#include "OSEVersionLibrary.h"

#include "HAL/PlatformProperties.h"
#include "HAL/PlatformMisc.h"

#include "Character/OSECharacterBase.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"


void FOSEBugSubmitter::SubmitBug(const TArray<FString>& Args, UWorld* world)
{
   if (!IsValid(world))
   {
      return;
   }

   const APlayerController* playerController = (*world->GetPlayerControllerIterator()).Get(0);
   const APawn* playerPawn = playerController ? playerController->GetPawnOrSpectator() : nullptr;

   FOSEBugData BugData;
   
   BugData.BuildVersion = UOSEVersionLibrary::GetBuildVersionString();
   BugData.BuildConfiguration = LexToString(FApp::GetBuildConfiguration());
   BugData.Platform = FPlatformProperties::IniPlatformName();
   BugData.GPU = FPlatformMisc::GetPrimaryGPUBrand();

   BugData.MapName = world->GetMapName();
   BugData.WorldTime = world->GetRealTimeSeconds();
   
   if (IsValid(playerPawn))
   {
      BugData.WorldPosition = playerPawn->GetActorLocation();

      APlayerCameraManager* cameraManager = playerController->PlayerCameraManager.Get();
      if (IsValid(cameraManager))
      {
         FVector cameraLocation = cameraManager->GetCameraLocation();
         FVector cameraForwardVector = cameraManager->GetCameraRotation().Vector();

         FHitResult outHit;
         world->LineTraceSingleByChannel(outHit, cameraLocation, cameraLocation + (cameraForwardVector * 10000.0f), ECC_Visibility);

         if (outHit.bBlockingHit)
         {
            // Collect data on object directly in front of camera
            AActor* directHit = outHit.GetActor();
            AOSECharacterBase* characterBase = Cast<AOSECharacterBase>(directHit);
            if (IsValid(characterBase))
            {
               BugData.DirectObjectData = GetAttributesString(world, characterBase);
            }
            else
            {
               BugData.DirectObjectData = directHit->GetName();
            }

            // Collect data on objects within a radius around the object directly in front of camera
            TArray<FHitResult> outHits;
            world->SweepMultiByChannel(outHits, outHit.ImpactPoint, outHit.ImpactPoint, FQuat::Identity, ECollisionChannel::ECC_Visibility, FCollisionShape::MakeSphere(1000.0f));

            TArray<AActor*> hitActors;
            for (FHitResult& singleHit : outHits)
            {
               if (singleHit.GetActor() != directHit)
               {
                  hitActors.AddUnique(singleHit.GetActor());
               }
            }

            FString nonCharacterNames;
            for (AActor* singleActor : hitActors)
            {
               characterBase = Cast<AOSECharacterBase>(singleActor);
               if (IsValid(characterBase))
               {
                  BugData.RadiusObjectData += GetAttributesString(world, characterBase);
                  BugData.RadiusObjectData += "\n";
               }
               else
               {
                  nonCharacterNames += singleActor->GetName() + "\n";
               }            
            }

            BugData.RadiusObjectData += nonCharacterNames;
         }
      }
   }
   
   FString argString;
   for (auto& arg : Args)
   {
      argString += " " + arg;
   }
   BugData.Summary = BugData.Description = argString;

   const UOSEBugReporterSettings*settings = GetDefault<UOSEBugReporterSettings>();

   settings->ReportManager->GetDefaultObject<UOSEBugReportManager>()->ReportBug(BugData);
}

FString FOSEBugSubmitter::GetAttributesString(UWorld* world, AOSECharacterBase* character)
{
   FString attributeString;
   UAbilitySystemComponent* abilitySystem = IsValid(character) ? character->GetAbilitySystemComponent() : NULL;
   if (IsValid(abilitySystem))
   {
      attributeString += character->GetName() + "\n";

      TArray<FGameplayAttribute> Attributes;
      abilitySystem->GetAllAttributes(Attributes);

      for (FGameplayAttribute& Attribute : Attributes)
      {
         bool bFoundAttribute;
         attributeString += Attribute.GetName() + ": " + FString::Printf(TEXT(": %f\n"), abilitySystem->GetGameplayAttributeValue(Attribute, bFoundAttribute));
      }
   }

   return attributeString;
}
