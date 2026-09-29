// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "TATAnimSetMapping.generated.h"

class UAnimInstance;
class UTATAnimSetMappingSchema;

// An asset that has the AnimSets that a character uses in different situations
// (e.g. unarmed, vs two-handed-tool vs downed, etc)
//
// AnimSets are animation layer ABPs that provide the basic locomotion.
// Tool-specific anim layers are separate, but tools may request a specific
// layer.
UCLASS(AutoExpandCategories="AnimSets")
class TAT_API UTATAnimSetMapping : public UDataAsset
{
   GENERATED_BODY()

public:
   TSubclassOf<UAnimInstance> GetAnimSetByTag(FGameplayTag tag) const;

   // TODO: add validation and schema

   // The default unarmed anim set
   UPROPERTY(EditDefaultsOnly, Category=AnimSets)
   TSubclassOf<UAnimInstance> DefaultAnimSet;

   UPROPERTY(EditDefaultsOnly, Category=AnimSets, meta = (Categories=AnimSet, ForceInlineRow))
   TMap<FGameplayTag, TSubclassOf<UAnimInstance>> AnimSetsByTag;

#if WITH_EDITORONLY_DATA
   // The schema to validate anim sets against for required values
   UPROPERTY(EditDefaultsOnly, Category=Validation)
   TObjectPtr<UTATAnimSetMappingSchema> Schema;
#endif

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};

// A schema asset to enforce that a mapping has required anim sets
UCLASS()
class TAT_API UTATAnimSetMappingSchema : public UDataAsset
{
   GENERATED_BODY()
public:
   // AnimSets that mappings are required to provide
   UPROPERTY(EditDefaultsOnly, Category=Schema, meta = (Categories = AnimSet))
   FGameplayTagContainer RequiredAnimSets;

   // If set to true, will warn if a mapping provides AnimSets that are not in this schema
   UPROPERTY(EditDefaultsOnly, Category=Schema)
   bool WarnOnExtraAnimSets = false;
};
