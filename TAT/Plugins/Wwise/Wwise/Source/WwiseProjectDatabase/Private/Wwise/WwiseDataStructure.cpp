/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#include "Wwise/WwiseDataStructure.h"
#include "Wwise/WwiseDirectoryVisitor.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"

#include "Wwise/Metadata/WwiseMetadataAcousticTexture.h"
#include "Wwise/Metadata/WwiseMetadataBus.h"
#include "Wwise/Metadata/WwiseMetadataDialogue.h"
#include "Wwise/Metadata/WwiseMetadataEvent.h"
#include "Wwise/Metadata/WwiseMetadataExternalSource.h"
#include "Wwise/Metadata/WwiseMetadataGameParameter.h"
#include "Wwise/Metadata/WwiseMetadataLanguage.h"
#include "Wwise/Metadata/WwiseMetadataMedia.h"
#include "Wwise/Metadata/WwiseMetadataPlatformInfo.h"
#include "Wwise/Metadata/WwiseMetadataPlugin.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"
#include "Wwise/Metadata/WwiseMetadataPluginInfo.h"
#include "Wwise/Metadata/WwiseMetadataPluginLib.h"
#include "Wwise/Metadata/WwiseMetadataProjectInfo.h"
#include "Wwise/Metadata/WwiseMetadataRootFile.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"
#include "Wwise/Metadata/WwiseMetadataSoundBanksInfo.h"
#include "Wwise/Metadata/WwiseMetadataState.h"
#include "Wwise/Metadata/WwiseMetadataStateGroup.h"
#include "Wwise/Metadata/WwiseMetadataSwitch.h"
#include "Wwise/Metadata/WwiseMetadataSwitchContainer.h"
#include "Wwise/Metadata/WwiseMetadataSwitchGroup.h"
#include "Wwise/Metadata/WwiseMetadataTrigger.h"

WwiseDataStructure::WwiseDataStructure(const WwiseDBString& InDirectoryPath, const WwiseDBString* InPlatform)
{
    if (InDirectoryPath.IsEmpty())
    {
        return;
    }

    IWwiseDirectoryVisitor<WwiseDirectoryVisitor> DirectoryVisitor(new WwiseDirectoryVisitor(InPlatform));
    DirectoryVisitor.IterateDirectory(WwiseDBString(*InDirectoryPath));
    auto Directory = DirectoryVisitor.Get();
    if (!Directory.IsValid())
    {
        WWISE_DB_LOG(Error, "Invalid Generated Directory %s", *InDirectoryPath);
        return;
    }

    WwiseDBString RequestedPlatformPath;
    if (InPlatform)
    {
        const auto& ProjectInfoPlatforms = Directory.ProjectInfo->ProjectInfo->Platforms;
        for (const auto& Platform : ProjectInfoPlatforms)
        {
            if (Platform.Name == *InPlatform)
            {
                WwiseDBString PlatformPath = Platform.Path;
                
                RequestedPlatformPath = InDirectoryPath / Platform.Path;
                RequestedPlatformPath.CollapseRelativeDirectories();
                //If the platfrom path contains a drive, it means that the directory path and platform path are on different drives
                if (RequestedPlatformPath.IsRelative())
                {
                    RequestedPlatformPath = InDirectoryPath / PlatformPath;
                }
                else
                {
                    RequestedPlatformPath = PlatformPath;
                }
            }
        }
    }

    if (InPlatform)
    {
	    if (Directory.Platforms.Size() == 0)
	    {
	        WWISE_DB_LOG(Error, "Could not find platform %s in Generated Directory %s", **InPlatform, *RequestedPlatformPath);
	        return;
	    }

        WWISE_DB_LOG(Verbose, "Parsing Wwise data structure for platform %s at: %s...", **InPlatform, *RequestedPlatformPath);
    }
    else
    {
        WWISE_DB_LOG(Verbose, "Parsing Wwise data structure at: %s...", *InDirectoryPath);
    }


    LoadDataStructure(std::move(Directory));
}

WwiseDataStructure::~WwiseDataStructure()
{
}

