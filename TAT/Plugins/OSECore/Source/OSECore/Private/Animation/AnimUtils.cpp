// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/AnimUtils.h"

// OSE
#include "Animation/OSEAnimGraphLink.h"

// UE5
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Curves/CurveVector.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimUtils)

DEFINE_LOG_CATEGORY_STATIC(LogOSEAnimUtils, Log, All);

namespace AnimUtilsImpl
{
   /// Optimized version of UAnimSequence::ExtractRootMotionFromRange
   static FTransform GetRootDelta(
      const FTransform& InToComponent,
      const FTransform& InPrevious,
      const FTransform& InCurrent)
   {
      const FTransform StartTransform = InToComponent * InPrevious;
      const FTransform EndTransform = InToComponent * InCurrent;

      return EndTransform.GetRelativeTransform(StartTransform);
   }

   static FTransform GetRootTransform(const UAnimSequence& InAnimSequence, const float Time)
   {
      const bool UseRawDataOnly = true;

      FTransform RootXfm;
      InAnimSequence.GetBoneTransform(RootXfm, FSkeletonPoseBoneIndex(0), static_cast<double>(Time), UseRawDataOnly);
      return RootXfm;
   }

   static void ResetCurve(FRealCurve& OutRealCurve, const float DefaultValue)
   {
      OutRealCurve.Reset();
      OutRealCurve.SetDefaultValue(DefaultValue);
   }

   static void ResetCurveIdx(UCurveVector& OutVectorCurve, const FVector& DefaultValue, const int32 Index)
   {
      ResetCurve(OutVectorCurve.FloatCurves[Index], DefaultValue[Index]);
   }

   static void ResetCurve(UCurveVector& OutVectorCurve, const FVector& DefaultValue)
   {
      ResetCurveIdx(OutVectorCurve, DefaultValue, 0);
      ResetCurveIdx(OutVectorCurve, DefaultValue, 1);
      ResetCurveIdx(OutVectorCurve, DefaultValue, 2);
   }

   static void AddRootCurvePositionIdx(const FVector& InRootDelta, UCurveVector& OutVectorCurve, const float Time, const int32 Index)
   {
      FRealCurve* Curve = &(OutVectorCurve.FloatCurves[Index]);
      FKeyHandle Handle = Curve->AddKey(Time, InRootDelta[Index]);
      Curve->SetKeyInterpMode(Handle, RCIM_Linear);
   }

   static void AddRootCurvePosition(const FVector& InRootDelta, UCurveVector& OutVectorCurve, const float Time)
   {
      AddRootCurvePositionIdx(InRootDelta, OutVectorCurve, Time, 0);
      AddRootCurvePositionIdx(InRootDelta, OutVectorCurve, Time, 1);
      AddRootCurvePositionIdx(InRootDelta, OutVectorCurve, Time, 2);
   }
}


// Extracts root motion position curves from the specified animation sequence.
// Existing curve object must exist, and its data is reset.
// Returns true on success.
bool UAnimUtils::RootMotionCurveExtract(UCurveVector* outRootCurve, const UAnimSequence* inAnimSequence)
{
   if (outRootCurve == nullptr)
      return false;

   if ((inAnimSequence == nullptr) || (!inAnimSequence->HasRootMotion()))
      return false;

   // Transform to Component Space Rotation (inverse root transform from first frame)
   const FTransform transformInit = AnimUtilsImpl::GetRootTransform(*inAnimSequence, 0.0f);
   const FTransform rootToComponent = FTransform(transformInit.GetRotation().Inverse());

   // Reset all the curves, and add a key for time=0
   AnimUtilsImpl::ResetCurve(*outRootCurve, FVector::ZeroVector);
   FTransform rootDeltaXfm = AnimUtilsImpl::GetRootDelta(rootToComponent, transformInit, transformInit);
   AnimUtilsImpl::AddRootCurvePosition(rootDeltaXfm.GetTranslation(), *outRootCurve, 0.0f);

   // Add a position key for every other frame
   const int32 numFrames = inAnimSequence->GetNumberOfSampledKeys();
   const float frameMult = inAnimSequence->GetPlayLength() / (float)(numFrames - 1);
   for (int32 frameIdx = 1; frameIdx < numFrames; ++frameIdx)
   {
      // Update the bone transforms
      const float frameTime = FMath::Clamp(frameMult * frameIdx, 0.0f, inAnimSequence->GetPlayLength());
      const FTransform transformCurr = AnimUtilsImpl::GetRootTransform(*inAnimSequence, frameTime);

      // Get the delta and add a key for this time
      rootDeltaXfm = AnimUtilsImpl::GetRootDelta(rootToComponent, transformInit, transformCurr);
      AnimUtilsImpl::AddRootCurvePosition(rootDeltaXfm.GetTranslation(), *outRootCurve, frameTime);
   }

   return true;
}

// Creates and extracts root motion position curves from the specified animation sequence.
// The curve is created and returned on success, otherwise null is returned.
UCurveVector* UAnimUtils::RootMotionCurveCreate(const UAnimSequence* inAnimSequence)
{
   if ((inAnimSequence == nullptr) || (!inAnimSequence->HasRootMotion()))
      return nullptr;

   UCurveVector* RootCurve = NewObject<UCurveVector>();
   if (!RootMotionCurveExtract(RootCurve, inAnimSequence))
      return nullptr;

   return RootCurve;
}

// Processes the anim graph links specified in the data asset, replacing them as needed.
bool UAnimUtils::LinkAnimGraphAssets(UAnimInstance* animInstance, const UOSEAnimGraphLinkAsset* animLinkAsset)
{
   bool result = false;

   if ((animInstance != nullptr) && (animLinkAsset != nullptr))
   {
      for (const auto& item : animLinkAsset->Links)
      {
         if (!item.Tag.IsNone())
         {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

            if (animInstance->GetLinkedAnimGraphInstanceByTag(item.Tag) == nullptr)
            {
               UE_LOG(LogOSEAnimUtils, Warning, TEXT("[%s] Anim graph instance with tag `%s` not found")
                  , *(animInstance->GetName())
                  , *(item.Tag.ToString())
               );
            }
#endif
            animInstance->LinkAnimGraphByTag(item.Tag, item.InstanceClass);
            result = true;
         }
      }
   }

   return result;
}

