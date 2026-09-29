// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATVersionV2.h"
#include "Version.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATVersionV2)

TArray<FString> UTATVersionV2::_buildArtifactDescriptorsCache;
FString UTATVersionV2::_buildArtifactDescriptor;

FString UTATVersionV2::ToString(EVersionComponent lastComponent)
{
   FString result = FString::Printf(TEXT("%d"), GetBuildVersionMajor());
   if(lastComponent >= EVersionComponent::Minor)
   {
      result.Appendf(TEXT(".%d"), GetBuildVersionMinor());
      if(lastComponent >= EVersionComponent::Patch)
      {
         result.Appendf(TEXT(".%d"), GetBuildNumber());
         if(lastComponent >= EVersionComponent::Changelist)
         {
            result.Appendf(TEXT("-%d"), GetBuildChangelistNumber());
            FString branch = GetBuildBranchDescriptor();
            if(lastComponent >= EVersionComponent::Branch && branch.Len() > 0)
            {
               result.Appendf(TEXT("+%s"), *branch);
            }
         }
      }
   }
   return result;
}

FString UTATVersionV2::GetBuildVersionString() { return FString(TAT_BUILD_VERSION_STRING); }

FString UTATVersionV2::GetBuildDate() { return FString(TAT_BUILD_DATE); }

int32 UTATVersionV2::GetBuildVersionMajor() { return TAT_BUILD_VERSION_MAJOR;  }

int32 UTATVersionV2::GetBuildVersionMinor() { return TAT_BUILD_VERSION_MINOR; }

int32 UTATVersionV2::GetBuildNumber() { return TAT_BUILD_NUMBER; }

int32 UTATVersionV2::GetBuildChangelistNumber() { return TAT_BUILD_VCS_NUMBER; }

FString UTATVersionV2::GetBuildBranch() { return FString{VERSION_TEXT(TAT_BUILD_VCS_BRANCH)}; }

FString UTATVersionV2::GetBuildBranchDescriptor() { return FString{VERSION_TEXT(TAT_BUILD_VCS_BRANCH)}.Replace(TEXT("/"), TEXT("+")); }

const TArray<FString>& UTATVersionV2::GetBuildArtifactDescriptors()
{
   return _buildArtifactDescriptorsCache;
}

const FString& UTATVersionV2::GetBuildArtifactDescriptor()
{
   return _buildArtifactDescriptor;
}

int UTATVersionV2::GetEdition() { return 2; }

void UTATVersionV2::CacheBuildArtifactDescriptors()
{
   _buildArtifactDescriptorsCache = TArray<FString>{
      FString(VERSION_TEXT(TAT_BUILD_PRODUCT_NAME)).ToLower(),
      VERSION_STRINGIFY(TAT_BUILD_VERSION_MAJOR),
      VERSION_STRINGIFY(TAT_BUILD_VERSION_MINOR),
      VERSION_STRINGIFY(TAT_BUILD_VCS_NUMBER),
      FString::FromInt( FNetworkVersion::GetNetworkCompatibleChangelist()),
      FString(VERSION_TEXT(TAT_BUILD_SHORT_NAME)).ToLower(),
      VERSION_TEXT(TAT_BUILD_DISCRIMINATOR),
      LexToString(FApp::GetBuildConfiguration()),
      FGenericPlatformMisc::GetEngineMode(),
      ANSI_TO_TCHAR(FPlatformProperties::PlatformName())
   };
}

void UTATVersionV2::CacheBuildArtifactDescriptor()
{
   if ( FNetworkVersion::GetNetworkCompatibleChangelist() == TAT_BUILD_VCS_NUMBER)
   {
      _buildArtifactDescriptor = FString::Printf(TEXT("%s-%s.%s.%s-%s-%s-%s-%s-%s"),
         *_buildArtifactDescriptorsCache[0],
         *_buildArtifactDescriptorsCache[1],
         *_buildArtifactDescriptorsCache[2],
         *_buildArtifactDescriptorsCache[3],
         *_buildArtifactDescriptorsCache[5],
         *_buildArtifactDescriptorsCache[6],
         *_buildArtifactDescriptorsCache[7],
         *_buildArtifactDescriptorsCache[8],
         *_buildArtifactDescriptorsCache[9]
         );
   }
   else
   {
      _buildArtifactDescriptor = FString::Printf(TEXT("%s-%s.%s.%s-compat.%s-%s-%s-%s-%s-%s"),
        *_buildArtifactDescriptorsCache[0],
        *_buildArtifactDescriptorsCache[1],
        *_buildArtifactDescriptorsCache[2],
        *_buildArtifactDescriptorsCache[3],
        *_buildArtifactDescriptorsCache[4],
        *_buildArtifactDescriptorsCache[5],
        *_buildArtifactDescriptorsCache[6],
        *_buildArtifactDescriptorsCache[7],
        *_buildArtifactDescriptorsCache[8],
        *_buildArtifactDescriptorsCache[9]
        );     
   }
}
