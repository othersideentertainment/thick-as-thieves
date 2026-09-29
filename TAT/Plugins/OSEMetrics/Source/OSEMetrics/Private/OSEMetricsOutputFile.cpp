// (c) 2022-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEMetricsOutputFile.h"

// ose
#include "OSEMetrics.h"
#include "OSEMetricsSystem.h"

// std
#include <shared_mutex>

DECLARE_CYCLE_STAT(TEXT("OSEMetricsOutputFile: Tick"), STAT_FileOutput_Tick, STATGROUP_OSEMetrics);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("OSEMetricsOutputFile: Time Spent in Query (ms)"), STAT_FileOutput_QueryTimeMs, STATGROUP_OSEMetrics);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("OSEMetricsOutputFile: Total Query Count"), STAT_FileOutput_QueryCount, STATGROUP_OSEMetrics);

static FString GetMetricsFilePath(const FDateTime& timestamp, int32 index, bool useProfilingDir)
{
   FString fileName;
   if (index <= 0)
   {
      fileName = FString::Printf(TEXT("OSEMetrics_%s.json"), *timestamp.ToString());
   }
   else
   {
      fileName = FString::Printf(TEXT("OSEMetrics_%s (%i).json"), *timestamp.ToString(), index);
   }
   return FPaths::ConvertRelativePathToFull(useProfilingDir ? FPaths::ProfilingDir() : FPaths::ProjectLogDir()) / fileName;
}

static bool NormalizeOutputPathAndCreateDirectoriesIfNeeded(FString& inOutPath)
{
   auto createDirectoryIfNeeded = [](const FString& path) -> bool
   {
      FString basePart, filenamePart, extPart;
      FPaths::Split(path, basePart, filenamePart, extPart);
      if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*basePart))
      {
         return FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*basePart);
      }
      return true;
   };

   if (FPaths::IsRelative(inOutPath))
   {
      inOutPath = FPaths::ConvertRelativePathToFull(inOutPath);
   }
   FPaths::NormalizeDirectoryName(inOutPath);
   FPaths::CollapseRelativeDirectories(inOutPath);
   return createDirectoryIfNeeded(inOutPath);
}

static TOptional<FString> MakeOutputFilePath(bool useProfilingDir)
{
   const FDateTime startTimestamp = FDateTime::UtcNow();
   int32 filenameIndex = 0;
   FString filePath = GetMetricsFilePath(startTimestamp, filenameIndex, useProfilingDir);
   constexpr int32 maxFilenameIters = 1000;
   while (IFileManager::Get().FileExists(*filePath))
   {
      ++filenameIndex;
      filePath = GetMetricsFilePath(startTimestamp, filenameIndex, useProfilingDir);
      if (filenameIndex > maxFilenameIters)
      {
         UE_LOG(LogOSEMetrics, Error, TEXT("Failed to generate filename for metrics data output. Tried %i variations on '%s'."), filenameIndex, *filePath);
         return NullOpt;
      }
   }
   if (!NormalizeOutputPathAndCreateDirectoriesIfNeeded(filePath))
   {
      UE_LOG(LogOSEMetrics, Error, TEXT("Failed to find valid path to write metrics data to. Tried %s"), *filePath);
      return NullOpt;
   }
   return filePath;
}

FOSEMetricsOutputFile::FOSEMetricsOutputFile(float queryIntervalSeconds, float flushIntervalSeconds, bool useProfilingDir)
   : _queryIntervalSeconds(queryIntervalSeconds)
   , _flushIntervalSeconds(flushIntervalSeconds)
   , _useProfilingDir(useProfilingDir)
{
}

FOSEMetricsOutputFile::~FOSEMetricsOutputFile()
{
   _StopThread();
}

bool FOSEMetricsOutputFile::Enable()
{
   if (_thread.IsValid())
   {
      return true;
   }

   _threadRunning.store(true);

   TWeakPtr<FOSEMetricsOutputFile> weakThis = AsWeak();
   _thread = MakeUnique<FThread>(TEXT("FOSEMetricsOutputFile"),
      [weakThis, useProfilingDir = this->_useProfilingDir, flushInterval = _flushIntervalSeconds]()
      {
         if (TSharedPtr<FOSEMetricsOutputFile> self = weakThis.Pin())
         {
            const TOptional<FString> outputPath = MakeOutputFilePath(useProfilingDir);
            if (!outputPath)
            {
               UE_LOG(LogOSEMetrics, Error, TEXT("FOSEMetricsOutputFile: Failed to find valid output filename; no data will be written to disk"));
               return;
            }
            self->_ThreadMain(*outputPath, FMath::Max(0.01f, flushInterval));
         }
      });
   return _thread.IsValid();
}