void WwiseDataStructure::LoadDataStructure(WwiseGeneratedFiles&& Directory)
{
	WWISE_DB_LOG(VeryVerbose, "Starting load task");

    // Create the file lists to be used in loading root files
    WwiseDBArray<WwiseDBString> FileListForRoot;
    {
        const WwiseDBString ProjectInfoPath = Directory.GeneratedRootFiles.ProjectInfoFile.Get<0>();
        if (ProjectInfoPath.IsEmpty())
        {
            WWISE_DB_LOG(Error, "- Could not find project info");
        }
        else
        {
            WWISE_DB_LOG(VeryVerbose, "- Adding project info: %s", *ProjectInfoPath);
            FileListForRoot.Add(ProjectInfoPath);
        }
    }

    WwiseDBSharedFuture<WwiseRootDataStructure*> RootFuture = WwiseAsyncShared([this, &FileListForRoot, &Directory] {
        WWISE_DB_LOG(Verbose, "Loading Generated file contents for root");
        auto JsonFiles = WwiseMetadataRootFile::LoadFiles(FileListForRoot);

        WWISE_DB_LOG(Verbose, "Parsing Generated file contents for root");
        auto RootDataStructure = new WwiseRootDataStructure(std::move(JsonFiles));
        RootDataStructure->GeneratedRootFiles = std::move(Directory.GeneratedRootFiles);
        return RootDataStructure;
        });

    for(auto& Platform : Directory.Platforms)
    {
        WwiseDBPair<const WwiseDBSharedPlatformId, WwiseGeneratedFiles::FPlatformFiles> Pair(Platform);
        Platforms.Add(Pair.GetFirst(), WwisePlatformDataStructure());
    }
    // Create the file lists to be used in loading files per platform
    WwiseParallelFor(Directory.Platforms, [this, &RootFuture, &Directory](const auto& Platform)
    {
        WwiseDBPair<const WwiseDBSharedPlatformId, WwiseGeneratedFiles::FPlatformFiles> Pair(Directory.Platforms(Platform));
        WWISE_DB_LOG(VeryVerbose, "Loading platform files: %s", *Pair.GetFirst().GetPlatformName());
        WwiseDBArray<WwiseDBString> FileList;
        const WwiseDBSharedPlatformId& PlatformRef = Pair.GetFirst();
        const WwiseGeneratedFiles::FPlatformFiles& Files = Pair.GetSecond();

        // Add Platform and Plug-in files
        const WwiseDBString PlatformInfoPath = Files.PlatformInfoFile.Get<0>();
        {
            if (PlatformInfoPath.IsEmpty())
            {
                WWISE_DB_LOG(Error, "No PlatformInfo file for platform %s", *PlatformRef.GetPlatformName());
                return;
            }
            WWISE_DB_LOG(VeryVerbose, "- Adding platform info: %s", *PlatformInfoPath);
            FileList.Add(PlatformInfoPath);
        }
        {
            const WwiseDBString PluginInfoPath = Files.PluginInfoFile.Get<0>();
            if (PluginInfoPath.IsEmpty())
            {
                WWISE_DB_LOG(Error, "No PluginInfo file for platform %s", *PlatformRef.GetPlatformName());
                return;
            }
            WWISE_DB_LOG(VeryVerbose, "- Adding plugin info: %s", *PluginInfoPath);
            FileList.Add(PluginInfoPath);
        }

        // Parse PlatformInfo file to detect settings
        // (will be parsed twice. Now once, and officially later - since the file is small, it's not a big worry)
        WWISE_DB_LOG(VeryVerbose, "Pre-parsing platform info file for settings");
        auto PlatformInfoFile = WwiseMetadataRootFile::LoadFile(WwiseDBString(PlatformInfoPath));
        if (!PlatformInfoFile || !PlatformInfoFile->PlatformInfo)
        {
           WWISE_DB_LOG(Error, "Could not read PlatformInfo for platform %s.", *PlatformRef.GetPlatformName());
           return;
        }
        const auto& Settings = PlatformInfoFile->PlatformInfo->Settings;
        bool bIsValid = true;
        if (!Settings.bCopyLooseStreamedMediaFiles)
        {
            bIsValid = false;
            WWISE_DB_LOG(Error, "Platform %s: Requires \"Copy Loose/Streamed Media\".", *PlatformRef.GetPlatformName());
        }
        if (!Settings.bGenerateMetadataJSON)
        {
            bIsValid = false;
            WWISE_DB_LOG(Error, "Platform %s: Requires \"Generate JSON Metadata\".", *PlatformRef.GetPlatformName());
        }
        if (Settings.bGenerateAllBanksMetadata && Settings.bGeneratePerBankMetadata)
        {
            WWISE_DB_LOG(Log, "Platform %s: Having both \"Generate All Banks Metadata file\" and \"Generate Per Bank Metadata file\" will use the latter.", *PlatformRef.GetPlatformName());
        }
        else if (Settings.bGenerateAllBanksMetadata)
        {
            WWISE_DB_LOG(Log, "Platform %s: Using \"Generate All Banks Metadata file\" is less efficient than Per Bank.", *PlatformRef.GetPlatformName());
        }
        else if (!Settings.bGeneratePerBankMetadata)
        {
            bIsValid = false;
            WWISE_DB_LOG(Error, "Platform %s: No metadata generated. Requires one of the \"Generate Metadata file\" option set.", *PlatformRef.GetPlatformName());
        }
        if (!Settings.bPrintObjectGuid)
        {
            bIsValid = false;
            WWISE_DB_LOG(Error, "Platform %s: Requires \"Object GUID\" Metadata.", *PlatformRef.GetPlatformName());
        }
        if (!Settings.bPrintObjectPath)
        {
            bIsValid = false;
            WWISE_DB_LOG(Error, "Platform %s: Requires \"Object Path\" Metadata.", *PlatformRef.GetPlatformName());
        }
        if (!Settings.bMaxAttenuationInfo)
        {
            bIsValid = false;
            WWISE_DB_LOG(Error, "Platform %s: Requires \"Max Attenuation\" Metadata.", *PlatformRef.GetPlatformName());
        }
        if (!Settings.bEstimatedDurationInfo)
        {
            bIsValid = false;
            WWISE_DB_LOG(Error, "Platform %s: Requires \"Estimated Duration\" Metadata.", *PlatformRef.GetPlatformName());
        }
        if (!bIsValid)
        {
            WWISE_DB_LOG(Verbose, "Skipping platform");
            return;
        }

        // Monolithic SoundBanksInfo or split files
        if (Settings.bGeneratePerBankMetadata)
        {
            if ((Files.MetadataFiles.Size() == 0))
            {
                WWISE_DB_LOG(Error, "Platform %s: Generated Per Bank metadata, but no metadata file found.", *PlatformRef.GetPlatformName());
                return;
            }

            FileList.Reserve(FileList.Size() + Files.MetadataFiles.Size());
            for (const auto& MetadataFile : Files.MetadataFiles)
            {
                WwiseDBPair<WwiseDBString, WwiseDateTime> MetadataPair(MetadataFile);
                const auto Path = MetadataFile.Key.GetPath();
                if (Path.Contains("/Bus/"_wwise_db) || Path.Contains("\\Bus\\"_wwise_db))
                {
                    WWISE_DB_LOG(VeryVerbose, "- Skipping bus metadata file: %s", *MetadataPair.GetFirst());
                    continue;
                }
                WWISE_DB_LOG(VeryVerbose, "- Adding metadata file: %s", *MetadataPair.GetFirst());
                FileList.Add(MetadataPair.GetFirst());
            }
        }
        else if (!Files.SoundbanksInfoFile.Get<0>().IsEmpty())
        {
            WWISE_DB_LOG(VeryVerbose, "- Adding monolithic SoundBanks info file: %s", *Files.SoundbanksInfoFile.Get<0>());
            FileList.Add(Files.SoundbanksInfoFile.Get<0>());
        }
        else
        {
            WWISE_DB_LOG(Error, "Platform %s: Generated All Banks metadata, but SoundBanksInfo.json file not found.", *PlatformRef.GetPlatformName());
            return;
        }

        WWISE_DB_LOG(Verbose, "Loading Generated file contents for platform %s", *PlatformRef.GetPlatformName());
        auto JsonFiles = WwiseMetadataRootFile::LoadFiles(FileList);

        auto PlatformData = WwisePlatformDataStructure(PlatformRef, *RootFuture.Get(), std::move(JsonFiles));
        PlatformData.GeneratedPlatformFiles = Directory.Platforms[PlatformRef];
        auto& PlatformDataRef = Platforms.FindChecked(PlatformRef);
        PlatformDataRef += std::move(PlatformData);
        PlatformDataRef.Platform = Pair.GetFirst();
        PlatformDataRef.PlatformRef = PlatformData.PlatformRef;
        PlatformDataRef.Guids = PlatformData.Guids;
        PlatformDataRef.Names = PlatformData.Names;
        PlatformDataRef.MediaUsageCount = PlatformData.MediaUsageCount;
        PlatformDataRef.SoundBankUsageCount = PlatformData.SoundBankUsageCount;
    });

    WWISE_DB_LOG(VeryVerbose, "Finalizing parsing root");
    if (auto* Result = RootFuture.Get())
    {
        // Will move result. Must have all other Futures done
        RootData = std::move(*Result);
        delete Result;
    }
}


