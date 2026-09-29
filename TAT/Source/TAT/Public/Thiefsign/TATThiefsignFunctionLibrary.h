// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Thiefsign/TATThiefsignSettings.h"
#include "Thiefsign/TATThiefsignTypes.h"

// ue
#include "Abilities/GameplayAbilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATThiefsignFunctionLibrary.generated.h"

class UNiagaraComponent;
class USceneComponent;

UCLASS()
class TAT_API UTATThiefsignFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
   // Helper function which builds a gameplay event payload to pass to a Thiefsign ability-triggering event.
   UFUNCTION(BlueprintPure)
   static FGameplayEventData BuildThiefsignAbilityEventData(FGameplayTag thiefsignIdentifier, FGameplayTag thiefsignAnimationIdentifier, AActor* instigator);

   // Retrieves the Thiefsign symbol identifier out of the specified gameplay event payload.
   UFUNCTION(BlueprintCallable)
   static void RetrieveThiefsignIdentifierFromEventData(const FGameplayEventData& eventData, FGameplayTag& thiefsignIdentifier);

   // Retrieves the Thiefsign animation identifier out of the specified gameplay event payload.
   UFUNCTION(BlueprintCallable)
   static void RetrieveThiefsignAnimationIdentifierFromEventData(const FGameplayEventData& eventData, FGameplayTag& thiefsignAnimationIdentifier);

   // Helper function which builds a gameplay cue parameters payload to pass to a Thiefsign ability-triggering gameplay cue.
   UFUNCTION(BlueprintPure)
   static FGameplayCueParameters BuildThiefsignCueParameters(FGameplayTag thiefsignIdentifier, AActor* instigator);

   // Retrieves the Thiefsign symbol identifier out of the specified gameplay cue payload.
   UFUNCTION(BlueprintCallable)
   static void RetrieveThiefsignIdentifierFromCueParameters(const FGameplayCueParameters& parameters, FGameplayTag& thiefsignIdentifier);

   // Retrieves the component and world position designated by Thiefsign cue parameters.
   UFUNCTION(BlueprintCallable)
   static bool FindThiefsignAttachmentFromCueParameters(const FGameplayCueParameters& parameters, const FTATThiefsignCharacterConfig& characterConfig, USceneComponent*& attachToComponent, FVector& worldLocation);

   // Combine two FTATThiefsignMaterialParameters structs, using the values from appendParams for parameter names found in both.
   UFUNCTION(BlueprintCallable)
   static void AppendThiefsignMaterialParams(UPARAM(Ref) FTATThiefsignMaterialParameters& baseParams, const FTATThiefsignMaterialParameters& appendParams);

   // Loads the niagara system class and creates it, applying the supplied parameters as variables in the niagara system.
   UFUNCTION(BlueprintCallable)
   static void CreateAndApplyThiefsignNiagaraSystemData(ETATThiefsignType type, const UObject* contextObject, const FGameplayTag& thiefsignIdentifier, bool isFirstPerson, USceneComponent* attachToComponent, FVector worldLocation);

};
