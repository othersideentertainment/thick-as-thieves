// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"

#include "TATPlayerPostProcStackComponent.generated.h"

class ATATPlayerPostProc;

USTRUCT()
struct FTATPlayerPostProcBinding
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery Query;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<ATATPlayerPostProc> PostProcClass;

   UPROPERTY(Transient)
   ATATPlayerPostProc* PostProcInstance = nullptr;
};


// -----------------------------------------------------------------------------------------------------------------------------------------
/// UTATPlayerPostProcStackComponent
///
/// A component attachable to controllers that automatically manages a set of post process components.
/// The set of post procs and their tag query are defined: the psot procs are automatically enabled/disabled as the tags on
/// the currently possessed player pawn change
// -----------------------------------------------------------------------------------------------------------------------------------------
UCLASS()
class TAT_API UTATPlayerPostProcStackComponent : public UActorComponent
{
   GENERATED_BODY()
public:

   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type reason) override;

   UFUNCTION(BlueprintPure, Category = "Graphics|TAT", meta = (DeterminesOutputType = "postProcClass"))
   ATATPlayerPostProc* GetPostProcInstanceByClass(TSubclassOf<ATATPlayerPostProc> postProcClass) const;

   UPROPERTY(EditDefaultsOnly)
   TArray<FTATPlayerPostProcBinding> PostProcBindings;

private:
   UFUNCTION()
   void _OnOwnerPawnChanged(APawn* newPawn);

   UFUNCTION()
   void _OnOwnerTagChanged(FGameplayTag tag, int32 count);

   UFUNCTION()
   void _OnOwnerPawnAbilitiesInitialized();

   void _BindToTagChanges(APawn* newPawn);
   void _UnbindToTagChanges(APawn* newPawn);

   void _SpawnPostProcInstances();
   void _DestroyPostProcInstances();

   void _RefreshPostProcs();
   void _InitializeRelevantTags();

   TMap<FGameplayTag, FDelegateHandle> _tagChangeBindings;

   TSet<FGameplayTag> _allRelevantTags;

   TWeakObjectPtr<APawn> _currentlyPossessedPawn;
};