WwiseRootDataStructure::WwiseRootDataStructure(WwiseMetadataFileMap&& InJsonFiles) :
    JsonFiles(std::move(InJsonFiles))
{
	for (const auto& JsonFileKV : JsonFiles)
    {
	    WwiseDBPair<WwiseDBString, WwiseMetadataSharedRootFilePtr> Pair(JsonFileKV);
        const auto JsonFilePath = WwiseDBString(Pair.GetFirst());
        if (Pair.GetSecond())
        {
            const WwiseMetadataSharedRootFilePtr& SharedRootFile = Pair.GetSecond();

            if (SharedRootFile->ProjectInfo)
            {
                const WwiseMetadataProjectInfo& ProjectInfo = *SharedRootFile->ProjectInfo;

                // PlatformReferenceNames + PlatformReferenceGuids;
                for (WwiseRefIndexType PlatformIndex = 0; PlatformIndex < ProjectInfo.Platforms.Size(); ++PlatformIndex)
                {
                    const WwiseMetadataPlatformReference& PlatformReference = ProjectInfo.Platforms[PlatformIndex];
                    PlatformNames.Add(PlatformReference.Name, WwiseRefPlatform(SharedRootFile, JsonFilePath, PlatformIndex));
                    PlatformGuids.Add(PlatformReference.GUID, WwiseRefPlatform(SharedRootFile, JsonFilePath, PlatformIndex));
                    Platforms.Emplace(WwiseDBSharedPlatformId(PlatformReference.GUID, *PlatformReference.Name, *PlatformReference.Path));
                }

                // LanguageNames, LanguageIds, LanguageRefs
                for (WwiseRefIndexType LanguageIndex = 0; LanguageIndex < ProjectInfo.Languages.Size(); ++LanguageIndex)
                {
                    const WwiseMetadataLanguage& Language = ProjectInfo.Languages[LanguageIndex];
                    LanguageNames.Add(Language.Name, WwiseRefLanguage(SharedRootFile, JsonFilePath, LanguageIndex));
                    LanguageIds.Add(Language.Id, WwiseRefLanguage(SharedRootFile, JsonFilePath, LanguageIndex));
                    Languages.Emplace(WwiseDBSharedLanguageId(Language.Id, *Language.Name, Language.bDefault ? WwiseDBLanguageRequirement::IsDefault : WwiseDBLanguageRequirement::IsOptional), nullptr);
                }
            }
        }
    }
}

WwisePlatformDataStructure::WwisePlatformDataStructure() :
    AcousticTextures(WwiseRefAcousticTexture::FGlobalIdsMap::GlobalIdsMap),
    AudioDevices(WwiseRefAudioDevice::FGlobalIdsMap::GlobalIdsMap),
    AuxBusses(WwiseRefAuxBus::FGlobalIdsMap::GlobalIdsMap),
    Busses(WwiseRefBus::FGlobalIdsMap::GlobalIdsMap),
    CustomPlugins(WwiseRefCustomPlugin::FGlobalIdsMap::GlobalIdsMap),
    DialogueArguments(WwiseRefDialogueArgument::FGlobalIdsMap::GlobalIdsMap),
    DialogueEvents(WwiseRefDialogueEvent::FGlobalIdsMap::GlobalIdsMap),
    Events(WwiseRefEvent::FGlobalIdsMap::GlobalIdsMap),
    ExternalSources(WwiseRefExternalSource::FGlobalIdsMap::GlobalIdsMap),
    GameParameters(WwiseRefGameParameter::FGlobalIdsMap::GlobalIdsMap),
    MediaFiles(WwiseRefMedia::FGlobalIdsMap::GlobalIdsMap),
    PluginLibs(WwiseRefPluginLib::FGlobalIdsMap::GlobalIdsMap),
    PluginShareSets(WwiseRefPluginShareSet::FGlobalIdsMap::GlobalIdsMap),
    SoundBanks(WwiseRefSoundBank::FGlobalIdsMap::GlobalIdsMap),
    States(WwiseRefState::FGlobalIdsMap::GlobalIdsMap),
    StateGroups(WwiseRefStateGroup::FGlobalIdsMap::GlobalIdsMap),
    Switches(WwiseRefSwitch::FGlobalIdsMap::GlobalIdsMap),
    SwitchGroups(WwiseRefSwitchGroup::FGlobalIdsMap::GlobalIdsMap),
    Triggers(WwiseRefTrigger::FGlobalIdsMap::GlobalIdsMap)
{}

