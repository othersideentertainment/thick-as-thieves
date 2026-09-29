// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "TATAtomicSave.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

#define LOCTEXT_NAMESPACE "FTATAtomicSaveModule"

void FTATAtomicSaveModule::StartupModule()
{
   // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FTATAtomicSaveModule::ShutdownModule()
{
   // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
   // we call this function before unloading the module.
}

#if PLATFORM_WINDOWS
// Partially adapted from FWindowsPlatformFile::NormalizeWindowsPath
// It works without normalizing locally, but may not support paths longer than MAX_PATH.
static void NormalizeWindowsPath(FStringBuilderBase& path)
{
   FPathViews::ToAbsolutePathInline(path);

   // Remove duplicate slashes
   const bool isUNCPath = path.ToView().StartsWith(TEXTVIEW("//"));

   FPathViews::RemoveDuplicateSlashes(path);

   if (isUNCPath)
   {
      // Keep // at the beginning.  If There are more than two / at the beginning, replace them with just //.
      path.Prepend(TEXTVIEW("/"));
   }

   // We now have a canonical, strict-valid, absolute Unreal Path.  Convert it to a Windows Path.
   for (TCHAR& aChar : TArrayView<TCHAR>(path.GetData(), path.Len()))
   {
      if (aChar == TEXT('/'))
      {
         aChar = TEXT('\\');
      }
   }

   // Handle Windows paths with null-terminated length over MAX_PATH
   if (path.Len() >= MAX_PATH)
   {
      if (isUNCPath)
      {
         path.ReplaceAt(0, 1, TEXTVIEW("\\\\?\\UNC"));
      }
      else
      {
         path.Prepend(TEXTVIEW("\\\\?\\"));
      }
   }
}
#endif

class FTATAtomicSaveGameSystem : public FGenericSaveGameSystem
{

public:
   virtual bool SaveGame(bool attemptToUseUI, const TCHAR* name, const int32 userIndex, const TArray<uint8>& data) override
   {
#if PLATFORM_WINDOWS
      const FString basePath = GetSaveGamePath(name);
      TStringBuilder<256> tempPath;
      tempPath.Append(basePath);
      tempPath.Append(TEXT(".temp"));
      
      if (!FFileHelper::SaveArrayToFile(data, *tempPath))
      {
         return false;
      }

      auto appendNormalized = [](const auto& input, FStringBuilderBase& builder) {
         builder.Append(input);
         NormalizeWindowsPath(builder);
      };

      // IFileManager::Get().Move will try to separately delete the target file if it exists,
      // which is distinctly non-atomic. And if you pass replace=false, which will call `MoveFileW`
      // on Windows, which will not replace an existing file.
      //
      // So this has to call MoveFileExW directly with MOVEFILE_REPLACE_EXISTING, bypassing the
      // HAL. This is also duplicating the path normalization logic. It works without normalizing
      // locally, but may not support paths longer than MAX_PATH.
      TStringBuilder<256> sourceNormalized;
      appendNormalized(tempPath, sourceNormalized);
      TStringBuilder<256> destNormalized;
      appendNormalized(basePath, destNormalized);
      return !!MoveFileExW(*sourceNormalized, *destNormalized, MOVEFILE_REPLACE_EXISTING);
#else
      // This module should only be set to compile on windows, but
      // in the case that it isn't, I don't want to fall back to something untested.
      // This is likely easier with linux syscalls, which unreal HAL may already call
      return Super::SaveGame(attemptToUseUI, name, userIndex, data);
#endif
   }
};

ISaveGameSystem* FTATAtomicSaveModule::GetSaveGameSystem()
{
   static FTATAtomicSaveGameSystem sSaveSystem;
   return &sSaveSystem;
}

#undef LOCTEXT_NAMESPACE
   
IMPLEMENT_MODULE(FTATAtomicSaveModule, TATAtomicSave)
