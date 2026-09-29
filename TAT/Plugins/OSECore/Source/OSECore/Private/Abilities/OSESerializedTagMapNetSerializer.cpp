// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSESerializedTagMapNetSerializer.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESerializedTagMapNetSerializer)

#if UE_WITH_IRIS

// ose
#include "Abilities/OSESerializedTagMap.h"

// tat
#include "GameplayTagsManager.h"
#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Iris/ReplicationState/ReplicationStateDescriptorBuilder.h"
#include "Iris/Serialization/NetSerializerDelegates.h"
#include "Iris/Serialization/NetSerializers.h"

namespace UE::Net
{

// A NetSerializer implementation for FOSESerializedTagMap
//
// It largely delegates to the NetSerializer for the FOSESerializedTagMap_Proxy struct,
// which has an array of pairs.
//
// Similar to what the engine does for FGameplayTagContainer (and some others)
//
// There is also a decent argument for changing the FOSESerializedTagMap struct to be an FFastArray, but this
// already does more than the original NetSerialize did. (But keep an eye on it, might have been simpler)
struct FOSESerializedTagMapNetSerializer
{
   // Version
   static const uint32 Version = 0;

   // Traits
   static constexpr bool bIsForwardingSerializer = true; // Triggers asserts if a function is missing
   static constexpr bool bHasDynamicState = true;

   // Types
   struct FQuantizedType
   {
      alignas(8) uint8 QuantizedStruct[16];
   };

   typedef FOSESerializedTagMap SourceType;
   typedef FQuantizedType QuantizedType;
   typedef FOSESerializedTagMapNetSerializerConfig ConfigType;

   static const ConfigType DefaultConfig;

   //
   static void Serialize(FNetSerializationContext&, const FNetSerializeArgs& args);
   static void Deserialize(FNetSerializationContext&, const FNetDeserializeArgs& args);

   static void SerializeDelta(FNetSerializationContext&, const FNetSerializeDeltaArgs& args);
   static void DeserializeDelta(FNetSerializationContext&, const FNetDeserializeDeltaArgs& args);

   static void Quantize(FNetSerializationContext&, const FNetQuantizeArgs& args);
   static void Dequantize(FNetSerializationContext&, const FNetDequantizeArgs& args);

   static bool IsEqual(FNetSerializationContext&, const FNetIsEqualArgs& args);
   static bool Validate(FNetSerializationContext&, const FNetValidateArgs& args);

   static void CloneDynamicState(FNetSerializationContext&, const FNetCloneDynamicStateArgs&);
   static void FreeDynamicState(FNetSerializationContext&, const FNetFreeDynamicStateArgs&);

   static void CollectNetReferences(FNetSerializationContext&, const FNetCollectReferencesArgs&);

private:
   class FNetSerializerRegistryDelegates final : private UE::Net::FNetSerializerRegistryDelegates
   {
   public:
      virtual ~FNetSerializerRegistryDelegates() override;

   private:
      virtual void OnPreFreezeNetSerializerRegistry() override;
      virtual void OnPostFreezeNetSerializerRegistry() override;
   };

   static void MapToProxy(const SourceType& map, FOSESerializedTagMap_Proxy& out);

