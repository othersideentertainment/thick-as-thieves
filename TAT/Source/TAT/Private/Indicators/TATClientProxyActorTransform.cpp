// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Indicators/TATClientProxyActorTransform.h"

// UE
#include <Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h>
#include <Iris/Serialization/NetSerializer.h>
#include <Iris/Serialization/PackedVectorNetSerializers.h>
#include <Iris/Serialization/RotatorNetSerializers.h>
#include <Net/Core/Trace/NetTrace.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClientProxyActorTransform)

void FTATClientProxyActorTransform::Set(const FTransform& transform, bool allowRotation, bool allowScale)
{
   Location = transform.GetLocation();
   AllowRotation = allowRotation;
   Rotation = AllowRotation ? transform.GetRotation().Rotator() : FRotator::ZeroRotator;
   AllowScale = allowScale;
   Scale = AllowScale ? transform.GetScale3D() : FVector::OneVector;
}

bool FTATClientProxyActorTransform::NetSerialize(FArchive& ar, UPackageMap* packageMap, bool& outSuccess)
{
   outSuccess = true;
   ar << Location;

   // Only serialize rotation if requested
   uint8 allowRotationCopy = AllowScale;
   ar.SerializeBits(&allowRotationCopy, 1);
   AllowRotation = !!allowRotationCopy;
   if (AllowRotation)
   {
      Rotation.SerializeCompressed(ar);
   }
   else if (ar.IsLoading())
   {
      Rotation = FRotator::ZeroRotator;
   }

   // Only serialize scale if requested
   uint8 allowScaleCopy = AllowScale;
   ar.SerializeBits(&allowScaleCopy, 1);
   AllowScale = !!allowScaleCopy;
   if (AllowScale)
   {
      bool scaleSuccess = false;
      Scale.NetSerialize(ar, packageMap, outSuccess);
      outSuccess &= scaleSuccess;
   }
   else if (ar.IsLoading())
   {
      Scale = FVector::OneVector;
   }

   return true;
}

namespace UE::Net
{
   UE_NET_DECLARE_SERIALIZER(FTATClientProxyActorTransformNetSerializer,);

   struct FTATClientProxyActorTransformNetSerializer
   {
      static constexpr uint32 Version = 0;

      typedef FNetSerializerConfig ConfigType;
      inline static const ConfigType DefaultConfig;

      enum EReplicationFlags : uint8
      {
         HasRotation = 1,
         HasScale = HasRotation << 1,
      };

      static constexpr uint32 ReplicationFlagsBits = 2;

      typedef FTATClientProxyActorTransform SourceType;

      typedef struct
      {
         uint8 ReplicationFlags;
         uint64 Location[4];
         uint16 Rotation[4];
         uint64 Scale[4];
      } QuantizedType;

      static void Serialize(FNetSerializationContext& context, const FNetSerializeArgs& args)
      {
         const QuantizedType& source = *reinterpret_cast<const QuantizedType*>(args.Source);

         FNetBitStreamWriter* writer = context.GetBitStreamWriter();

         writer->WriteBits(source.ReplicationFlags, ReplicationFlagsBits);

         {
            UE_NET_TRACE_SCOPE(Location, *writer, context.GetTraceCollector(), ENetTraceVerbosity::Verbose);
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FVectorNetQuantizeNetSerializer);

            FNetSerializeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Source = NetSerializerValuePointer(&source.Location[0]);
            memberSerializer.Serialize(context, memberArgs);
         }

         if (source.ReplicationFlags & EReplicationFlags::HasRotation)
         {
            UE_NET_TRACE_SCOPE(Rotation, *writer, context.GetTraceCollector(), ENetTraceVerbosity::Verbose);
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FRotatorAsShortNetSerializer);

            FNetSerializeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Source = NetSerializerValuePointer(&source.Rotation[0]);
            memberSerializer.Serialize(context, memberArgs);
         }

         if (source.ReplicationFlags & EReplicationFlags::HasScale)
         {
            UE_NET_TRACE_SCOPE(Scale, *writer, context.GetTraceCollector(), ENetTraceVerbosity::Verbose);
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FVectorNetQuantizeNetSerializer);

            FNetSerializeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Source = NetSerializerValuePointer(&source.Scale[0]);
            memberSerializer.Serialize(context, memberArgs);
         }
      }

      static void Deserialize(FNetSerializationContext& context, const FNetDeserializeArgs& args)
      {
         QuantizedType& target = *reinterpret_cast<QuantizedType*>(args.Target);

         FNetBitStreamReader* reader = context.GetBitStreamReader();

         target.ReplicationFlags = reader->ReadBits(ReplicationFlagsBits);

         {
            UE_NET_TRACE_SCOPE(Location, *reader, context.GetTraceCollector(), ENetTraceVerbosity::Verbose);
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FVectorNetQuantizeNetSerializer);

            FNetDeserializeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Target = NetSerializerValuePointer(&target.Location[0]);
            memberSerializer.Deserialize(context, memberArgs);
         }

         if (target.ReplicationFlags & EReplicationFlags::HasRotation)
         {
            UE_NET_TRACE_SCOPE(Rotation, *reader, context.GetTraceCollector(), ENetTraceVerbosity::Verbose);
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FRotatorAsShortNetSerializer);

            FNetDeserializeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Target = NetSerializerValuePointer(&target.Rotation[0]);
            memberSerializer.Deserialize(context, memberArgs);
         }

         if (target.ReplicationFlags & EReplicationFlags::HasScale)
         {
            UE_NET_TRACE_SCOPE(Scale, *reader, context.GetTraceCollector(), ENetTraceVerbosity::Verbose);
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FVectorNetQuantizeNetSerializer);

            FNetDeserializeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Target = NetSerializerValuePointer(&target.Scale[0]);
            memberSerializer.Deserialize(context, memberArgs);
         }
      }

      static void Quantize(FNetSerializationContext& context, const FNetQuantizeArgs& args)
      {
         const SourceType& source = *reinterpret_cast<const SourceType*>(args.Source);
         QuantizedType& target = *reinterpret_cast<QuantizedType*>(args.Target);

         target.ReplicationFlags = 0;
         target.ReplicationFlags |= source.AllowRotation ? EReplicationFlags::HasRotation : 0;
         target.ReplicationFlags |= source.AllowScale ? EReplicationFlags::HasScale : 0;

         {
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FVectorNetQuantizeNetSerializer);

            FNetQuantizeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Source = NetSerializerValuePointer(&source.Location);
            memberArgs.Target = NetSerializerValuePointer(&target.Location[0]);
            memberSerializer.Quantize(context, memberArgs);
         }

         if (target.ReplicationFlags & EReplicationFlags::HasRotation)
         {
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FRotatorAsShortNetSerializer);

            FNetQuantizeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Source = NetSerializerValuePointer(&source.Rotation);
            memberArgs.Target = NetSerializerValuePointer(&target.Rotation[0]);
            memberSerializer.Quantize(context, memberArgs);
         }

         if (target.ReplicationFlags & EReplicationFlags::HasScale)
         {
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FVectorNetQuantizeNetSerializer);

            FNetQuantizeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Source = NetSerializerValuePointer(&source.Scale);
            memberArgs.Target = NetSerializerValuePointer(&target.Scale[0]);
            memberSerializer.Quantize(context, memberArgs);
         }
      }

      static void Dequantize(FNetSerializationContext& context, const FNetDequantizeArgs& args)
      {
         const QuantizedType& source = *reinterpret_cast<const QuantizedType*>(args.Source);
         SourceType& target = *reinterpret_cast<SourceType*>(args.Target);

         target.AllowRotation = source.ReplicationFlags & EReplicationFlags::HasRotation;
         target.AllowScale = source.ReplicationFlags & EReplicationFlags::HasScale;

         {
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FVectorNetQuantizeNetSerializer);

            FNetDequantizeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Source = NetSerializerValuePointer(&source.Location[0]);
            memberArgs.Target = NetSerializerValuePointer(&target.Location);
            memberSerializer.Dequantize(context, memberArgs);
         }

         if (source.ReplicationFlags & EReplicationFlags::HasRotation)
         {
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FRotatorAsShortNetSerializer);

            FNetDequantizeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Source = NetSerializerValuePointer(&source.Rotation[0]);
            memberArgs.Target = NetSerializerValuePointer(&target.Rotation);
            memberSerializer.Dequantize(context, memberArgs);
         }
         else
         {
            target.Rotation = FRotator::ZeroRotator;
         }

         if (source.ReplicationFlags & EReplicationFlags::HasScale)
         {
            const FNetSerializer& memberSerializer = UE_NET_GET_SERIALIZER(FVectorNetQuantizeNetSerializer);

            FNetDequantizeArgs memberArgs = args;
            memberArgs.NetSerializerConfig = NetSerializerConfigParam(memberSerializer.DefaultConfig);
            memberArgs.Source = NetSerializerValuePointer(&source.Scale[0]);
            memberArgs.Target = NetSerializerValuePointer(&target.Scale);
            memberSerializer.Dequantize(context, memberArgs);
         }
         else
         {
            target.Scale = FVector::OneVector;
         }
      }

      static bool IsEqual(FNetSerializationContext& context, const FNetIsEqualArgs& args)
      {
         QuantizedType quantizedValue0 = {};
         QuantizedType quantizedValue1 = {};

         if (args.bStateIsQuantized)
         {
            quantizedValue0 = *reinterpret_cast<const QuantizedType*>(args.Source0);
            quantizedValue1 = *reinterpret_cast<const QuantizedType*>(args.Source1);
         }
         else
         {
            FNetQuantizeArgs quantizeArgs = {};
            quantizeArgs.NetSerializerConfig = args.NetSerializerConfig;

            quantizeArgs.Source = NetSerializerValuePointer(args.Source0);
            quantizeArgs.Target = NetSerializerValuePointer(&quantizedValue0);
            Quantize(context, quantizeArgs);

            quantizeArgs.Source = NetSerializerValuePointer(args.Source1);
            quantizeArgs.Target = NetSerializerValuePointer(&quantizedValue1);
            Quantize(context, quantizeArgs);
         }

         return FPlatformMemory::Memcmp(&quantizedValue0, &quantizedValue1, sizeof(QuantizedType)) == 0;
      }
   };

   UE_NET_IMPLEMENT_SERIALIZER(FTATClientProxyActorTransformNetSerializer);
   UE_NET_IMPLEMENT_FORWARDING_NETSERIALIZER_AND_REGISTRY_DELEGATES(TATClientProxyActorTransform, FTATClientProxyActorTransformNetSerializer);
}
