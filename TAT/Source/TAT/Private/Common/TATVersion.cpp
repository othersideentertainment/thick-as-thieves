// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Common/TATVersion.h"
#include "../../Resources/Version.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATVersion)

FString UTATVersionV1::ToString(EVersionComponent lastComponent)
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

FString UTATVersionV1::GetBuildVersionString() { return FString(TAT_BUILD_VERSION_STRING); }

FString UTATVersionV1::GetBuildDate() { return FString(TAT_BUILD_DATE); }

int32 UTATVersionV1::GetBuildVersionMajor() { return TAT_BUILD_VERSION_MAJOR;  }

int32 UTATVersionV1::GetBuildVersionMinor() { return TAT_BUILD_VERSION_MINOR; }

int32 UTATVersionV1::GetBuildNumber() { return TAT_BUILD_NUMBER; }

int32 UTATVersionV1::GetBuildChangelistNumber() { return TAT_BUILD_VCS_NUMBER; }

FString UTATVersionV1::GetBuildBranch() { return FString{VERSION_TEXT(TAT_BUILD_VCS_BRANCH)}; }

FString UTATVersionV1::GetBuildBranchDescriptor() { return FString{VERSION_TEXT(TAT_BUILD_VCS_BRANCH)}.Replace(TEXT("/"), TEXT("+")); }

const TArray<FString>& UTATVersionV1::GetBuildArtifactDescriptors() { static TArray<FString> descriptors{}; return descriptors; }

const FString& UTATVersionV1::GetBuildArtifactDescriptor() { static FString versionString{TAT_BUILD_VERSION_STRING}; return versionString; }

int UTATVersionV1::GetEdition() { return 1; }

