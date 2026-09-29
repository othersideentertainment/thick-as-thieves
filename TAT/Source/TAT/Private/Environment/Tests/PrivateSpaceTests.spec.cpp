// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

// tat
#include "Character/TATCharacterAIBase.h"
#include "Disguise/TATDisguiseComponent.h"
#include "Environment/TATPrivateSpaceGameplayTagDefines.h"
#include "Environment/TATPrivateSpaceVolume.h"
#include "Player/TATCharacter.h"
#include "Player/TATPlayerController.h"

// ose
#include "Player/OSEPlayerState.h"

// ue5
#include "Misc/AutomationTest.h"
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "CollectionManagerModule.h"
#include "ICollectionManager.h"
#include "Components/BrushComponent.h"

#include "Tests/AutomationEditorCommon.h"

// NOTE : temporarily disabled, since it hangs starting PIE in 5.4
BEGIN_DEFINE_SPEC(FPrivateSpaceSpec, "TAT.AI.PrivateSpace", EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::Disabled)
// Define class variables here
TObjectPtr<UWorld> world;
FDelegateHandle handle;
END_DEFINE_SPEC(FPrivateSpaceSpec)

template< class T >
void SpawnAndPossessCharacter(UWorld* world, const FSoftObjectPath& actorClassPath, TObjectPtr<T>& characterPointer, const TSubclassOf<AController> controllerClass)
{
   UObject* actorClass = actorClassPath.TryLoad();
   check(actorClass);

   const UBlueprint* asBlueprint = CastChecked<UBlueprint>(actorClass);
   
   FActorSpawnParameters params;
   params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   characterPointer = world->SpawnActor<T>(asBlueprint->GeneratedClass, params);

   if(controllerClass)
   {
      FActorSpawnParameters spawnInfo;
      spawnInfo.Instigator = characterPointer->GetInstigator();
      spawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
      spawnInfo.OverrideLevel = characterPointer->GetLevel();
      spawnInfo.ObjectFlags |= RF_Transient;	// We never want to save AI controllers into a map
      AController* newController = world->SpawnActor<AController>(controllerClass, characterPointer->GetActorLocation(), characterPointer->GetActorRotation(), spawnInfo);
      if(newController->IsA<ATATPlayerController>())
      {
         if(const ATATPlayerController* asPlayerController = Cast<ATATPlayerController>(newController))
         {
            asPlayerController->GetPlayerState<AOSEPlayerState>()->AuthoritySetTeam(1);
         }
      }
      if (newController != nullptr)
      {
         newController->Possess(characterPointer);
      }
   }
   else
   {
      characterPointer->SpawnDefaultController();
   }
   check(characterPointer);
}

