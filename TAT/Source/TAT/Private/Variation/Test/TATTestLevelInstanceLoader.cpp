// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Test/TATTestLevelInstanceLoader.h"


// ue5
#include "Engine/LevelStreamingDynamic.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTestLevelInstanceLoader)

// Sets default values
ATATTestLevelInstanceLoader::ATATTestLevelInstanceLoader()
{
   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
   RootComponent->Mobility = EComponentMobility::Static;
#if WITH_EDITORONLY_DATA
   RootComponent->bVisualizeComponent = true;
#endif

   PrimaryActorTick.bCanEverTick = false;
   bReplicates = true;

}

// Called when the game starts or when spawned
void ATATTestLevelInstanceLoader::BeginPlay()
{
   Super::BeginPlay();
   
   if (HasAuthority() && PossibleLevels.Num() > 0)
   {
      _levelToLoad = PossibleLevels[FMath::RandHelper(PossibleLevels.Num())];
      _TryLoadInstance();
   }
}

void ATATTestLevelInstanceLoader::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATTestLevelInstanceLoader, _levelToLoad);
}

void ATATTestLevelInstanceLoader::_OnRep_LevelToLoad()
{
   _TryLoadInstance();
}

void ATATTestLevelInstanceLoader::_TryLoadInstance()
{
   const FString shortPackageName = FPackageName::GetShortName(_levelToLoad.GetLongPackageName());
   const FString nameOverride = FString::Printf(TEXT("%s_%s"), *shortPackageName, *LevelSuffix);

   UWorld* world = GetWorld();
   ULevelStreamingDynamic::FLoadLevelInstanceParams params(world, _levelToLoad.GetLongPackageName(), GetActorTransform());
   params.OptionalLevelNameOverride = &nameOverride;
   params.bLoadAsTempPackage = true;
   params.bInitiallyVisible = true;

   bool outSuccess = false;
   _streamingLevel = ULevelStreamingDynamic::LoadLevelInstance(params, outSuccess);
}


