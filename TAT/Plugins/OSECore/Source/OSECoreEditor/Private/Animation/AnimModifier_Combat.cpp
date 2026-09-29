// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/AnimModifier_Combat.h"

// ose
#include "Animation/AnimMetadata_Combat.h"
#include "Animation/AnimNotifyState_Combat.h"

// ue4
#include "AnimPose.h"
#include "AnimationBlueprintLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimModifier_Combat)

DEFINE_LOG_CATEGORY_STATIC(LogAnimModifier_CombatHitboxes, Log, All);

FCombatHitboxesSettings::FCombatHitboxesSettings()
{
   HitboxTrackNameBase = TEXT("Combat_Hitboxes");
   HitboxBoneName = TEXT("weapon_r");
   HitboxOffsetFromBone = FVector::ZeroVector;
}

UAnimModifier_CombatHitboxes::UAnimModifier_CombatHitboxes()
   : Super()
{
   CombatAnimStartTrackName = TEXT("Combat_AnimStart");
   CombatAnimEndTrackName = TEXT("Combat_AnimEnd");
   CombatAnimAutoAimTrackName = TEXT("Combat_AutoAim");
   RootBoneName = TEXT("root");

   // default to one
   Settings.AddDefaulted(1);
}

void UAnimModifier_CombatHitboxes::OnApply_Implementation(UAnimSequence* animationSequence)
{
   if (!animationSequence)
      return;

   // combat start track add
   UAnimationBlueprintLibrary::AddAnimationNotifyTrack(animationSequence, CombatAnimStartTrackName, FLinearColor::Blue);

   // add combat start notify
   {
      UAnimNotify_CombatAnimationStart* animNotifyStart = CastChecked<UAnimNotify_CombatAnimationStart>(UAnimationBlueprintLibrary::AddAnimationNotifyEvent(animationSequence, CombatAnimStartTrackName, 0.0f, UAnimNotify_CombatAnimationStart::StaticClass()));
   }

   // combat end track add if it doesn't exist
   if (!UAnimationBlueprintLibrary::IsValidAnimNotifyTrackName(animationSequence, CombatAnimEndTrackName))
   {
      UAnimationBlueprintLibrary::AddAnimationNotifyTrack(animationSequence, CombatAnimEndTrackName, FLinearColor::Blue);

      const float endAtTime = animationSequence->GetPlayLength() - 0.2f;
      UAnimNotify_CombatAnimationEnd* animNotifyEnd = CastChecked<UAnimNotify_CombatAnimationEnd>(UAnimationBlueprintLibrary::AddAnimationNotifyEvent(animationSequence, CombatAnimEndTrackName, endAtTime, UAnimNotify_CombatAnimationEnd::StaticClass()));
   }

   // add combat auto aim notify if it doesn't exist
   if (!UAnimationBlueprintLibrary::IsValidAnimNotifyTrackName(animationSequence, CombatAnimAutoAimTrackName))
   {
      UAnimationBlueprintLibrary::AddAnimationNotifyTrack(animationSequence, CombatAnimAutoAimTrackName, FLinearColor::Blue);

      UAnimNotify_CombatAutoAim* autoAim = CastChecked<UAnimNotify_CombatAutoAim>(UAnimationBlueprintLibrary::AddAnimationNotifyEvent(animationSequence, CombatAnimAutoAimTrackName, 0.0f, UAnimNotify_CombatAutoAim::StaticClass()));
   }

   // get or add our hitbox metadata to this sequence
   UAnimMetadata_CombatHitboxes* combatHitboxesMeta = UAnimMetadata_CombatHitboxes::GetAnimMetadata(animationSequence);
   if (!combatHitboxesMeta)
   {
      UAnimMetaData* animMetaBase = nullptr;
      UAnimationBlueprintLibrary::AddMetaData(animationSequence, UAnimMetadata_CombatHitboxes::StaticClass(), animMetaBase);
      combatHitboxesMeta = CastChecked<UAnimMetadata_CombatHitboxes>(animMetaBase);
   }

   // gotta have it!
   check(combatHitboxesMeta);

   // reset hitbox metadata because we're repopulating it
   combatHitboxesMeta->ResetHitboxLocations();
   combatHitboxesMeta->SetCombatAnimationInfo(CombatAnimationInfo);

   // unique tracks even if we re-use the same bone
   TMap<FName, int> usedBones;

   // unique indices between all settings so we can just lookup by int at runtime and not trackname + int
   int hitboxIndex = 0;

   for (const FCombatHitboxesSettings& settings : Settings)
   {
      int& usedBoneNum = usedBones.FindOrAdd(settings.HitboxBoneName);
      FName trackName = *FString::Printf(TEXT("%s: %s_%d"), *settings.HitboxTrackNameBase.ToString(), *settings.HitboxBoneName.ToString(), usedBoneNum);
      usedBoneNum++;

      // hitbox track add
      UAnimationBlueprintLibrary::AddAnimationNotifyTrack(animationSequence, trackName, FLinearColor::Gray);

      // sanity checks upfront
      if (settings.StartTime >= settings.EndTime)
      {
         UE_LOG(LogAnimModifier_CombatHitboxes, Error, TEXT("StartTime must be < EndTime"));
         continue;
      }

      if (settings.NumHitboxes < 2)
      {
         UE_LOG(LogAnimModifier_CombatHitboxes, Error, TEXT("Must have >= 2 hitboxes!"));
         continue;
      }

      // the bone tree path for our target bone
      TArray<FName> bonePath;
      UAnimationBlueprintLibrary::FindBonePathToRoot(animationSequence, settings.HitboxBoneName, bonePath);

      // how long is our timestep?  +1 so we're inclusive of the EndTime when spreading out the samples.
      const float timeStep = (settings.EndTime - settings.StartTime) / float(settings.NumHitboxes - 1);

      for (int idx = 0; idx < settings.NumHitboxes; ++idx)
      {
         // time for this step
         float curTime = settings.StartTime + (timeStep * float(idx));

         // add and setup hitbox metadata
         UAnimNotifyState_CombatHitbox* animNotifyStateHitbox = CastChecked<UAnimNotifyState_CombatHitbox>(UAnimationBlueprintLibrary::AddAnimationNotifyStateEvent(animationSequence, trackName, curTime, settings.Duration, UAnimNotifyState_CombatHitbox::StaticClass()));
         animNotifyStateHitbox->HitboxIndex = hitboxIndex;

         // start calculating our xfm from the offset past our defined bone
         FTransform finalXfm = FTransform(settings.HitboxOffsetFromBone);

         FAnimPose animPose;
         UAnimPoseExtensions::GetAnimPoseAtTime(animationSequence, curTime, FAnimPoseEvaluationOptions(), animPose);

         // would this be equivalent to using the world pose space sans the loop?
         for (FName boneName : bonePath)
         {
            FTransform boneXfm = UAnimPoseExtensions::GetBonePose(animPose, boneName, EAnimPoseSpaces::Local);
            finalXfm *= boneXfm;
         }

         // create the hitbox metadata
         FCombatHitboxMetadata metadata;
         metadata.Location = finalXfm.GetLocation();
         metadata.Radius = settings.HitboxRadius;
         metadata.DebugTraceColor = settings.DebugTraceColor;
         metadata.DebugTraceHitColor = settings.DebugTraceHitColor;

         // add this index + metadata to our anim metadata component
         combatHitboxesMeta->AddHitboxMetadata(animNotifyStateHitbox->HitboxIndex, metadata);

         // next index
         ++hitboxIndex;
      }
   }
}

void UAnimModifier_CombatHitboxes::OnRevert_Implementation(UAnimSequence* animationSequence)
{
   if (!animationSequence)
      return;

   // remove combat playing track
   UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(animationSequence, CombatAnimStartTrackName);

   // intentionally not removing the combat anim end track -- we want to generate this track once and leave it there for manual adjustment 
   //UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(animationSequence, CombatAnimEndTrackName);

   // intentionally not removing the combat auto aim track -- we want to generate this track once and leave it there for manual adjustment 
   //UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(animationSequence, CombatAnimAutoAimTrackName);

   // remove hitbox tracks
   TArray<FName> trackNames;
   UAnimationBlueprintLibrary::GetAnimationNotifyTrackNames(animationSequence, trackNames);
   for(FName trackName : trackNames)
   {
      for (const FCombatHitboxesSettings& settings : Settings)
      {
         if (trackName.ToString().Contains(settings.HitboxTrackNameBase.ToString()))
         {            
            UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(animationSequence, trackName);
         }
      }
   }

   // remove the metadata component
   UAnimationBlueprintLibrary::RemoveMetaDataOfClass(animationSequence, UAnimMetadata_CombatHitboxes::StaticClass());
}