   static FOSESerializedTagMapNetSerializer::FNetSerializerRegistryDelegates NetSerializerRegistryDelegates;
   static FStructNetSerializerConfig StructNetSerializerConfig;
   static const FNetSerializer* StructNetSerializer;
};
UE_NET_IMPLEMENT_SERIALIZER(FOSESerializedTagMapNetSerializer);

const FOSESerializedTagMapNetSerializer::ConfigType FOSESerializedTagMapNetSerializer::DefaultConfig;

FOSESerializedTagMapNetSerializer::FNetSerializerRegistryDelegates FOSESerializedTagMapNetSerializer::NetSerializerRegistryDelegates;
FStructNetSerializerConfig FOSESerializedTagMapNetSerializer::StructNetSerializerConfig;
const FNetSerializer* FOSESerializedTagMapNetSerializer::StructNetSerializer = &UE_NET_GET_SERIALIZER(FStructNetSerializer);

void FOSESerializedTagMapNetSerializer::Serialize(FNetSerializationContext& context, const FNetSerializeArgs& args)
{
   FNetSerializeArgs internalArgs = args;
   internalArgs.NetSerializerConfig = &StructNetSerializerConfig;
   return StructNetSerializer->Serialize(context, internalArgs);
}

void FOSESerializedTagMapNetSerializer::Deserialize(FNetSerializationContext& context, const FNetDeserializeArgs& args)
{
   FNetDeserializeArgs internalArgs = args;
   internalArgs.NetSerializerConfig = &StructNetSerializerConfig;
   return StructNetSerializer->Deserialize(context, internalArgs);
}

void FOSESerializedTagMapNetSerializer::SerializeDelta(FNetSerializationContext& context, const FNetSerializeDeltaArgs& args)
{
   FNetSerializeDeltaArgs internalArgs = args;
   internalArgs.NetSerializerConfig = &StructNetSerializerConfig;
   return StructNetSerializer->SerializeDelta(context, internalArgs);
}

void FOSESerializedTagMapNetSerializer::DeserializeDelta(FNetSerializationContext& context, const FNetDeserializeDeltaArgs& args)
{
   FNetDeserializeDeltaArgs internalArgs = args;
   internalArgs.NetSerializerConfig = &StructNetSerializerConfig;
   return StructNetSerializer->DeserializeDelta(context, internalArgs);
}

void FOSESerializedTagMapNetSerializer::Quantize(FNetSerializationContext& context, const FNetQuantizeArgs& args)
{
   check(UGameplayTagsManager::Get().ShouldUseFastReplication());

   FOSESerializedTagMap_Proxy intermediateValue;

   // It's preferred but not vital that the default state represents the true default state. Because of the brittle tag loading/adding we choose to not store tags in the default state.
   if (!context.IsInitializingDefaultState())
   {
      const SourceType& sourceValue = *reinterpret_cast<const SourceType*>(args.Source);
      MapToProxy(sourceValue, intermediateValue);
   }

   FNetQuantizeArgs internalArgs = args;
   internalArgs.Source = NetSerializerValuePointer(&intermediateValue);
   internalArgs.NetSerializerConfig = &StructNetSerializerConfig;
   return StructNetSerializer->Quantize(context, internalArgs);
}

void FOSESerializedTagMapNetSerializer::Dequantize(FNetSerializationContext& context, const FNetDequantizeArgs& args)
{
   FOSESerializedTagMap_Proxy intermediateValue;

   FNetDequantizeArgs internalArgs = args;
   internalArgs.Target = NetSerializerValuePointer(&intermediateValue);
   internalArgs.NetSerializerConfig = &StructNetSerializerConfig;

   StructNetSerializer->Dequantize(context, internalArgs);

   FOSESerializedTagMap& targetValue = *reinterpret_cast<FOSESerializedTagMap*>(args.Target);
   targetValue.Values.Reset();
   for (const FOSESerializedTagMap_ProxyEntry& entry : intermediateValue.Entries)
   {
      targetValue.Values.Add(entry.Tag, entry.Count);
   }
}

bool FOSESerializedTagMapNetSerializer::IsEqual(FNetSerializationContext& context, const FNetIsEqualArgs& args)
{
   if (args.bStateIsQuantized)
   {
      check(UGameplayTagsManager::Get().ShouldUseFastReplication());
      FNetIsEqualArgs internalArgs = args;
      internalArgs.NetSerializerConfig = &StructNetSerializerConfig;
      return StructNetSerializer->IsEqual(context, internalArgs);
   }
   else
   {
      const SourceType& sourceValue0 = *reinterpret_cast<const SourceType*>(args.Source0);
      const SourceType& sourceValue1 = *reinterpret_cast<const SourceType*>(args.Source1);

      return sourceValue0.Values.OrderIndependentCompareEqual(sourceValue1.Values);
   }
}

bool FOSESerializedTagMapNetSerializer::Validate(FNetSerializationContext& context, const FNetValidateArgs& args)
{
   const SourceType& sourceValue = *reinterpret_cast<const SourceType*>(args.Source);

   FOSESerializedTagMap_Proxy intermediateValue;
   MapToProxy(sourceValue, intermediateValue);

   FNetValidateArgs InternalArgs = args;
   InternalArgs.Source = NetSerializerValuePointer(&intermediateValue);
   InternalArgs.NetSerializerConfig = &StructNetSerializerConfig;

   return StructNetSerializer->Validate(context, InternalArgs);
}

void FOSESerializedTagMapNetSerializer::CollectNetReferences(FNetSerializationContext& context, const FNetCollectReferencesArgs& args)
{
   // There are no references.
}

void FOSESerializedTagMapNetSerializer::CloneDynamicState(FNetSerializationContext& context, const FNetCloneDynamicStateArgs& args)
{
   FNetCloneDynamicStateArgs internalArgs = args;
   internalArgs.NetSerializerConfig = &StructNetSerializerConfig;
   return StructNetSerializer->CloneDynamicState(context, internalArgs);
}

void FOSESerializedTagMapNetSerializer::FreeDynamicState(FNetSerializationContext& context, const FNetFreeDynamicStateArgs& args)
{
   FNetFreeDynamicStateArgs internalArgs = args;
   internalArgs.NetSerializerConfig = &StructNetSerializerConfig;
   return StructNetSerializer->FreeDynamicState(context, internalArgs);
}

void FOSESerializedTagMapNetSerializer::MapToProxy(const SourceType& map, FOSESerializedTagMap_Proxy& out)
{
   const UGameplayTagsManager& tagManager = UGameplayTagsManager::Get();
   const int32 tagCount = map.Values.Num();
   if (tagCount <= 0)
   {
      return;
   }

   struct FPair
   {
      FGameplayTag Tag;
      int32 ArrayIndex = 0;
      FGameplayTagNetIndex TagIndex;
   };

   // Needs sorted array in order to determine equality between quantized states. Can also be used to implement more bandwidth efficient serialization.
   TArray<FPair> sortedArray;
   sortedArray.AddZeroed(tagCount);
   {
      FPair* indexPairs = sortedArray.GetData();
      int32 indexPairCount = 0;
      for (const TPair<FGameplayTag, int32>& pair : map.Values)
      {
         indexPairs[indexPairCount].Tag = pair.Key;
         indexPairs[indexPairCount].ArrayIndex = indexPairCount;
         indexPairs[indexPairCount].TagIndex = tagManager.GetNetIndexFromTag(pair.Key);
         ++indexPairCount;
      }

      auto sortByTagIndex = [](const FPair& value0, const FPair& value1)->bool
      {
         if (value0.TagIndex != value1.TagIndex)
         {
            return value0.TagIndex < value1.TagIndex;
         }
         else
         {
            return value0.ArrayIndex < value1.ArrayIndex;
         }
      };
      Algo::Sort(sortedArray, sortByTagIndex);
   }

   // Copy sorted tags to helper struct instance.
   {
      out.Entries.Reserve(tagCount);
      for (const FPair& pair : sortedArray)
      {
         out.Entries.Emplace(pair.Tag, map.Values.FindChecked(pair.Tag));
      }
   }
}

static const FName PropertyNetSerializerRegistry_NAME_OSESerializedTagMap("OSESerializedTagMap");
UE_NET_IMPLEMENT_NAMED_STRUCT_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_OSESerializedTagMap, FOSESerializedTagMapNetSerializer);

