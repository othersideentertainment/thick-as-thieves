// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"

// ose
#include "OSEConditionalVoiceLineDataAsset.h"
#include "OSEIndividualKnowledgeComponent.h"

#include "OSEVoiceLineKnowledgeComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OSEVOICELINEKNOWLEDGE_API UOSEVoiceLineKnowledgeComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSEVoiceLineKnowledgeComponent();

   virtual void InitializeComponent() override;

   bool GetVoiceVerbForConditionalVoiceLine(const AActor* actor,
                                            const FGameplayTag conditionalVoiceLineTag,
                                            FGameplayTag& outVoiceVerbTag) const;
   bool GetVoiceVerbForConditionalVoiceLineAndContainer(const FGameplayTag conditionalVoiceLineTag,
                                                        const FGameplayTagContainer& voiceLineTagContainer,
                                                        FGameplayTag&
                                                        outVoiceVerbTag) const;
private:
   const TObjectPtr<UOSEConditionalVoiceLineDataAsset>* GetVoiceLineDataAssetForTag(FGameplayTag conditionalVoiceLineTag) const;

   UPROPERTY(EditDefaultsOnly)
   TArray<TObjectPtr<UOSEConditionalVoiceLineDataAsset>> _voiceLineDataMap;

   UPROPERTY(Transient)
   UOSEIndividualKnowledgeComponent* _individualKnowledgeComponent { nullptr };
};
