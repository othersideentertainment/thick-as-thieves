// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeSchema.h"

#include "TATSmartObjectStateTreeSchema.generated.h"


UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta = (DisplayName = "[TAT] Smart Object State Tree"))
class TAT_API UTATSmartObjectStateTreeSchema : public UStateTreeSchema
{
   GENERATED_BODY()

public:
   UTATSmartObjectStateTreeSchema();
   
   UClass* GetContextControllerClass() const { return _ContextControllerClass; };
   UClass* GetContextPawnClass() const { return _ContextPawnClass; };
   UClass* GetSmartObjectActorClass() const { return _SmartObjectActorClass; };

protected:
   virtual bool IsStructAllowed(const UScriptStruct* inScriptStruct) const override;
   virtual bool IsClassAllowed(const UClass* inClass) const override;
   virtual auto IsExternalItemAllowed(const UStruct& inStruct) const -> bool override;

   virtual TConstArrayView<FStateTreeExternalDataDesc> GetContextDataDescs() const override { return _ContextDataDescs; }

   virtual void PostLoad() override;

#if WITH_EDITOR
   virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR
   
   UPROPERTY(EditAnywhere, Category="Defaults")
   TSubclassOf<AAIController> _ContextControllerClass;
   
   UPROPERTY(EditAnywhere, Category="Defaults")
   TSubclassOf<APawn> _ContextPawnClass;

   UPROPERTY(EditAnywhere, Category="Defaults")
   TSubclassOf<AActor> _SmartObjectActorClass;

   UPROPERTY()
   TArray<FStateTreeExternalDataDesc> _ContextDataDescs;
};