FOSESerializedTagMapNetSerializer::FNetSerializerRegistryDelegates::~FNetSerializerRegistryDelegates()
{
   UE_NET_UNREGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_OSESerializedTagMap);
}

void FOSESerializedTagMapNetSerializer::FNetSerializerRegistryDelegates::OnPreFreezeNetSerializerRegistry()
{
   UE_NET_REGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_OSESerializedTagMap);
}

void FOSESerializedTagMapNetSerializer::FNetSerializerRegistryDelegates::OnPostFreezeNetSerializerRegistry()
{
   UStruct* structInfo = FOSESerializedTagMap_Proxy::StaticStruct();
   StructNetSerializerConfig.StateDescriptor = FReplicationStateDescriptorBuilder::CreateDescriptorForStruct(structInfo);
   const FReplicationStateDescriptor* descriptor = StructNetSerializerConfig.StateDescriptor.GetReference();

   check(descriptor != nullptr);
   // We have an empty CollectNetReferences implementation so make sure everything is as expected.
   check(!EnumHasAnyFlags(descriptor->Traits, EReplicationStateTraits::HasObjectReference));

   // Validate our assumptions regarding quantized state size and alignment.
   static_assert(offsetof(FQuantizedType, QuantizedStruct) == 0U, "Expected buffer for struct to be first member of FQuantizedType.");
   if (sizeof(FQuantizedType::QuantizedStruct) < descriptor->InternalSize || alignof(FQuantizedType) < descriptor->InternalAlignment)
   {
      LowLevelFatalError(TEXT("FQuantizedType::QuantizedStruct has size %u and alignment %u but requires size %u and alignment %u."), uint32(sizeof(FQuantizedType::QuantizedStruct)), uint32(alignof(FQuantizedType)), uint32(descriptor->InternalSize), uint32(descriptor->InternalAlignment)); 
   }
}

}

#endif // UE_WITH_IRIS