void FOSEMetricsOutputFile::Disable()
{
   _StopThread();
}

void FOSEMetricsOutputFile::Tick(float deltaSeconds)
{
   ensureMsgf(IsInGameThread(), TEXT("FOSEMetricsOutputFile::Tick called outside of the game thread!"));

   SCOPE_CYCLE_COUNTER(STAT_FileOutput_Tick);

   timeSinceLastQuery += deltaSeconds;
   if (timeSinceLastQuery >= _queryIntervalSeconds)
   {
      timeSinceLastQuery = 0.0f;

      if (TSharedPtr<FOSEMetricsSystem> metricsSystem = _GetMetricsSystem())
      {
#if STATS
         const double startTimeMs = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());
#endif

         WriteMetrics(metricsSystem->QueryAll());

#if STATS
         SET_FLOAT_STAT(STAT_FileOutput_QueryTimeMs, static_cast<float>(FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64()) - startTimeMs));
         INC_DWORD_STAT(STAT_FileOutput_QueryCount);
#endif
      }
   }
}

void FOSEMetricsOutputFile::OnMetricGroupAdded(FName groupName)
{
}

void FOSEMetricsOutputFile::OnMetricAdded(FName groupName, FName metricName)
{
}

FString FOSEMetricsOutputFile::ToString() const
{
   return FString::Printf(TEXT("FOSEMetricsOutputFile(path: \"%s\")"), *_outputFilePath);
}

void FOSEMetricsOutputFile::WriteMetrics(FOSEMetricsResult&& metricsResult)
{
   if (metricsResult.IsValid())
   {
      std::unique_lock lock(_pendingOutputDataMutex);
      _pendingOutputData.Add(MoveTemp(metricsResult.Data));
   }
   else
   {
      UE_LOG(LogOSEMetrics, Error, TEXT("Got invalid metrics data: %s"), *metricsResult.ErrorMessage);
   }
}

void FOSEMetricsOutputFile::_StopThread()
{
   if (_thread)
   {
      _threadRunning.store(false);
      _thread->Join();
      _thread.Reset();
   }
}

void FOSEMetricsOutputFile::_ThreadMain(const FString& filePath, float flushInterval)
{
   double lastFlushTime = 0.0;

   // open the file for writing
   TUniquePtr<FArchive> metricsFile{ IFileManager::Get().CreateFileWriter(*filePath) };
   if (metricsFile)
   {
      UE_LOG(LogOSEMetrics, Log, TEXT("FOSEMetricsOutputFile: Opened file for writing metrics data: %s"), *filePath);
   }
   else
   {
      UE_LOG(LogOSEMetrics, Error, TEXT("FOSEMetricsOutputFile: Failed to open metrics file for writing: %s"), *filePath);
      return;
   }

   while (_threadRunning && !IsEngineExitRequested())
   {
      const double now = FPlatformTime::Seconds();
      if (lastFlushTime <= 0 || static_cast<float>(now - lastFlushTime) >= flushInterval)
      {
         lastFlushTime = now;

         {
            std::unique_lock lock(_pendingOutputDataMutex);
            uint8 lineEnding[] = { '\n', 0 };
            for (const TSharedPtr<FJsonObject>& obj : _pendingOutputData)
            {
               if (!obj)
               {
                  continue;
               }
               FString jsonString;
               if (FJsonSerializer::Serialize(obj.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&jsonString)))
               {
                  FTCHARToUTF8 converter{ *jsonString };
                  metricsFile->Serialize((void*)converter.Get(), converter.Length());
                  metricsFile->Serialize(lineEnding, sizeof(lineEnding) - 1);
               }
            }
            _pendingOutputData.Reset();
         }

         //UE_LOG(LogOSEMetrics, Verbose, TEXT("Flush metrics data to %s"), *filePath);
         metricsFile->Flush();
      }

      // Cap the sleep time to make sure we don't take too long to stop the thread
      FPlatformProcess::Sleep(FMath::Clamp(flushInterval, 0.01f, 0.5f));
   }

   UE_LOG(LogOSEMetrics, Log, TEXT("Closing metrics file %s"), *filePath);
   metricsFile->Close();

   UE_LOG(LogOSEMetrics, Log, TEXT("Metrics output file thread stopping normally"));
}
