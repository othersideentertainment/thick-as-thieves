// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "TATDisguiseTool.h"
#include "Tools/TATToolComponent.h"
#include "Tools/TATToolFunctionLibrary.h"

#include "TATDisguiseToolComponent.generated.h"

class ACharacter;
class USkeletalMesh;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct TAT_API FTATDisguiseToolParams
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Tool Params")
   TObjectPtr<USkeletalMesh> ToolMesh1P;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Tool Params")
   TObjectPtr<USkeletalMesh> ToolMesh3P;
};

UCLASS()
class TAT_API UTATDisguiseToolComponent : public UTATToolComponent
{
   GENERATED_BODY()

public:
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   virtual bool OnUnequip_Implementation() override;

   /// Called when disguise is enabled or disabled for a character with this tool in their toolset
   UFUNCTION(BlueprintNativeEvent, Category = "Disguise Tool")
   void OnDisguiseStateChanged(bool disguiseActive, const FDisguiseSnapshot& snapshot);
   void OnDisguiseStateChanged_Implementation(bool disguiseActive, const FDisguiseSnapshot& snapshot) {}

   UFUNCTION(BlueprintNativeEvent, Category = "Disguise Tool")
   void OnDisguiseParamsUpdated(const FTATDisguiseToolParams& newParams);
   void OnDisguiseParamsUpdated_Implementation(const FTATDisguiseToolParams& newParams);

   UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Disguise Tool")
   bool IsValidDisguiseTarget(AActor* actor) const;
   bool IsValidDisguiseTarget_Implementation(AActor* actor) const { return true; }

   /// Sets the appearance of this tool to the first default tool of a character type
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Disguise Tool")
   void AuthoritySetToolDisguiseFromCharacterType(TSubclassOf<ACharacter> disguisedAs);

   /// Make this tool look like another tool
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Disguise Tool")
   void AuthoritySetToolDisguise(TSubclassOf<UToolComponent> disguisedAsTool);

   /// Restores the original param data from the CDO
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Disguise Tool")
   void AuthorityRevertToolDisguise();

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Tool")
   FDisguiseSnapshot DisguiseSnapshot;

   /// Max range for targeting a character to steal an appearance from
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Tool")
   float StealRange = 2000.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Tool", Meta = (InlineEditConditionToggle))
   bool UsePreventStealWhenSeen = true;

   /// Don't allow stealing an appearance from an actor who has line of sight on you.
   ///
   /// Note that this is a reverse trace check back to the player, so for these params, source is the actor whose appearance will potentially be copied
   /// and target is the owner of the disguise tool.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Tool", Meta = (EditCondition = "UsePreventStealWhenSeen"))
   FTATLineOfSightTraceParams PreventStealWhenSeen;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Tool")
   FCollisionProfileName StealTraceProfile;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Tool")
   FGameplayTagContainer StealTargetTags;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Tool")
   FGameplayTagContainer DisguiseActiveTags;

   /// The disguise is auto-cancelled as soon as any of these gameplay tags are present on the character
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Disguise Cancellation Triggers")
   FGameplayTagQuery AbilityQueryToDisableDisguise;

   /// Data Table of FDisguiseIntegrityReductionData to specify how disguise reductions happen
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Disguise Integrity System")
   TObjectPtr<UDataTable> DisguiseIntegrityReductionTable;

   /// Maximum disguise integrity value
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Disguise Integrity System")
   float MaxDisguiseIntegrity = 100.0f;

   UPROPERTY(EditDefaultsOnly)
   bool ShouldRemoveDisguiseOnUnEquip { false };

   /// Particle system to spawn on nearby characters that would degrade the disguise integrity
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Disguise Integrity System")
   TObjectPtr<UNiagaraSystem> NearbyCharacterStateRadiusEffect;

protected:
   UFUNCTION()
   void _OnRep_DisguiseToolParams();

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_DisguiseToolParams)
   FTATDisguiseToolParams _disguiseToolParams;

};