WwisePlatformDataStructure::WwisePlatformDataStructure(const WwiseDBSharedPlatformId& InPlatform, WwiseRootDataStructure& InRootData, WwiseMetadataFileMap&& InJsonFiles) :
    Platform(InPlatform),
    JsonFiles(std::move(InJsonFiles)),
    AcousticTextures(WwiseRefAcousticTexture::FGlobalIdsMap::GlobalIdsMap),
    AudioDevices(WwiseRefAudioDevice::FGlobalIdsMap::GlobalIdsMap),
    AuxBusses(WwiseRefAuxBus::FGlobalIdsMap::GlobalIdsMap),
    Busses(WwiseRefBus::FGlobalIdsMap::GlobalIdsMap),
    CustomPlugins(WwiseRefCustomPlugin::FGlobalIdsMap::GlobalIdsMap),
    DialogueArguments(WwiseRefDialogueArgument::FGlobalIdsMap::GlobalIdsMap),
    DialogueEvents(WwiseRefDialogueEvent::FGlobalIdsMap::GlobalIdsMap),
    Events(WwiseRefEvent::FGlobalIdsMap::GlobalIdsMap),
    ExternalSources(WwiseRefExternalSource::FGlobalIdsMap::GlobalIdsMap),
    GameParameters(WwiseRefGameParameter::FGlobalIdsMap::GlobalIdsMap),
    MediaFiles(WwiseRefMedia::FGlobalIdsMap::GlobalIdsMap),
    PluginLibs(WwiseRefPluginLib::FGlobalIdsMap::GlobalIdsMap),
    PluginShareSets(WwiseRefPluginShareSet::FGlobalIdsMap::GlobalIdsMap),
    SoundBanks(WwiseRefSoundBank::FGlobalIdsMap::GlobalIdsMap),
    States(WwiseRefState::FGlobalIdsMap::GlobalIdsMap),
    StateGroups(WwiseRefStateGroup::FGlobalIdsMap::GlobalIdsMap),
    Switches(WwiseRefSwitch::FGlobalIdsMap::GlobalIdsMap),
    SwitchGroups(WwiseRefSwitchGroup::FGlobalIdsMap::GlobalIdsMap),
    Triggers(WwiseRefTrigger::FGlobalIdsMap::GlobalIdsMap)
{
	for (const auto& JsonFileKV : JsonFiles)
    {
	    WwiseDBPair<WwiseDBString, WwiseMetadataSharedRootFilePtr> Pair(JsonFileKV);
        const auto JsonFilePath = WwiseDBString(Pair.GetFirst());
        if (Pair.GetSecond())
        {
            WwiseMetadataSharedRootFilePtr SharedRootFile = Pair.GetSecond();
            if (SharedRootFile->PlatformInfo)
            {
                // Platform have different information depending on its location.
                // Project's Platform contains the Path, generator version, and Guid.
                // PlatformInfo contains all the other information, including generation data.
                // So we must merge one into the other.
                const WwiseMetadataPlatformInfo& PlatformInfo = *SharedRootFile->PlatformInfo;
                WwiseRefPlatform NewPlatformRef(SharedRootFile, JsonFilePath);

                const auto& PlatformName = PlatformInfo.Platform.Name;

                // Update PlatformNames
                WwiseRefPlatform* RootPlatformByName = InRootData.PlatformNames.Find(PlatformName);
                if ((!RootPlatformByName))
                {
                    WWISE_DB_LOG(Error, "Could not find platform %s in ProjectInfo", *PlatformName);
                    continue;
                }
                RootPlatformByName->Merge(std::move(NewPlatformRef));

                // Update PlatformGUID
                const auto* PlatformReference = RootPlatformByName->GetPlatformReference();
                if(PlatformReference == nullptr)
                {
                    continue;
                }
                const auto& Guid = PlatformReference->GUID;
                WwiseRefPlatform* RootPlatformByGuid = InRootData.PlatformGuids.Find(Guid);
                if ((!RootPlatformByGuid))
                {
                    WWISE_DB_LOG(Error, "Could not find platform %s guid %s in ProjectInfo", *PlatformName, *Guid.ToString());
                    continue;
                }
                *RootPlatformByGuid = *RootPlatformByName;
                PlatformRef = *RootPlatformByName;
            }

            if (SharedRootFile->PluginInfo)
            {
                const WwiseMetadataPluginInfo& PluginInfo = *SharedRootFile->PluginInfo;

                // PluginLibNames + PluginLibIDs
                for (WwiseRefIndexType PluginLibIndex = 0; PluginLibIndex < PluginInfo.PluginLibs.Size(); ++PluginLibIndex)
                {
                    const WwiseMetadataPluginLib& PluginLib = PluginInfo.PluginLibs[PluginLibIndex];
                    const auto& PluginRef = WwiseRefPluginLib(SharedRootFile, JsonFilePath, PluginLibIndex);
                    AddRefToMap(PluginLibs, PluginRef, PluginLib.LibId, &PluginLib.LibName, nullptr, nullptr);
                    PluginLibNames.Add(PluginLib.LibName, PluginRef);
                }
            }

            if (SharedRootFile->ProjectInfo)
            {
                const WwiseMetadataProjectInfo& ProjectInfo = *SharedRootFile->ProjectInfo;
                // Should be loaded in WwiseRootDataStructure
            }

            if (SharedRootFile->SoundBanksInfo)
            {
                const WwiseMetadataSoundBanksInfo& SoundBanksInfo = *SharedRootFile->SoundBanksInfo;

                // SoundBanks
                for (WwiseRefIndexType SoundBankIndex = 0; SoundBankIndex < SoundBanksInfo.SoundBanks.Size(); ++SoundBankIndex)
                {
                    const WwiseMetadataSoundBank& SoundBank = SoundBanksInfo.SoundBanks[SoundBankIndex];
                    const WwiseDBShortId LanguageId = InRootData.GetLanguageId(SoundBank.Language);
                    AddRefToMap(SoundBanks, WwiseRefSoundBank(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId), SoundBank.Id, &SoundBank.ShortName, &SoundBank.ObjectPath, &SoundBank.GUID);

                    // Media
                    for (WwiseRefIndexType MediaIndex = 0; MediaIndex < SoundBank.Media.Size(); ++MediaIndex)
                    {
                        const WwiseMetadataMedia& File = SoundBank.Media[MediaIndex];
                        MediaFiles.Add(WwiseDatabaseMediaIdKey(File.Id, SoundBank.Id),
                            WwiseRefMedia(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, MediaIndex));
                    }

                    // DialogueEvents
                    for (WwiseRefIndexType DialogueEventIndex = 0; DialogueEventIndex < SoundBank.DialogueEvents.Size(); ++DialogueEventIndex)
                    {
                        const WwiseMetadataDialogueEvent& DialogueEvent = SoundBank.DialogueEvents[DialogueEventIndex];
                        AddBasicRefToMap(DialogueEvents, WwiseRefDialogueEvent(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, DialogueEventIndex), DialogueEvent);

                        // DialogueArguments
                        for (WwiseRefIndexType DialogueArgumentIndex = 0; DialogueArgumentIndex < DialogueEvent.Arguments.Size(); ++DialogueArgumentIndex)
                        {
                            const WwiseMetadataDialogueArgument& DialogueArgument = DialogueEvent.Arguments[DialogueArgumentIndex];
                            AddBasicRefToMap(DialogueArguments, WwiseRefDialogueArgument(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, DialogueEventIndex, DialogueArgumentIndex), DialogueArgument);
                        }
                    }

                    // We have multiple copies of the Busses. We currently want the Init Bank version.
                    if (SoundBank.IsInitBank())
                    {
                        AddSoundBankCount(SoundBank.Id, LanguageId);

                        // Busses
	                    for (WwiseRefIndexType BusIndex = 0; BusIndex < SoundBank.Busses.Size(); ++BusIndex)
	                    {
	                        const WwiseMetadataBus& Bus = SoundBank.Busses[BusIndex];
	                        AddBasicRefToMap(Busses, WwiseRefBus(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, BusIndex), Bus);
	                    }

	                    // AuxBusses
	                    for (WwiseRefIndexType AuxBusIndex = 0; AuxBusIndex < SoundBank.AuxBusses.Size(); ++AuxBusIndex)
	                    {
	                        const WwiseMetadataBus& AuxBus = SoundBank.AuxBusses[AuxBusIndex];
	                        AddBasicRefToMap(AuxBusses, WwiseRefAuxBus(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, AuxBusIndex), AuxBus);
	                    }
                    }

                    // Plugins
                    if (SoundBank.Plugins)
                    {
                        const auto& Plugins = *SoundBank.Plugins;

                        // CustomPlugins
                        for (WwiseRefIndexType CustomPluginIndex = 0; CustomPluginIndex < Plugins.Custom.Size(); ++CustomPluginIndex)
                        {
                            const WwiseMetadataPlugin& CustomPlugin = Plugins.Custom[CustomPluginIndex];
                            AddBasicRefToMap(CustomPlugins, WwiseRefCustomPlugin(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, CustomPluginIndex), CustomPlugin);
	                        AddMediaRefsCount(CustomPlugin.MediaRefs);
                        }

                        // PluginShareSets
                        for (WwiseRefIndexType PluginShareSetIndex = 0; PluginShareSetIndex < Plugins.ShareSets.Size(); ++PluginShareSetIndex)
                        {
                            const WwiseMetadataPlugin& PluginShareSet = Plugins.ShareSets[PluginShareSetIndex];
                            AddBasicRefToMap(PluginShareSets, WwiseRefPluginShareSet(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, PluginShareSetIndex), PluginShareSet);
	                        AddMediaRefsCount(PluginShareSet.MediaRefs);
                        }

                        // AudioDevices
                        for (WwiseRefIndexType AudioDeviceIndex = 0; AudioDeviceIndex < Plugins.AudioDevices.Size(); ++AudioDeviceIndex)
                        {
                            const WwiseMetadataPlugin& AudioDevice = Plugins.AudioDevices[AudioDeviceIndex];
                            AddBasicRefToMap(AudioDevices, WwiseRefAudioDevice(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, AudioDeviceIndex), AudioDevice);
	                        AddMediaRefsCount(AudioDevice.MediaRefs);
                        }
                    }

                    // Events
                    for (WwiseRefIndexType EventIndex = 0; EventIndex < SoundBank.Events.Size(); ++EventIndex)
                    {
                        const WwiseMetadataEvent& Event = SoundBank.Events[EventIndex];
                        AddEventRefToMap(Events, WwiseRefEvent(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, EventIndex), Event);
                        AddMediaRefsCount(Event.MediaRefs);
                        AddSoundBankCount(SoundBank.Id, LanguageId);

                        // Switch Containers
                        WwiseDBArray<WwiseRefIndexType> ContainerIndex;
                        ContainerIndex.Add(0);
                        while (true)
                        {
                            if (ContainerIndex.Size() == 0)
                            {
                                // Fully done
                                break;
                            }

                            // Retrieve Container
                            const WwiseMetadataSwitchContainer* Container = nullptr;
                            const auto* ContainerArray = &Event.SwitchContainers;
                            for (WwiseRefIndexType ContainerLevel = 0; ContainerLevel < ContainerIndex.Size() && ContainerArray; ++ContainerLevel)
                            {
                                WwiseRefIndexType CurrentIndex = ContainerIndex[ContainerLevel];
                                if (!ContainerArray->IsValidIndex(CurrentIndex))
                                {
                                    // Done last level
                                    ContainerArray = nullptr;
                                    Container = nullptr;
                                    break;
                                }
                                Container = &(*ContainerArray)[CurrentIndex];
                                ContainerArray = &Container->Children;

                                AddMediaRefsCount(Container->MediaRefs);
                            }

                            if (Container == nullptr)
                            {
                                // Done this level
                                ContainerIndex.Pop();

                                if (ContainerIndex.Size() > 0)
                                {
                                    ++ContainerIndex[ContainerIndex.Size() - 1];
                                }
                                continue;
                            }

                            if (Container->MediaRefs.Size() > 0 || Container->ExternalSourceRefs.Size() >0 || Container->PluginRefs != nullptr)
                            {
                                const auto& Ref = WwiseRefSwitchContainer(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, EventIndex, ContainerIndex);
                                SwitchContainersByEvent.Add(WwiseDatabaseLocalizableIdKey(Event.Id, LanguageId), Ref);
                            }

                            if (ContainerArray->Size() > 0)
                            {
                                // There are children. Add one sublevel
                                ContainerIndex.Add(0);
                            }
                            else
                            {
                                // No children. Next.
                                ++ContainerIndex[ContainerIndex.Size() - 1];
                            }
                        }
                    }

                    // ExternalSources
                    for (WwiseRefIndexType ExternalSourceIndex = 0; ExternalSourceIndex < SoundBank.ExternalSources.Size(); ++ExternalSourceIndex)
                    {
                        const WwiseMetadataExternalSource& ExternalSource = SoundBank.ExternalSources[ExternalSourceIndex];
                        AddRefToMap(ExternalSources, WwiseRefExternalSource(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, ExternalSourceIndex), ExternalSource.Cookie, &ExternalSource.Name, &ExternalSource.ObjectPath, &ExternalSource.GUID);
                    }

                    // AcousticTextures
                    for (WwiseRefIndexType AcousticTextureIndex = 0; AcousticTextureIndex < SoundBank.AcousticTextures.Size(); ++AcousticTextureIndex)
                    {
                        const WwiseMetadataAcousticTexture& AcousticTexture = SoundBank.AcousticTextures[AcousticTextureIndex];
                        AddBasicRefToMap(AcousticTextures, WwiseRefAcousticTexture(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, AcousticTextureIndex), AcousticTexture);
                    }

                    // GameParameters
                    for (WwiseRefIndexType GameParameterIndex = 0; GameParameterIndex < SoundBank.GameParameters.Size(); ++GameParameterIndex)
                    {
                        const WwiseMetadataGameParameter& GameParameter = SoundBank.GameParameters[GameParameterIndex];
                        AddBasicRefToMap(GameParameters, WwiseRefGameParameter(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, GameParameterIndex), GameParameter);
                    }

                    // StateGroups
                    for (WwiseRefIndexType StateGroupIndex = 0; StateGroupIndex < SoundBank.StateGroups.Size(); ++StateGroupIndex)
                    {
                        const WwiseMetadataStateGroup& StateGroup = SoundBank.StateGroups[StateGroupIndex];
                        AddBasicRefToMap(StateGroups, WwiseRefStateGroup(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, StateGroupIndex), StateGroup);
                        
                        // States
                        for (WwiseRefIndexType StateIndex = 0; StateIndex < StateGroup.States.Size(); ++StateIndex)
                        {
                            const WwiseMetadataState& State = StateGroup.States[StateIndex];
                            const WwiseRefState StateRef(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, StateGroupIndex, StateIndex);
                            const WwiseAnyRef AnyRef = WwiseAnyRef::Create(StateRef);
                            States.Add(WwiseDatabaseLocalizableGroupValueKey(StateGroup.Id, State.Id, LanguageId), StateRef);
                            if (State.GUID != WwiseDBGuid()) Guids.Add(WwiseDatabaseLocalizableGuidKey(State.GUID, LanguageId), AnyRef);
                            if (!State.Name.IsEmpty()) Names.Add(WwiseDatabaseLocalizableNameKey(State.Name, LanguageId), AnyRef);
                            if (!State.ObjectPath.IsEmpty()) Names.Add(WwiseDatabaseLocalizableNameKey(State.ObjectPath, LanguageId), AnyRef);
                        }
                    }

                    // SwitchGroups
                    for (WwiseRefIndexType SwitchGroupIndex = 0; SwitchGroupIndex < SoundBank.SwitchGroups.Size(); ++SwitchGroupIndex)
                    {
                        const WwiseMetadataSwitchGroup& SwitchGroup = SoundBank.SwitchGroups[SwitchGroupIndex];
                        AddBasicRefToMap(SwitchGroups, WwiseRefSwitchGroup(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, SwitchGroupIndex), SwitchGroup);

                        // Switches
                        for (WwiseRefIndexType SwitchIndex = 0; SwitchIndex < SwitchGroup.Switches.Size(); ++SwitchIndex)
                        {
                            const WwiseMetadataSwitch& Switch = SwitchGroup.Switches[SwitchIndex];
                            const WwiseRefSwitch SwitchRef(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, SwitchGroupIndex, SwitchIndex);
                            const WwiseAnyRef AnyRef = WwiseAnyRef::Create(SwitchRef);
                            Switches.Add(WwiseDatabaseLocalizableGroupValueKey(SwitchGroup.Id, Switch.Id, LanguageId), SwitchRef);
                            if (Switch.GUID != WwiseDBGuid()) Guids.Add(WwiseDatabaseLocalizableGuidKey(Switch.GUID, LanguageId), AnyRef);
                            if (!Switch.Name.IsEmpty()) Names.Add(WwiseDatabaseLocalizableNameKey(Switch.Name, LanguageId), AnyRef);
                            if (!Switch.ObjectPath.IsEmpty()) Names.Add(WwiseDatabaseLocalizableNameKey(Switch.ObjectPath, LanguageId), AnyRef);
                        }
                    }

                    // Triggers
                    for (WwiseRefIndexType TriggerIndex = 0; TriggerIndex < SoundBank.Triggers.Size(); ++TriggerIndex)
                    {
                        const WwiseMetadataTrigger& Trigger = SoundBank.Triggers[TriggerIndex];
                        AddBasicRefToMap(Triggers, WwiseRefTrigger(SharedRootFile, JsonFilePath, SoundBankIndex, LanguageId, TriggerIndex), Trigger);
                    }
                }
            }
        }
    }
}