void CreatePrivateZone(UWorld* world, ATATPrivateSpaceVolume*& volume, const FVector& location)
{
   FActorSpawnParameters params;
   params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   volume = world->SpawnActor<ATATPrivateSpaceVolume>(ATATPrivateSpaceVolume::StaticClass(), location, FRotator::ZeroRotator, params);

   // The Brush Volume builder moves the actor, to remove warnings, flagging as movable.
   volume->GetBrushComponent()->SetMobility(EComponentMobility::Movable);

   // We need to manually create a box volume here, in the editor placing the actor will generate a cube -
   // HOWEVER spawning a volume at runtime will lead to no basic cube to be created.
   
   UCubeBuilder* builder = NewObject<UCubeBuilder>();
   UActorFactory::CreateBrushForVolumeActor(volume, builder);

   // Cause volume to recognise new colliders and create overlaps
   // DO NOT notify of the overlap before the initial overlaps are handled.
   
   volume->UpdateOverlaps(false);
   volume->HandleInitialOverlaps();
}
void FPrivateSpaceSpec::Define()
{
   LatentBeforeEach([this](const FDoneDelegate& doneDelegate)
   {
      FAutomationEditorCommonUtils::LoadMap("/Game/AI/Tests/TEST_Empty");
      handle = FEditorDelegates::PostPIEStarted.AddLambda([this, doneDelegate](const bool bIsSimulatingInEditor)
      {
         if(bIsSimulatingInEditor == false)
         {
            world = GEditor->GetPIEWorldContext()->World();
            if(world->AreActorsInitialized())
            {
               doneDelegate.Execute();
               FEditorDelegates::PostPIEStarted.Remove(handle);
            }
            else
            {
               world->OnActorsInitialized.AddLambda([doneDelegate, this](const UWorld::FActorsInitializedParams params)
               {
                  doneDelegate.Execute();
                  FEditorDelegates::PostPIEStarted.Remove(handle);
               });
            }
         }
      });
      FStartPIECommand* startPieCommand = new FStartPIECommand(false);
      while(startPieCommand->Update() == false){}
   });
   
   TArray<FSoftObjectPath> aiCharacterSoftPaths;
   TArray<FSoftObjectPath> playerCharacterPaths;
   const ICollectionManager& collectionManager = FCollectionManagerModule::GetModule().Get();
   collectionManager.GetObjectsInCollection(TEXT("AI_Characters_ForTest"), ECollectionShareType::CST_All, aiCharacterSoftPaths, ECollectionRecursionFlags::All);
   collectionManager.GetObjectsInCollection(TEXT("Player_Characters_ForTest"), ECollectionShareType::CST_All, playerCharacterPaths, ECollectionRecursionFlags::All);

   
   Describe(TEXT("public / private tests"), [this, aiCharacterSoftPaths, playerCharacterPaths]()
   {
      LatentIt(TEXT("Test all characters in Player_Characters_ForTest"),[this, aiCharacterSoftPaths, playerCharacterPaths](const FDoneDelegate& doneDelegate)
      {
         for (auto playerCharacterPath : playerCharacterPaths)
         {
            TObjectPtr<ATATCharacter> intruderCharacter;
            SpawnAndPossessCharacter(world, playerCharacterPath, intruderCharacter, ATATPlayerController::StaticClass());
            intruderCharacter->SetActorLocation(FVector(5000,0,0));

            ATATPrivateSpaceVolume* privateSpaceVolume = nullptr;
            CreatePrivateZone(world, privateSpaceVolume, FVector(0,0,0));
            intruderCharacter->SetActorLocation(privateSpaceVolume->GetActorLocation());

            TestTrue(FString::Printf(TEXT("Character [%s] is flagged as intruder after entering"),*playerCharacterPath.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));

            intruderCharacter->SetActorLocation(FVector(5000,0,0));

            TestFalse(FString::Printf(TEXT("Character [%s] is not flagged as intruder after leaving"),*playerCharacterPath.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));
            
            UTATDisguiseComponent* disguiseComponent = ITATDisguisableCharacterInterface::Execute_GetDisguiseComponent(intruderCharacter);
            
            FDisguiseSnapshot disguiseSnapshot;
            disguiseSnapshot.Team = 2;
            disguiseSnapshot.ActorClass = intruderCharacter->GetClass();
            disguiseSnapshot.AllowedPrivateZones.AddTag(privateSpaceVolume->GetPrivateZoneGameplayTag());
            
            disguiseComponent->AuthorityActivateDisguise(disguiseSnapshot, intruderCharacter);
            intruderCharacter->SetActorLocation(privateSpaceVolume->GetActorLocation());

            TestFalse(FString::Printf(TEXT("Character [%s] is not flagged as intruder after entering with a disguise"),*playerCharacterPath.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));
            disguiseComponent->AuthorityEndDisguise();
            TestTrue(FString::Printf(TEXT("Character [%s] is flagged as intruder after disguise ends"),*playerCharacterPath.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));
            disguiseComponent->AuthorityActivateDisguise(disguiseSnapshot, intruderCharacter);
            TestFalse(FString::Printf(TEXT("Character [%s] is not flagged as intruder after equipping disguise"),*playerCharacterPath.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));
         }
         doneDelegate.Execute();
      });
      LatentIt(TEXT("Test all characters in AI_Characters_ForTest"),[this, aiCharacterSoftPaths, playerCharacterPaths](const FDoneDelegate& doneDelegate)
      {
         for (auto aiCharacterPath : aiCharacterSoftPaths)
         {
            {
               TObjectPtr<ATATCharacterAIBase> intruderCharacter;
               SpawnAndPossessCharacter(world, aiCharacterPath, intruderCharacter, nullptr);
               intruderCharacter->SetActorLocation(FVector(5000,0,0));
         
               ATATPrivateSpaceVolume* privateSpaceVolume = nullptr;
               CreatePrivateZone(world, privateSpaceVolume, intruderCharacter->GetActorLocation());
         
               TestFalse(FString::Printf(TEXT("Character [%s] is not flagged as intruder"),*aiCharacterPath.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));
         
               privateSpaceVolume->Destroy();
               intruderCharacter->Destroy();
         
               SpawnAndPossessCharacter(world, aiCharacterPath, intruderCharacter, nullptr);
               intruderCharacter->SetActorLocation(FVector(10000,0,0));

               CreatePrivateZone(world, privateSpaceVolume, intruderCharacter->GetActorLocation());
         
               intruderCharacter->SetActorLocation(FVector(6000,0,0));
               TestFalse(FString::Printf(TEXT("Character [%s] is not flagged as intruder AFTER leaving space"),*aiCharacterPath.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));
               intruderCharacter->SetActorLocation(privateSpaceVolume->GetActorLocation());
               TestFalse(FString::Printf(TEXT("Character [%s] is not flagged as intruder AFTER entering space"),*aiCharacterPath.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));
         
               privateSpaceVolume->Destroy();
               intruderCharacter->Destroy();
            }
            for (auto otherCharacter : aiCharacterSoftPaths)
            {
               if(aiCharacterPath == otherCharacter)
                  continue;
               TObjectPtr<ATATCharacterAIBase> intruderCharacter;
               TObjectPtr<ATATCharacterAIBase> guardCharacter;

               SpawnAndPossessCharacter(world, aiCharacterPath,guardCharacter, nullptr);
               SpawnAndPossessCharacter(world, otherCharacter,intruderCharacter, nullptr);
            
               intruderCharacter->SetActorLocation(FVector(5000,0,0));
            
               ATATPrivateSpaceVolume* privateSpaceVolume = nullptr;
               CreatePrivateZone(world, privateSpaceVolume, FVector(10000,0,0));

               TestTrue(FString::Printf(TEXT("Character [%s] is NOT hostile"), *otherCharacter.GetAssetName()),
                        UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(
                           guardCharacter, intruderCharacter,
                           ETATTeamDisguiseHandling::UseApparentTeam) != EOSETeamAttitude::Hostile);

               intruderCharacter->SetActorLocation(privateSpaceVolume->GetActorLocation(), true);

               if(intruderCharacter->TeamCharacter == ETATTeamCharacterType::Guard)
               {
                  TestFalse(FString::Printf(TEXT("Character [%s] is NOT flagged as intruder AFTER entering space"), *otherCharacter.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));
               }
               else
               {
                  TestTrue(FString::Printf(TEXT("Character [%s] is flagged as intruder AFTER entering space"), *otherCharacter.GetAssetName()), intruderCharacter->HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_INTRUDING));
               }
            
               TestFalse(FString::Printf(TEXT("Character [%s] is NOT hostile to [%s]"), *otherCharacter.GetAssetName(), *aiCharacterPath.GetAssetName()), UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(
                                                guardCharacter, intruderCharacter,
                                                ETATTeamDisguiseHandling::UseApparentTeam) == EOSETeamAttitude::Hostile);
               privateSpaceVolume->Destroy();
            
               intruderCharacter->GetController()->Destroy();
               intruderCharacter->Destroy();
            
               guardCharacter->GetController()->Destroy();
               guardCharacter->Destroy();
            }
         }
         doneDelegate.Execute();
      });
   });
   
   LatentAfterEach([this](const FDoneDelegate& doneDelegate)
   {
     FEndPlayMapCommand* endPlayMap = new FEndPlayMapCommand();
     while(endPlayMap->Update() == false)
     {         
     }
     doneDelegate.Execute();
   });
}
#endif
