// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "VoiceOver/OSEVoiceOverLineRequestParams.h"

// ue4
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"

#include "OSEVoiceOverLineHelperComponent.generated.h"

class UAbilitySystemComponent;

///////////////////////////////////////////////////////////////////
///        EOSEGameplayTagToVOLineTriggerFlags
///////////////////////////////////////////////////////////////////

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EOSEGameplayTagToVOLineTriggerFlags : uint8
{
   None            = 0        UMETA(Hidden),
   CountAdd        = 1 << 0   UMETA(DisplayName = "Add"),       // First time this tag is added
   CountIncrement  = 1 << 1   UMETA(DisplayName = "Increment"), // Each time this tag is incremented
   CountDecrement  = 1 << 2   UMETA(DisplayName = "Decrement"), // Each time this tag is decremented
   CountRemove     = 1 << 3   UMETA(DisplayName = "Remove"),    // When this tag is removed
};
ENUM_CLASS_FLAGS(EOSEGameplayTagToVOLineTriggerFlags);

///////////////////////////////////////////////////////////////////
///        FGameplayTagToVOLine
///////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct OSECORE_API FOSEGameplayTagToVOLine
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Over")
   FGameplayTag GameplayTag;

   UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/OSECore.EOSEGameplayTagToVOLineTriggerFlags"), Category = "Voice Over")
   uint32 TriggersOnFlags = 0;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Over")
   FOSEVoiceOverLineRequestParams RequestParams;
};

///////////////////////////////////////////////////////////////////
///        UOSEGameplayTagVOLineDataAsset
///////////////////////////////////////////////////////////////////

UCLASS(BlueprintType)
class OSECORE_API UOSEGameplayTagVOLineDataAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Over")
   TArray<FOSEGameplayTagToVOLine> VoiceOvers;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) override;
#endif
};

///////////////////////////////////////////////////////////////////
///        UOSEVoiceOverLineHelperComponent
///////////////////////////////////////////////////////////////////

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = "Voice Over")
class OSECORE_API UOSEVoiceOverLineHelperComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTriggerVoiceLine, const FOSEVoiceOverLineRequestParams&, params);

public:
   UOSEVoiceOverLineHelperComponent();

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   UPROPERTY(EditAnywhere, Category = "Voice Over")
   TArray<UOSEGameplayTagVOLineDataAsset*> GameplayTagVOLineAssets;

   UPROPERTY(BlueprintAssignable, Category = "Voice Over")
   FOnTriggerVoiceLine AuthorityOnTriggerVoiceLine;

private:
   void _AuthorityBindToAbilitySystemComponent();
   void _AuthorityUnbindFromAbilitySystemComponent();
   UAbilitySystemComponent* _GetAbilitySystemComponent() const;
   void _ForEachTagToVOLine(const TFunction<void(const FOSEGameplayTagToVOLine&)>& cb) const;

private:
   void _AuthorityOnGameplayTagChanged(const FGameplayTag tag, int32 newCount);

private:
   FGameplayTagCountContainer _trackedTagCounts;
};