WwisePlatformDataStructure::WwisePlatformDataStructure(const WwisePlatformDataStructure& Rhs) :
    WwiseRefAcousticTexture::FGlobalIdsMap(Rhs),
    WwiseRefAudioDevice::FGlobalIdsMap(Rhs),
    WwiseRefAuxBus::FGlobalIdsMap(Rhs),
    WwiseRefBus::FGlobalIdsMap(Rhs),
    WwiseRefCustomPlugin::FGlobalIdsMap(Rhs),
    WwiseRefDialogueArgument::FGlobalIdsMap(Rhs),
    WwiseRefDialogueEvent::FGlobalIdsMap(Rhs),
    WwiseRefEvent::FGlobalIdsMap(Rhs),
    WwiseRefExternalSource::FGlobalIdsMap(Rhs),
    WwiseRefGameParameter::FGlobalIdsMap(Rhs),
    WwiseRefMedia::FGlobalIdsMap(Rhs),
    WwiseRefPluginLib::FGlobalIdsMap(Rhs),
    WwiseRefPluginShareSet::FGlobalIdsMap(Rhs),
    WwiseRefSoundBank::FGlobalIdsMap(Rhs),
    WwiseRefState::FGlobalIdsMap(Rhs),
    WwiseRefStateGroup::FGlobalIdsMap(Rhs),
    WwiseRefSwitch::FGlobalIdsMap(Rhs),
    WwiseRefSwitchGroup::FGlobalIdsMap(Rhs),
    WwiseRefTrigger::FGlobalIdsMap(Rhs),
    Platform(Rhs.Platform),
    PlatformRef(Rhs.PlatformRef),
    GeneratedPlatformFiles(Rhs.GeneratedPlatformFiles),
    JsonFiles(Rhs.JsonFiles),
    AcousticTextures(WwiseRefAcousticTexture::FGlobalIdsMap::GlobalIdsMap),
    AudioDevices(WwiseRefAudioDevice::FGlobalIdsMap::GlobalIdsMap),
    AuxBusses(WwiseRefAuxBus::FGlobalIdsMap::GlobalIdsMap),
    Busses(WwiseRefBus::FGlobalIdsMap::GlobalIdsMap),
    CustomPlugins(WwiseRefCustomPlugin::FGlobalIdsMap::GlobalIdsMap),
    DialogueArguments(WwiseRefDialogueArgument::FGlobalIdsMap::GlobalIdsMap),
    DialogueEvents(WwiseRefDialogueEvent::FGlobalIdsMap::GlobalIdsMap),
    Events(WwiseRefEvent::FGlobalIdsMap::GlobalIdsMap),
    ExternalSources(WwiseRefExternalSource::FGlobalIdsMap::GlobalIdsMap),
    GameParameters(WwiseRefGameParameter::FGlobalIdsMap::GlobalIdsMap),
    MediaFiles(WwiseRefMedia::FGlobalIdsMap::GlobalIdsMap),
    PluginLibs(WwiseRefPluginLib::FGlobalIdsMap::GlobalIdsMap),
    PluginShareSets(WwiseRefPluginShareSet::FGlobalIdsMap::GlobalIdsMap),
    SoundBanks(WwiseRefSoundBank::FGlobalIdsMap::GlobalIdsMap),
    States(WwiseRefState::FGlobalIdsMap::GlobalIdsMap),
    StateGroups(WwiseRefStateGroup::FGlobalIdsMap::GlobalIdsMap),
    Switches(WwiseRefSwitch::FGlobalIdsMap::GlobalIdsMap),
    SwitchGroups(WwiseRefSwitchGroup::FGlobalIdsMap::GlobalIdsMap),
    Triggers(WwiseRefTrigger::FGlobalIdsMap::GlobalIdsMap),
    PluginLibNames(Rhs.PluginLibNames),
    SwitchContainersByEvent(Rhs.SwitchContainersByEvent),
    Guids(Rhs.Guids),
    Names(Rhs.Names),
    MediaUsageCount(Rhs.MediaUsageCount),
    SoundBankUsageCount(Rhs.SoundBankUsageCount)
{}

