// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

//ose editor
#include "VoiceOver/OSEVoiceOverAssetFactories.h"

//ose
#include "VoiceOver/OSEVoiceOverBucket.h"
#include "VoiceOver/OSEVoiceOverLine.h"
#include "VoiceOver/OSEVoiceOverConversation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverAssetFactories)

UOSEVoiceOverLineFactory::UOSEVoiceOverLineFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UOSEVoiceOverLine::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UOSEVoiceOverLineFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   UOSEVoiceOverLine* voiceOverLine = NewObject<UOSEVoiceOverLine>(inParent, name, flags);

   return voiceOverLine;
}

UOSEVoiceOverConversationFactory::UOSEVoiceOverConversationFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UOSEVoiceOverConversation::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UOSEVoiceOverConversationFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   UOSEVoiceOverConversation* voiceOverConversation = NewObject<UOSEVoiceOverConversation>(inParent, name, flags);

   return voiceOverConversation;
}

UOSEVoiceOverBucketFactory::UOSEVoiceOverBucketFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UOSEVoiceOverBucket::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UOSEVoiceOverBucketFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   UOSEVoiceOverBucket* voiceOverBucket = NewObject<UOSEVoiceOverBucket>(inParent, name, flags);

   return voiceOverBucket;
}
