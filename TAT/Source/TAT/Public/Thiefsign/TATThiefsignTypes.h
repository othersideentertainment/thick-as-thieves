// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"

#include "TATThiefsignTypes.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class UPaperSprite;
class UTexture;

UENUM(BlueprintType)
enum class ETATThiefsignType : uint8
{
   // Thiefsign glyph will appear in a player's hand
   Hand,

   // Thiefsign glyph will be left by player on in-world surface
   Decal,

   // A designated Animation Montage and Gameplay Cue will play
   Emote
};

UENUM(BlueprintType)
enum class ETATThiefsignMaterialParamPerspective : uint8
{
   // Apply param to both first and third person
   FirstAndThirdPerson,

   // Apply param to first person only
   FirstPerson,

   // Apply param to third person only
   ThirdPerson
};

USTRUCT(BlueprintType)
struct TAT_API FTATThiefsignMaterialParamKey
{
   GENERATED_BODY()

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
   FName ParamName = NAME_None;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
   ETATThiefsignMaterialParamPerspective Perspective = ETATThiefsignMaterialParamPerspective::FirstAndThirdPerson;

   FORCEINLINE bool operator==(const FTATThiefsignMaterialParamKey& other) const
   {
      return (ParamName == other.ParamName)
         && (Perspective == other.Perspective);
   }
};

inline uint32 GetTypeHash(const FTATThiefsignMaterialParamKey& paramKey)
{
   return HashCombine(GetTypeHash(paramKey.ParamName), GetTypeHash(paramKey.Perspective));
}

USTRUCT(BlueprintType)
struct TAT_API FTATThiefsignMaterialParameters
{
   GENERATED_BODY()

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
   TMap<FTATThiefsignMaterialParamKey, TSoftObjectPtr<UTexture>> TextureParameters;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
   TMap<FTATThiefsignMaterialParamKey, FLinearColor> ColorParameters;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
   TMap<FTATThiefsignMaterialParamKey, float> FloatParameters;

   void AppendParams(const FTATThiefsignMaterialParameters& overrides);

   void ApplyToMaterial(UMaterialInstanceDynamic* material, bool isFirstPerson, TFunction<void()>&& applyFinishedCallback) const;
};

USTRUCT(BlueprintType)
struct TAT_API FTATThiefsignNiagaraParameters
{
   GENERATED_BODY()

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
   TMap<FTATThiefsignMaterialParamKey, TSoftObjectPtr<UTexture>> TextureParameters;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
   TMap<FTATThiefsignMaterialParamKey, FLinearColor> ColorParameters;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
   TMap<FTATThiefsignMaterialParamKey, float> FloatParameters;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
   TMap<FTATThiefsignMaterialParamKey, int> IntegerParameters;

   void AppendParams(const FTATThiefsignNiagaraParameters& other);

   void ApplyToNiagaraSystem(UNiagaraComponent* niagaraSystem, bool isFirstPerson, TFunction<void()>&& applyFinishedCallback) const;
};

UENUM(meta = (Bitflags))
enum class ETATThiefsignVFXConfigShowType : uint8
{
   ShowClasses    = 1 << 0,
   ShowMaterials  = 1 << 1,
   ShowNiagara    = 1 << 2,

   // helpers
   ShowAll                 = ShowClasses | ShowMaterials | ShowNiagara,
   HideClasses             = ShowMaterials | ShowNiagara,
   HideNiagara             = ShowClasses | ShowMaterials,
   HideClassesAndNiagara   = ShowMaterials
};
ENUM_CLASS_FLAGS(ETATThiefsignVFXConfigShowType);

USTRUCT(BlueprintType)
struct TAT_API FTATThiefsignVFXConfig
{
   GENERATED_BODY()

   FTATThiefsignVFXConfig(ETATThiefsignVFXConfigShowType displayType = ETATThiefsignVFXConfigShowType::ShowAll)
      : _displayType(displayType) { }

   // Material to apply to billboard/decal component spawned for thiefsign symbol VFX
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "_displayType & '/Script/TAT.ETATThiefsignVFXConfigShowType::ShowClasses'", EditConditionHides))
   TSoftObjectPtr<UMaterialInterface> MaterialClass = nullptr;

   // Configure parameters on supplied MaterialClass, if non-null
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATThiefsignMaterialParameters MaterialParams;

   // Niagara system to spawn for thiefsign symbol VFX
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "_displayType & '/Script/TAT.ETATThiefsignVFXConfigShowType::ShowClasses' && _displayType & '/Script/TAT.ETATThiefsignVFXConfigShowType::ShowNiagara'", EditConditionHides))
   TSoftObjectPtr<UNiagaraSystem> NiagaraSystemClass = nullptr;

   // Configure variables on supplied NiagaraSystemClass, if non-null
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "_displayType & '/Script/TAT.ETATThiefsignVFXConfigShowType::ShowNiagara'", EditConditionHides))
   FTATThiefsignNiagaraParameters NiagaraSystemParams;

   bool IsValid() const;
   void Merge(const FTATThiefsignVFXConfig& other);

private:
   UPROPERTY()
   ETATThiefsignVFXConfigShowType _displayType;
};

USTRUCT(BlueprintType)
struct TAT_API FTATThiefsignInfo : public FTableRowBase
{
   GENERATED_BODY()

public:
   // Unique identifier used to distinguish between thiefsign signals
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Thiefsign"))
   FGameplayTag Identifier = FGameplayTag::EmptyTag;

   // Is this thisfsign signal enabled for use in-game?
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool IsEnabled = true;

   // Player-facing name for this thiefsign signal
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText DisplayName;

   // Player-facing icon used in thiefsign menu
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayThumbnail = "true"))
   TSoftObjectPtr<UPaperSprite> DisplayUISprite = nullptr;

   // Tag that represents what animation to play when this thiefsign is selected
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Animation.Character"))
   FGameplayTag AnimationTag;

   // Visual representation of the hand-UX thiefsign symbol in-game
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATThiefsignVFXConfig HandVFXConfigOverrides = FTATThiefsignVFXConfig(ETATThiefsignVFXConfigShowType::HideClasses);

   // Visual representation of the decal-UX thiefsign symbol in-game
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FTATThiefsignVFXConfig DecalVFXConfigOverrides = FTATThiefsignVFXConfig(ETATThiefsignVFXConfigShowType::HideClassesAndNiagara); 
};