WwisePlatformDataStructure::WwisePlatformDataStructure(WwisePlatformDataStructure&& Rhs) :
    WwiseRefAcousticTexture::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefAudioDevice::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefAuxBus::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefBus::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefCustomPlugin::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefDialogueArgument::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefDialogueEvent::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefEvent::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefExternalSource::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefGameParameter::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefMedia::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefPluginLib::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefPluginShareSet::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefSoundBank::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefState::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefStateGroup::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefSwitch::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefSwitchGroup::FGlobalIdsMap(std::move(Rhs)),
    WwiseRefTrigger::FGlobalIdsMap(std::move(Rhs)),
    Platform(std::move(Rhs.Platform)),
    PlatformRef(std::move(Rhs.PlatformRef)),
    GeneratedPlatformFiles(std::move(Rhs.GeneratedPlatformFiles)),
    JsonFiles(std::move(Rhs.JsonFiles)),
    AcousticTextures(WwiseRefAcousticTexture::FGlobalIdsMap::GlobalIdsMap),
    AudioDevices(WwiseRefAudioDevice::FGlobalIdsMap::GlobalIdsMap),
    AuxBusses(WwiseRefAuxBus::FGlobalIdsMap::GlobalIdsMap),
    Busses(WwiseRefBus::FGlobalIdsMap::GlobalIdsMap),
    CustomPlugins(WwiseRefCustomPlugin::FGlobalIdsMap::GlobalIdsMap),
    DialogueArguments(WwiseRefDialogueArgument::FGlobalIdsMap::GlobalIdsMap),
    DialogueEvents(WwiseRefDialogueEvent::FGlobalIdsMap::GlobalIdsMap),
    Events(WwiseRefEvent::FGlobalIdsMap::GlobalIdsMap),
    ExternalSources(WwiseRefExternalSource::FGlobalIdsMap::GlobalIdsMap),
    GameParameters(WwiseRefGameParameter::FGlobalIdsMap::GlobalIdsMap),
    MediaFiles(WwiseRefMedia::FGlobalIdsMap::GlobalIdsMap),
    PluginLibs(WwiseRefPluginLib::FGlobalIdsMap::GlobalIdsMap),
    PluginShareSets(WwiseRefPluginShareSet::FGlobalIdsMap::GlobalIdsMap),
    SoundBanks(WwiseRefSoundBank::FGlobalIdsMap::GlobalIdsMap),
    States(WwiseRefState::FGlobalIdsMap::GlobalIdsMap),
    StateGroups(WwiseRefStateGroup::FGlobalIdsMap::GlobalIdsMap),
    Switches(WwiseRefSwitch::FGlobalIdsMap::GlobalIdsMap),
    SwitchGroups(WwiseRefSwitchGroup::FGlobalIdsMap::GlobalIdsMap),
    Triggers(WwiseRefTrigger::FGlobalIdsMap::GlobalIdsMap),
    PluginLibNames(std::move(Rhs.PluginLibNames)),
    SwitchContainersByEvent(std::move(Rhs.SwitchContainersByEvent)),
    Guids(std::move(Rhs.Guids)),
    Names(std::move(Rhs.Names)),
    MediaUsageCount(std::move(Rhs.MediaUsageCount)),
    SoundBankUsageCount(std::move(Rhs.SoundBankUsageCount))
{}

WwiseRootDataStructure& WwiseRootDataStructure::operator+=(WwiseRootDataStructure&& Rhs)
{
    //GeneratedRootFiles += std::move(Rhs.GeneratedRootFiles);
    JsonFiles.Append(std::move(Rhs.JsonFiles));
    LanguageNames.Append(std::move(Rhs.LanguageNames));
    LanguageIds.Append(std::move(Rhs.LanguageIds));
    PlatformNames.Append(std::move(Rhs.PlatformNames));
    PlatformGuids.Append(std::move(Rhs.PlatformGuids));
    return *this;
}

WwisePlatformDataStructure& WwisePlatformDataStructure::operator+=(WwisePlatformDataStructure&& Rhs)
{
    GeneratedPlatformFiles.Append(std::move(Rhs.GeneratedPlatformFiles));
    JsonFiles.Append(std::move(Rhs.JsonFiles));
    MediaFiles.Append(std::move(Rhs.MediaFiles));
    PluginLibNames.Append(std::move(Rhs.PluginLibNames));
    PluginLibs.Append(std::move(Rhs.PluginLibs));
    PluginShareSets.Append(std::move(Rhs.PluginShareSets));
    CustomPlugins.Append(std::move(Rhs.CustomPlugins));
    SoundBanks.Append(std::move(Rhs.SoundBanks));
    DialogueEvents.Append(std::move(Rhs.DialogueEvents));
    DialogueArguments.Append(std::move(Rhs.DialogueArguments));
    Busses.Append(std::move(Rhs.Busses));
    AuxBusses.Append(std::move(Rhs.AuxBusses));
    Events.Append(std::move(Rhs.Events));
    ExternalSources.Append(std::move(Rhs.ExternalSources));
    AcousticTextures.Append(std::move(Rhs.AcousticTextures));
    GameParameters.Append(std::move(Rhs.GameParameters));
    StateGroups.Append(std::move(Rhs.StateGroups));
    SwitchGroups.Append(std::move(Rhs.SwitchGroups));
    Triggers.Append(std::move(Rhs.Triggers));
    States.Append(std::move(Rhs.States));
    Switches.Append(std::move(Rhs.Switches));
    SwitchContainersByEvent.Append(std::move(Rhs.SwitchContainersByEvent));
    AudioDevices.Append(std::move(Rhs.AudioDevices));
    return *this;
}

bool WwisePlatformDataStructure::GetFromId(WwiseRefMedia& OutRef, WwiseDBShortId InShortId, WwiseDBShortId InLanguageId, WwiseDBShortId InSoundBankId) const
{
    if (InSoundBankId != 0)
    {
        WwiseDatabaseMediaIdKey MediaId(InShortId, InSoundBankId);
        if (MediaFiles.Contains(MediaId))
        {
            OutRef = MediaFiles.FindChecked(MediaId);   
        }
    }
    else
    {
        for (const auto& MediaFile : MediaFiles)
        {
            WwiseDBPair<MediaIdKey, WwiseRefMedia> Pair(MediaFile);
            if (Pair.GetFirst().MediaId == InShortId &&
                Pair.GetSecond().GetMedia()->Location != WwiseMetadataMediaLocation::OtherBank)
            {
                OutRef = MediaFiles.FindChecked(Pair.GetFirst());
                break;
            }
        }
    }

    if (!OutRef.IsValid())
    {
        WWISE_DB_LOG(Warning, "Could not find Media %" PRIu32 " (Lang=%" PRIu32 "; SB=%" PRIu32 ")", InShortId, InLanguageId, InSoundBankId);
        return false;
    }

    return true;
}

void WwisePlatformDataStructure::AddMediaRefsCount(const WwiseDBArray<WwiseMetadataMediaReference>& InMediaRefs)
{
    for (const auto& MediaRef : InMediaRefs)
    {
        const WwiseDatabaseMediaIdKey MediaId{ MediaRef.Id, 0 };
        if (auto CountPtr = MediaUsageCount.Find(MediaId))
        {
            ++(*CountPtr);
        }
        else
        {
            MediaUsageCount.Add(MediaId, 1);
        }
    }
}

void WwisePlatformDataStructure::AddSoundBankCount(const uint32 InId, const uint32 InLanguageId)
{
    const WwiseDatabaseLocalizableIdKey SoundBankId{ InId, InLanguageId };
    if (auto CountPtr = SoundBankUsageCount.Find(SoundBankId))
    {
        ++(*CountPtr);
    }
    else
    {
        SoundBankUsageCount.Add(SoundBankId, 1);
    }
}

WwiseDataStructure& WwiseDataStructure::operator+=(WwiseDataStructure&& Rhs)
{
    std::scoped_lock ScopeLockLhs(Lock);
    std::scoped_lock ScopeLockRhs(Rhs.Lock);

    Platforms.Append(std::move(Rhs.Platforms));

    return *this;
}
